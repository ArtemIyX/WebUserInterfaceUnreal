#include "Slate/CefBrowserSurface.h"

#include "CefWebUi.h"
#include "ID3D12DynamicRHI.h"
#include "InputCoreTypes.h"
#include "RenderGraphUtils.h"
#include "RenderingThread.h"
#include "RHI.h"
#include "Slate/CefBrowserSurfaceConstants.h"
#include "Slate/CefBrowserSurfaceFunctions.h"
#include "Slate/CefBrowserSurfaceStats.h"
#include "Sessions/CefWebUiBrowserSession.h"
#include "Services/CefControlWriter.h"
#include "Services/CefInputWriter.h"
#include "Services/CefSharedMemoryNames.h"
#include "Logging/StructuredLog.h"
#include "Misc/ScopeLock.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <d3d12.h>
#include "Windows/HideWindowsPlatformTypes.h"

class FCefBrowserSurfaceResourceState : public TSharedFromThis<FCefBrowserSurfaceResourceState, ESPMode::ThreadSafe>
{
public:
	FCriticalSection Lock;
	FTextureRHIRef MainTextures[3];
	FTextureRHIRef PopupTexture;
	TRefCountPtr<ID3D12Fence> SharedGpuFence;
	uint64 Generation = 1;
	uint64 LastFrameId = 0;
	uint32 OpenRetryCount = 0;
	bool bMainOpenPending = false;
	bool bPopupOpenPending = false;
	bool bFenceOpenPending = false;
	bool bRetryPending = false;
};

class FCefBrowserSurfaceDrawer : public ICustomSlateElement, public TSharedFromThis<FCefBrowserSurfaceDrawer, ESPMode::ThreadSafe>
{
public:
	void EnqueueUpdate(
		const FTextureRHIRef& InMainTexture,
		const FTextureRHIRef& InPopupTexture,
		const FCefSharedFrame& InFrame,
		const FIntRect& InDestRect,
		uint64 InGeneration)
	{
		TSharedRef<FCefBrowserSurfaceDrawer, ESPMode::ThreadSafe> self = AsShared();
		const FIntPoint SourceSize(
			FMath::Max(1, static_cast<int32>(InFrame.Width)),
			FMath::Max(1, static_cast<int32>(InFrame.Height)));
		const bool bInPopupPlane = ((InFrame.Flags & CefFrameFlag_PopupPlane) != 0);
		const bool bInPopupVisible = InFrame.bPopupVisible;
		const FIntRect popupRectangle(
			InFrame.PopupRect.X,
			InFrame.PopupRect.Y,
			InFrame.PopupRect.X + InFrame.PopupRect.W,
			InFrame.PopupRect.Y + InFrame.PopupRect.H);

		ENQUEUE_RENDER_COMMAND(CefSlateDrawerUpdate)(
			[self, InMainTexture, InPopupTexture, InDestRect, SourceSize, popupRectangle, bInPopupVisible, bInPopupPlane, InGeneration](FRHICommandListImmediate&) {
				self->mainTexture = InMainTexture;
				self->popupTexture = InPopupTexture;
				self->destRect = InDestRect;
				self->srcSize = SourceSize;
				self->popupRect = popupRectangle;
				self->bPopupVisible = bInPopupVisible;
				self->bPopupPlane = bInPopupPlane;
				self->generation = InGeneration;
			});
	}

	virtual void Draw_RenderThread(FRDGBuilder& graphBuilder, const FDrawPassInputs& inputs) override
	{
		if (!mainTexture.IsValid() || !inputs.OutputTexture)
		{
			return;
		}

		const FIntRect outputRect(FIntPoint::ZeroValue, inputs.OutputTexture->Desc.Extent);
		const FIntRect clippedDest = CefWebUi::BrowserSurface::MakeIntersection(CefWebUi::BrowserSurface::MakeIntersection(destRect, inputs.SceneViewRect), outputRect);
		if (clippedDest.Width() <= 0 || clippedDest.Height() <= 0)
		{
			return;
		}

		FRDGTextureRef mainTex = RegisterExternalTexture(graphBuilder, mainTexture.GetReference(), TEXT("CefSlateMainTex"), ERDGTextureFlags::ForceImmediateFirstBarrier);
		const FIntPoint mainExtent = mainTex->Desc.Extent;
		const FIntRect srcRect(0, 0, FMath::Min(srcSize.X, mainExtent.X), FMath::Min(srcSize.Y, mainExtent.Y));
		CefWebUi::BrowserSurface::AddCefSlateBlitPass(
			graphBuilder,
			mainTex,
			inputs.OutputTexture,
			destRect,
			clippedDest,
			srcRect,
			mainExtent,
			CefWebUi::BrowserSurface::ECefSlateBlitPlane::Main);

		if (!bPopupPlane || !bPopupVisible || !popupTexture.IsValid() || popupRect.Width() <= 0 || popupRect.Height() <= 0)
		{
			return;
		}

		const FIntRect popupMappingDest = CefWebUi::BrowserSurface::MapSourceRectToDestRect(popupRect, FIntRect(0, 0, srcSize.X, srcSize.Y), destRect);
		const FIntRect popupDrawDest = CefWebUi::BrowserSurface::MakeIntersection(
			CefWebUi::BrowserSurface::MakeIntersection(popupMappingDest, clippedDest),
			outputRect);
		if (popupDrawDest.Width() <= 0 || popupDrawDest.Height() <= 0)
		{
			return;
		}

		FRDGTextureRef popupTex = RegisterExternalTexture(graphBuilder, popupTexture.GetReference(), TEXT("CefSlatePopupTex"), ERDGTextureFlags::ForceImmediateFirstBarrier);
		const FIntPoint popupExtent = popupTex->Desc.Extent;
		CefWebUi::BrowserSurface::AddCefSlateBlitPass(
			graphBuilder,
			popupTex,
			inputs.OutputTexture,
			popupMappingDest,
			popupDrawDest,
			popupRect,
			popupExtent,
			CefWebUi::BrowserSurface::ECefSlateBlitPlane::Popup);
	}

private:
	FTextureRHIRef mainTexture;
	FTextureRHIRef popupTexture;
	FIntRect destRect;
	FIntRect popupRect;
	FIntPoint srcSize = FIntPoint::ZeroValue;
	bool bPopupVisible = false;
	bool bPopupPlane = false;
	uint64 generation = 0;
};

namespace
{
	FCefSharedMemoryNames GetSurfaceSharedNames(const TWeakObjectPtr<UCefWebUiBrowserSession>& InSession)
	{
		const FString sessionId = InSession.IsValid() ? InSession->GetSessionId().ToString() : FString();
		return BuildCefSharedMemoryNames(BuildCefSessionScope(sessionId));
	}
}

SCefBrowserSurface::~SCefBrowserSurface()
{
	ReleaseResources();
}

void SCefBrowserSurface::Construct(const FArguments& inArgs)
{
	ResourceState = MakeShared<FCefBrowserSurfaceResourceState, ESPMode::ThreadSafe>();
	BrowserSession = inArgs._BrowserSession;
	DesiredBrowserWidth = FMath::Max(1, inArgs._BrowserWidth);
	DesiredBrowserHeight = FMath::Max(1, inArgs._BrowserHeight);
	AppliedBrowserWidth = DesiredBrowserWidth;
	AppliedBrowserHeight = DesiredBrowserHeight;
	TargetBrowserWidth = DesiredBrowserWidth;
	TargetBrowserHeight = DesiredBrowserHeight;
	LastSentBrowserWidth = DesiredBrowserWidth;
	LastSentBrowserHeight = DesiredBrowserHeight;
	bAutoResizePending = false;
	bAwaitingAutoResizeApply = false;
	AutoResizeRetryCount = 0;
	bAutoResizeFailureLogged = false;
	LastAutoResizeObservedTimeSec = 0.0;
	LastAutoResizeSentTimeSec = 0.0;
	LastAutoResizeAwaitStartTimeSec = 0.0;
	LastConsumerFrameTimeSec = 0.0;
	LastCadenceSentTimeSec = 0.0;
	LastHostTuningPushTimeSec = 0.0;
	LastTelemetryLogTimeSec = 0.0;
	LastInputEventTimeSec = 0.0;
	LastInputToFrameLatencyMs = 0.0;
	SmoothedCadenceUs = 0;
	LastSeenFrameId = 0;
	InputEventSerial = 0;
	LastLatencyMeasuredInputSerial = 0;
	LastLatencyFrameId = 0;
	LastCursorType = ECefCustomCursorType::CT_NONE;
	TelemetryConsumedFrames = 0;
	TelemetryFrameGapCount = 0;
	TelemetryInputLatencySamples = 0;
	TelemetryInputLatencyMsSum = 0;
	TelemetryInputLatencyMsMax = 0;
	TelemetryPaintCalls = 0;
	TelemetryTickCalls = 0;
	TelemetryTimerCalls = 0;
	TelemetryPollSuccess = 0;
	TelemetryFenceNotReadyCount = 0;
	SET_DWORD_STAT(STAT_CefTel_ConsumedFrames, 0);
	SET_DWORD_STAT(STAT_CefTel_FrameGaps, 0);
	SET_DWORD_STAT(STAT_CefTel_InputToFrameLatencyMs, 0);
	SET_DWORD_STAT(STAT_CefTel_InputToFrameLatencyMaxMs, 0);
	SET_DWORD_STAT(STAT_CefTel_FenceNotReady, 0);
}

void SCefBrowserSurface::SetBrowserSession(TWeakObjectPtr<UCefWebUiBrowserSession> inBrowserSession)
{
	if (BrowserSession == inBrowserSession)
	{
		return;
	}

	ResetResourceState();
	LastFrame = FCefSharedFrame();
	DeferredFrame = FCefSharedFrame();
	bHasFrame = false;
	bHasDeferredFrame = false;
	DeferredRetryCount = 0;
	LastSeenFrameId = 0;
	LastSubmittedFrameId = 0;
	LastSubmittedGeneration = 0;
	bSharedTexturesReopenPending = false;
	BrowserSession = inBrowserSession;
	UnbindFrameReaderDelegate();
	FrameReader.Reset();
	ControlWriter.Reset();
}

void SCefBrowserSurface::SetBrowserSize(int32 inBrowserWidth, int32 inBrowserHeight)
{
	DesiredBrowserWidth = FMath::Max(1, inBrowserWidth);
	DesiredBrowserHeight = FMath::Max(1, inBrowserHeight);
	TargetBrowserWidth = DesiredBrowserWidth;
	TargetBrowserHeight = DesiredBrowserHeight;
	LastSentBrowserWidth = DesiredBrowserWidth;
	LastSentBrowserHeight = DesiredBrowserHeight;
	bAutoResizePending = false;
	bAwaitingAutoResizeApply = false;
	AutoResizeRetryCount = 0;
	bAutoResizeFailureLogged = false;
	LastAutoResizeObservedTimeSec = 0.0;
	LastAutoResizeSentTimeSec = 0.0;
	LastAutoResizeAwaitStartTimeSec = 0.0;
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 SCefBrowserSurface::OnPaint(
	const FPaintArgs& args,
	const FGeometry& allottedGeometry,
	const FSlateRect& myCullingRect,
	FSlateWindowElementList& outDrawElements,
	int32 layerId,
	const FWidgetStyle& inWidgetStyle,
	bool bParentEnabled) const
{
	SCOPE_CYCLE_COUNTER(STAT_CefSlate_Draw);
	++TelemetryPaintCalls;
	PollLatestFrame();
	if (!bHasFrame)
	{
		return layerId;
	}

	const double nowSec = FPlatformTime::Seconds();
	UpdateFrameTelemetry(LastFrame);
	MaybeUpdateCadenceFeedback(nowSec);
	MaybePushHostTuning(nowSec);
	MaybeUpdateCursor(LastFrame, nowSec);
	MaybeLogAndResetTelemetry(nowSec);

	const uint32 slotCount = FMath::Clamp(SharedSlotCount, 1u, MaxSharedSlots);
	const uint32 slot = LastFrame.WriteSlot % slotCount;
	const FTextureRHIRef mainTexture = GetSharedTexture(slot);
	if (!mainTexture.IsValid())
	{
		return layerId;
	}

	if (!CustomDrawer.IsValid())
	{
		CustomDrawer = MakeShared<FCefBrowserSurfaceDrawer, ESPMode::ThreadSafe>();
	}

	const bool usePopupPlane = ((LastFrame.Flags & CefFrameFlag_PopupPlane) != 0);
	if (usePopupPlane)
	{
		const bool bPopupPlaneReady = EnsurePopupPlaneRhi(LastFrame);
		if (!bPopupPlaneReady)
		{
			return layerId;
		}
	}

	const FSlateRect renderRect = allottedGeometry.GetRenderBoundingRect();
	const FIntRect destRect(
		FMath::RoundToInt(renderRect.Left),
		FMath::RoundToInt(renderRect.Top),
		FMath::RoundToInt(renderRect.Right),
		FMath::RoundToInt(renderRect.Bottom));
	const uint64 generation = GetResourceGeneration();
	const bool bNeedsUpdate = LastSubmittedFrameId != LastFrame.FrameId ||
		LastSubmittedGeneration != generation ||
		LastSubmittedDestRect != destRect ||
		LastSubmittedPopupRect != FIntRect(
			LastFrame.PopupRect.X,
			LastFrame.PopupRect.Y,
			LastFrame.PopupRect.X + LastFrame.PopupRect.W,
			LastFrame.PopupRect.Y + LastFrame.PopupRect.H) ||
		bLastSubmittedPopupVisible != LastFrame.bPopupVisible;
	if (bNeedsUpdate)
	{
		CustomDrawer->EnqueueUpdate(
			mainTexture,
			GetSharedPopupTexture(),
			LastFrame,
			destRect,
			generation);
		LastSubmittedFrameId = LastFrame.FrameId;
		LastSubmittedGeneration = generation;
		LastSubmittedDestRect = destRect;
		LastSubmittedPopupRect = FIntRect(
			LastFrame.PopupRect.X,
			LastFrame.PopupRect.Y,
			LastFrame.PopupRect.X + LastFrame.PopupRect.W,
			LastFrame.PopupRect.Y + LastFrame.PopupRect.H);
		bLastSubmittedPopupVisible = LastFrame.bPopupVisible;
	}

	FSlateDrawElement::MakeCustom(outDrawElements, layerId, CustomDrawer);
	return layerId + 1;
}

void SCefBrowserSurface::Tick(const FGeometry& allottedGeometry, const double inCurrentTime, const float inDeltaTime)
{
	SLeafWidget::Tick(allottedGeometry, inCurrentTime, inDeltaTime);
	++TelemetryTickCalls;
	MaybeQueueAutoResize(allottedGeometry, inCurrentTime);
	MaybeSendAutoResize(inCurrentTime);
}

FVector2D SCefBrowserSurface::ComputeDesiredSize(float layoutScaleMultiplier) const
{
	(void)layoutScaleMultiplier;
	return FVector2D(static_cast<float>(DesiredBrowserWidth), static_cast<float>(DesiredBrowserHeight));
}

FReply SCefBrowserSurface::OnMouseMove(const FGeometry& myGeometry, const FPointerEvent& mouseEvent)
{
	MarkInputEvent();
	TSharedPtr<FCefInputWriter> inputWriter;
	if (!TryGetInputWriter(inputWriter))
	{
		return FReply::Unhandled();
	}

	int32 x = 0;
	int32 y = 0;
	GetBrowserCoords(myGeometry, mouseEvent.GetScreenSpacePosition(), x, y);
	inputWriter->WriteMouseMove(x, y);
	return FReply::Handled();
}

FReply SCefBrowserSurface::OnMouseButtonDown(const FGeometry& myGeometry, const FPointerEvent& mouseEvent)
{
	MarkInputEvent();
	TSharedPtr<FCefInputWriter> inputWriter;
	if (!TryGetInputWriter(inputWriter))
	{
		return FReply::Unhandled();
	}

	ECefMouseButton button = ECefMouseButton::Left;
	if (!SlateButtonToCef(mouseEvent.GetEffectingButton(), button))
	{
		return FReply::Unhandled();
	}

	int32 x = 0;
	int32 y = 0;
	GetBrowserCoords(myGeometry, mouseEvent.GetScreenSpacePosition(), x, y);
	inputWriter->WriteMouseButton(x, y, button, false);
	return FReply::Handled().CaptureMouse(AsShared());
}

FReply SCefBrowserSurface::OnMouseButtonUp(const FGeometry& myGeometry, const FPointerEvent& mouseEvent)
{
	MarkInputEvent();
	TSharedPtr<FCefInputWriter> inputWriter;
	if (!TryGetInputWriter(inputWriter))
	{
		return FReply::Unhandled();
	}

	ECefMouseButton button = ECefMouseButton::Left;
	if (!SlateButtonToCef(mouseEvent.GetEffectingButton(), button))
	{
		return FReply::Unhandled();
	}

	int32 x = 0;
	int32 y = 0;
	GetBrowserCoords(myGeometry, mouseEvent.GetScreenSpacePosition(), x, y);
	inputWriter->WriteMouseButton(x, y, button, true);
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SCefBrowserSurface::OnMouseWheel(const FGeometry& myGeometry, const FPointerEvent& mouseEvent)
{
	MarkInputEvent();
	TSharedPtr<FCefInputWriter> inputWriter;
	if (!TryGetInputWriter(inputWriter))
	{
		return FReply::Unhandled();
	}

	int32 x = 0;
	int32 y = 0;
	GetBrowserCoords(myGeometry, mouseEvent.GetScreenSpacePosition(), x, y);
	inputWriter->WriteMouseScroll(x, y, 0.0f, mouseEvent.GetWheelDelta() * 120.0f);
	return FReply::Handled();
}

FReply SCefBrowserSurface::OnKeyDown(const FGeometry& myGeometry, const FKeyEvent& keyEvent)
{
	MarkInputEvent();
	TSharedPtr<FCefInputWriter> inputWriter;
	if (!TryGetInputWriter(inputWriter))
	{
		return FReply::Unhandled();
	}

	const uint32 modifiers = CefWebUi::BrowserSurface::BuildCefKeyModifiers(keyEvent);
	inputWriter->WriteKey(keyEvent.GetKeyCode(), modifiers, true);
	return FReply::Handled();
}

FReply SCefBrowserSurface::OnKeyUp(const FGeometry& myGeometry, const FKeyEvent& keyEvent)
{
	MarkInputEvent();
	TSharedPtr<FCefInputWriter> inputWriter;
	if (!TryGetInputWriter(inputWriter))
	{
		return FReply::Unhandled();
	}

	const uint32 modifiers = CefWebUi::BrowserSurface::BuildCefKeyModifiers(keyEvent);
	inputWriter->WriteKey(keyEvent.GetKeyCode(), modifiers, false);
	return FReply::Handled();
}

FReply SCefBrowserSurface::OnKeyChar(const FGeometry& myGeometry, const FCharacterEvent& characterEvent)
{
	MarkInputEvent();
	TSharedPtr<FCefInputWriter> inputWriter;
	if (!TryGetInputWriter(inputWriter))
	{
		return FReply::Unhandled();
	}

	const FModifierKeysState& mk = characterEvent.GetModifierKeys();
	if (mk.IsControlDown() || mk.IsAltDown() || mk.IsCommandDown())
	{
		return FReply::Handled();
	}

	inputWriter->WriteChar(static_cast<uint16>(characterEvent.GetCharacter()));
	return FReply::Handled();
}

void SCefBrowserSurface::HandleFrameReady()
{
	Invalidate(EInvalidateWidgetReason::Paint);
}

EActiveTimerReturnType SCefBrowserSurface::HandleActiveTimer(double currentTime, float deltaTime)
{
	(void)currentTime;
	(void)deltaTime;
	++TelemetryTimerCalls;
	if (!HasPendingResourceWork())
	{
		bActiveTimerRegistered = false;
		return EActiveTimerReturnType::Stop;
	}

	Invalidate(EInvalidateWidgetReason::Paint);
	return EActiveTimerReturnType::Continue;
}

void SCefBrowserSurface::StartActiveTimer() const
{
	if (bActiveTimerRegistered)
	{
		return;
	}

	bActiveTimerRegistered = true;
	const_cast<SCefBrowserSurface*>(this)->RegisterActiveTimer(
		CefWebUi::BrowserSurface::ActiveRepaintPeriodSec,
		FWidgetActiveTimerDelegate::CreateSP(const_cast<SCefBrowserSurface*>(this), &SCefBrowserSurface::HandleActiveTimer));
}

void SCefBrowserSurface::UnbindFrameReaderDelegate()
{
	if (!FrameReadyDelegateHandle.IsValid())
	{
		return;
	}

	if (TSharedPtr<FCefFrameReader> reader = FrameReader.Pin())
	{
		reader->OnFrameReady.Remove(FrameReadyDelegateHandle);
	}
	FrameReadyDelegateHandle.Reset();
}

bool SCefBrowserSurface::TryGetFrameReader(TSharedPtr<FCefFrameReader>& outFrameReader) const
{
	if (TSharedPtr<FCefFrameReader> reader = FrameReader.Pin())
	{
		outFrameReader = reader;
		return true;
	}

	const TWeakObjectPtr<UCefWebUiBrowserSession> session = BrowserSession;
	if (!session.IsValid())
	{
		return false;
	}

	FrameReader = session->GetFrameReaderPtr();
	outFrameReader = FrameReader.Pin();
	if (outFrameReader.IsValid() && !FrameReadyDelegateHandle.IsValid())
	{
		FrameReadyDelegateHandle = outFrameReader->OnFrameReady.AddSP(const_cast<SCefBrowserSurface*>(this), &SCefBrowserSurface::HandleFrameReady);
	}
	return outFrameReader.IsValid();
}

bool SCefBrowserSurface::TryGetInputWriter(TSharedPtr<FCefInputWriter>& outInputWriter) const
{
	const TWeakObjectPtr<UCefWebUiBrowserSession> session = BrowserSession;
	if (!session.IsValid())
	{
		return false;
	}

	outInputWriter = session->GetInputWriterPtr().Pin();
	return outInputWriter.IsValid();
}

bool SCefBrowserSurface::TryGetControlWriter(TSharedPtr<FCefControlWriter>& outControlWriter) const
{
	if (TSharedPtr<FCefControlWriter> writer = ControlWriter.Pin())
	{
		outControlWriter = writer;
		return true;
	}

	const TWeakObjectPtr<UCefWebUiBrowserSession> session = BrowserSession;
	if (!session.IsValid())
	{
		return false;
	}

	ControlWriter = session->GetControlWriterPtr();
	outControlWriter = ControlWriter.Pin();
	return outControlWriter.IsValid();
}

void SCefBrowserSurface::MaybePushHostTuning(double nowSec) const
{
	TSharedPtr<FCefControlWriter> controlWriter;
	if (!TryGetControlWriter(controlWriter))
	{
		return;
	}
	if (!controlWriter->IsOpen())
	{
		controlWriter->Open();
	}
	if (!controlWriter->IsOpen())
	{
		return;
	}

	if (LastHostTuningPushTimeSec > 0.0 && (nowSec - LastHostTuningPushTimeSec) < CefWebUi::BrowserSurface::HostTuningPushPeriodSec)
	{
		return;
	}

	controlWriter->SetFrameRate(CefWebUi::BrowserSurface::HostFrameRate);
	controlWriter->SetMaxInFlightBeginFrames(CefWebUi::BrowserSurface::HostMaxInFlightBeginFrames);
	controlWriter->SetFlushIntervalFrames(CefWebUi::BrowserSurface::HostFlushIntervalFrames);
	controlWriter->SetKeyframeIntervalUs(CefWebUi::BrowserSurface::HostKeyframeIntervalUs);
	LastHostTuningPushTimeSec = nowSec;
}

void SCefBrowserSurface::MaybeUpdateCadenceFeedback(double nowSec) const
{
	if (!CefWebUi::BrowserSurface::EnableCadenceFeedback)
	{
		return;
	}

	TSharedPtr<FCefControlWriter> controlWriter;
	if (!TryGetControlWriter(controlWriter))
	{
		return;
	}
	if (!controlWriter->IsOpen())
	{
		controlWriter->Open();
	}
	if (!controlWriter->IsOpen())
	{
		return;
	}

	if (LastConsumerFrameTimeSec > 0.0)
	{
		const double dtSec = nowSec - LastConsumerFrameTimeSec;
		if (dtSec > 0.0001)
		{
			const uint32 dtUs = static_cast<uint32>(FMath::Clamp(dtSec * 1000000.0, 1000.0, 1000000.0));
			SmoothedCadenceUs = (SmoothedCadenceUs == 0) ? dtUs : static_cast<uint32>((SmoothedCadenceUs * 7ull + dtUs) / 8ull);
		}
	}
	LastConsumerFrameTimeSec = nowSec;

	if (LastCadenceSentTimeSec > 0.0 && (nowSec - LastCadenceSentTimeSec) < 0.10)
	{
		return;
	}

	const uint32 cadenceToSend = (SmoothedCadenceUs > 0) ? SmoothedCadenceUs : 16666u;
	controlWriter->SetConsumerCadenceUs(cadenceToSend);
	LastCadenceSentTimeSec = nowSec;
}

void SCefBrowserSurface::UpdateFrameTelemetry(const FCefSharedFrame& frame) const
{
	++TelemetryConsumedFrames;
	SET_DWORD_STAT(STAT_CefTel_ConsumedFrames, TelemetryConsumedFrames);

	if (LastSeenFrameId != 0 && frame.FrameId > (LastSeenFrameId + 1))
	{
		const uint64 gap = frame.FrameId - (LastSeenFrameId + 1);
		const uint32 remaining = (TelemetryFrameGapCount < MAX_uint32) ? (MAX_uint32 - TelemetryFrameGapCount) : 0;
		TelemetryFrameGapCount += static_cast<uint32>(FMath::Min<uint64>(gap, remaining));
		SET_DWORD_STAT(STAT_CefTel_FrameGaps, TelemetryFrameGapCount);
	}
	LastSeenFrameId = frame.FrameId;
}

void SCefBrowserSurface::MaybeUpdateCursor(const FCefSharedFrame& frame, double nowSec) const
{
	if (frame.CursorType == LastCursorType)
	{
		return;
	}

	const double hz = CefWebUi::BrowserSurface::CursorUpdateRateHz;
	const double minDt = (hz > 0.0) ? (1.0 / hz) : 0.0;
	if (minDt > 0.0 && (nowSec - LastCursorUpdateTimeSec) < minDt)
	{
		return;
	}

	LastCursorType = frame.CursorType;
	LastCursorUpdateTimeSec = nowSec;
	const_cast<SCefBrowserSurface*>(this)->SetCursor(FCefFrameReader::MapCefCursor(frame.CursorType));
}

void SCefBrowserSurface::MaybeLogAndResetTelemetry(double nowSec) const
{
	const double logPeriodSec = CefWebUi::BrowserSurface::TelemetryLogPeriodSec;
	if (logPeriodSec <= 0.0)
	{
		return;
	}
	if (LastTelemetryLogTimeSec <= 0.0)
	{
		LastTelemetryLogTimeSec = nowSec;
		return;
	}
	if (nowSec - LastTelemetryLogTimeSec < logPeriodSec)
	{
		return;
	}

	const uint32 avgInputLatencyMs = (TelemetryInputLatencySamples > 0) ? (TelemetryInputLatencyMsSum / TelemetryInputLatencySamples) : 0;
	UE_LOG(LogCefWebUiTelemetry, Log,
		TEXT("[CefSlateTelemetry] frames=%u gaps=%u fence_not_ready=%u input_latency_avg_ms=%u input_latency_max_ms=%u paint_calls=%u tick_calls=%u timer_calls=%u poll_success=%u last_latency_frame_id=%llu"),
		TelemetryConsumedFrames,
		TelemetryFrameGapCount,
		TelemetryFenceNotReadyCount,
		avgInputLatencyMs,
		TelemetryInputLatencyMsMax,
		TelemetryPaintCalls,
		TelemetryTickCalls,
		TelemetryTimerCalls,
		TelemetryPollSuccess,
		static_cast<unsigned long long>(LastLatencyFrameId));

	LastTelemetryLogTimeSec = nowSec;
	TelemetryConsumedFrames = 0;
	TelemetryFrameGapCount = 0;
	TelemetryInputLatencySamples = 0;
	TelemetryInputLatencyMsSum = 0;
	TelemetryInputLatencyMsMax = 0;
	TelemetryPaintCalls = 0;
	TelemetryTickCalls = 0;
	TelemetryTimerCalls = 0;
	TelemetryPollSuccess = 0;
	TelemetryFenceNotReadyCount = 0;
	SET_DWORD_STAT(STAT_CefTel_ConsumedFrames, 0);
	SET_DWORD_STAT(STAT_CefTel_FrameGaps, 0);
	SET_DWORD_STAT(STAT_CefTel_InputToFrameLatencyMs, static_cast<uint32>(LastInputToFrameLatencyMs));
	SET_DWORD_STAT(STAT_CefTel_InputToFrameLatencyMaxMs, 0);
	SET_DWORD_STAT(STAT_CefTel_FenceNotReady, 0);
}

void SCefBrowserSurface::MarkInputEvent()
{
	LastInputEventTimeSec = FPlatformTime::Seconds();
	++InputEventSerial;
}

void SCefBrowserSurface::PollLatestFrame() const
{
	SCOPE_CYCLE_COUNTER(STAT_CefSlate_PollFrame);
	TSharedPtr<FCefFrameReader> frameReader;
	if (!TryGetFrameReader(frameReader))
	{
		return;
	}

	FCefSharedFrame frame;
	if (bHasDeferredFrame)
	{
		const bool ready = IsFrameGpuReady(DeferredFrame);
		if (!ready)
		{
			DeferredRetryCount = FMath::Min(DeferredRetryCount + 1u, MAX_uint32);
			++TelemetryFenceNotReadyCount;
			SET_DWORD_STAT(STAT_CefTel_FenceNotReady, TelemetryFenceNotReadyCount);
			return;
		}
		LastFrame = DeferredFrame;
		bHasFrame = true;
		bHasDeferredFrame = false;
		DeferredRetryCount = 0;
	}
	else if (frameReader->PollSharedTexture(frame))
	{
		if (!CefWebUi::BrowserSurface::SanitizeSharedFrameMetadata(frame))
		{
			UE_LOGFMT(LogCefWebUiTelemetry, Warning,
				"[CefSlate] rejected invalid frame metadata frame_id={0}", frame.FrameId);
			return;
		}
		++TelemetryPollSuccess;
		if (!IsFrameGpuReady(frame))
		{
			DeferredFrame = frame;
			bHasDeferredFrame = true;
			DeferredRetryCount = 0;
			++TelemetryFenceNotReadyCount;
			SET_DWORD_STAT(STAT_CefTel_FenceNotReady, TelemetryFenceNotReadyCount);
			return;
		}

		if (LastSeenFrameId != 0 && frame.FrameId <= LastSeenFrameId)
		{
			const bool bResizeReset = ((frame.Flags & CefFrameFlag_Resized) != 0);
			if (!bResizeReset)
			{
				return;
			}

			UE_LOG(LogCefWebUiTelemetry, Warning,
				TEXT("[CefAutoResize] frame id reset accepted last=%llu new=%llu"),
				static_cast<unsigned long long>(LastSeenFrameId),
				static_cast<unsigned long long>(frame.FrameId));
			LastSeenFrameId = 0;
		}

		LastFrame = frame;
		bHasFrame = true;
	}

	if (bHasFrame && InputEventSerial != 0 && InputEventSerial != LastLatencyMeasuredInputSerial)
	{
		const double nowSec = FPlatformTime::Seconds();
		const uint32 latencyMs = static_cast<uint32>(FMath::Clamp((nowSec - LastInputEventTimeSec) * 1000.0, 0.0, 10000.0));
		LastInputToFrameLatencyMs = static_cast<double>(latencyMs);
		LastLatencyMeasuredInputSerial = InputEventSerial;
		LastLatencyFrameId = LastFrame.FrameId;
		++TelemetryInputLatencySamples;
		const uint32 remaining = (TelemetryInputLatencyMsSum < MAX_uint32) ? (MAX_uint32 - TelemetryInputLatencyMsSum) : 0;
		TelemetryInputLatencyMsSum += FMath::Min(latencyMs, remaining);
		TelemetryInputLatencyMsMax = FMath::Max(TelemetryInputLatencyMsMax, latencyMs);
		SET_DWORD_STAT(STAT_CefTel_InputToFrameLatencyMs, latencyMs);
		SET_DWORD_STAT(STAT_CefTel_InputToFrameLatencyMaxMs, TelemetryInputLatencyMsMax);
	}

	if (!bHasFrame)
	{
		return;
	}

	HandleAppliedFrameSize(static_cast<int32>(LastFrame.Width), static_cast<int32>(LastFrame.Height));
	if (bSharedTexturesReopenPending)
	{
		ResetResourceState();
		bSharedTexturesReopenPending = false;
		UE_LOG(LogCefWebUiTelemetry, Log,
			TEXT("[CefAutoResize] shared textures reopen requested"));
	}

	SharedSlotCount = FMath::Clamp(LastFrame.SlotCount, 1u, MaxSharedSlots);
	EnsureSharedRhi(LastFrame);
	if ((LastFrame.Flags & CefFrameFlag_PopupPlane) != 0)
	{
		const bool bPopupPlaneReady = EnsurePopupPlaneRhi(LastFrame);
		if (!bPopupPlaneReady)
		{
			return;
		}
	}
}

bool SCefBrowserSurface::EnsureSharedGpuFence() const
{
	if (!CefWebUi::BrowserSurface::UseGpuFenceCheck || !ResourceState.IsValid())
	{
		return !CefWebUi::BrowserSurface::UseGpuFenceCheck;
	}

	const TSharedRef<FCefBrowserSurfaceResourceState, ESPMode::ThreadSafe> state = ResourceState.ToSharedRef();
	uint64 generation = 0;
	FString fenceName;
	{
		FScopeLock lock(&state->Lock);
		if (state->SharedGpuFence.IsValid())
		{
			return true;
		}
		if (state->bFenceOpenPending || state->OpenRetryCount >= CefWebUi::BrowserSurface::ResourceOpenMaxRetries)
		{
			return false;
		}
		state->bFenceOpenPending = true;
		generation = state->Generation;
		fenceName = GetSurfaceSharedNames(BrowserSession).SharedGpuFenceName;
	}

	StartActiveTimer();
	ENQUEUE_RENDER_COMMAND(CefSlateOpenSharedFence)(
		[state, fenceName, generation](FRHICommandListImmediate&) mutable {
			TRefCountPtr<ID3D12Fence> importedFence;
			if (ID3D12DynamicRHI* d3d12Rhi = GetID3D12DynamicRHI())
			{
				if (ID3D12Device* d3dDevice = d3d12Rhi->RHIGetDevice(0))
				{
					HANDLE sharedHandle = nullptr;
					if (SUCCEEDED(d3dDevice->OpenSharedHandleByName(*fenceName, GENERIC_ALL, &sharedHandle)) && sharedHandle)
					{
						ID3D12Fence* fence = nullptr;
						if (SUCCEEDED(d3dDevice->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&fence))) && fence)
						{
							importedFence = fence;
							fence->Release();
						}
						CloseHandle(sharedHandle);
					}
				}
			}

			FScopeLock lock(&state->Lock);
			if (state->Generation != generation)
			{
				return;
			}
			state->bFenceOpenPending = false;
			if (importedFence.IsValid())
			{
				state->SharedGpuFence = importedFence;
				state->OpenRetryCount = 0;
				state->bRetryPending = false;
			}
			else
			{
				++state->OpenRetryCount;
				state->bRetryPending = state->OpenRetryCount < CefWebUi::BrowserSurface::ResourceOpenMaxRetries;
			}
		});
	return false;
}

bool SCefBrowserSurface::IsFrameGpuReady(const FCefSharedFrame& frame) const
{
	if (!CefWebUi::BrowserSurface::UseGpuFenceCheck || frame.GpuFenceValue == 0)
	{
		return true;
	}
	if (!EnsureSharedGpuFence() || !ResourceState.IsValid())
	{
		return false;
	}

	TRefCountPtr<ID3D12Fence> fence;
	{
		FScopeLock lock(&ResourceState->Lock);
		fence = ResourceState->SharedGpuFence;
	}
	return fence.IsValid() && fence->GetCompletedValue() >= frame.GpuFenceValue;
}

void SCefBrowserSurface::EnsureSharedRhi(const FCefSharedFrame& InFrame) const
{
	if (!ResourceState.IsValid())
	{
		return;
	}

	const TSharedRef<FCefBrowserSurfaceResourceState, ESPMode::ThreadSafe> state = ResourceState.ToSharedRef();
	const uint32 slotCount = FMath::Clamp(InFrame.SlotCount, 1u, MaxSharedSlots);
	uint64 generation = 0;
	FString texturePrefix;
	{
		FScopeLock lock(&state->Lock);
		if (state->LastFrameId != InFrame.FrameId)
		{
			state->LastFrameId = InFrame.FrameId;
			state->OpenRetryCount = 0;
			state->bRetryPending = false;
		}
		bool bReady = true;
		for (uint32 Index = 0; Index < slotCount; ++Index)
		{
			bReady &= state->MainTextures[Index].IsValid();
		}
		if (bReady || state->bMainOpenPending || state->OpenRetryCount >= CefWebUi::BrowserSurface::ResourceOpenMaxRetries)
		{
			return;
		}
		state->bMainOpenPending = true;
		generation = state->Generation;
		texturePrefix = GetSurfaceSharedNames(BrowserSession).SharedTextureNamePrefix;
	}

	StartActiveTimer();
	ENQUEUE_RENDER_COMMAND(CefSlateOpenSharedTextures)(
		[state, texturePrefix, generation, slotCount](FRHICommandListImmediate&) mutable {
			FTextureRHIRef importedTextures[MaxSharedSlots];
			if (ID3D12DynamicRHI* d3d12Rhi = GetID3D12DynamicRHI())
			{
				if (ID3D12Device* d3dDevice = d3d12Rhi->RHIGetDevice(0))
				{
					for (uint32 Index = 0; Index < slotCount; ++Index)
					{
						const FString textureName = FString::Printf(TEXT("%s_%u"), *texturePrefix, Index);
						HANDLE sharedHandle = nullptr;
						if (FAILED(d3dDevice->OpenSharedHandleByName(*textureName, GENERIC_ALL, &sharedHandle)) || !sharedHandle)
						{
							break;
						}

						ID3D12Resource* d3dResource = nullptr;
						const HRESULT hr = d3dDevice->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&d3dResource));
						CloseHandle(sharedHandle);
						if (FAILED(hr) || !d3dResource)
						{
							break;
						}

						const D3D12_RESOURCE_DESC desc = d3dResource->GetDesc();
						if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || desc.MipLevels == 0)
						{
							d3dResource->Release();
							break;
						}

						importedTextures[Index] = d3d12Rhi->RHICreateTexture2DFromResource(
							PF_B8G8R8A8,
							ETextureCreateFlags::External | ETextureCreateFlags::ShaderResource,
							FClearValueBinding::None,
							d3dResource);
						d3dResource->Release();
					}
				}
			}

			FScopeLock lock(&state->Lock);
			if (state->Generation != generation)
			{
				return;
			}
			state->bMainOpenPending = false;
			for (uint32 Index = 0; Index < slotCount; ++Index)
			{
				if (importedTextures[Index].IsValid())
				{
					state->MainTextures[Index] = importedTextures[Index];
				}
			}
			bool bReady = true;
			for (uint32 Index = 0; Index < slotCount; ++Index)
			{
				bReady &= state->MainTextures[Index].IsValid();
			}
			if (bReady)
			{
				state->OpenRetryCount = 0;
				state->bRetryPending = false;
			}
			else
			{
				++state->OpenRetryCount;
				state->bRetryPending = state->OpenRetryCount < CefWebUi::BrowserSurface::ResourceOpenMaxRetries;
			}
		});
}

bool SCefBrowserSurface::EnsurePopupPlaneRhi(const FCefSharedFrame& InFrame) const
{
	(void)InFrame;
	if (!ResourceState.IsValid())
	{
		return false;
	}

	const TSharedRef<FCefBrowserSurfaceResourceState, ESPMode::ThreadSafe> state = ResourceState.ToSharedRef();
	uint64 generation = 0;
	FString popupTextureName;
	{
		FScopeLock lock(&state->Lock);
		if (state->PopupTexture.IsValid())
		{
			return true;
		}
		if (state->bPopupOpenPending || state->OpenRetryCount >= CefWebUi::BrowserSurface::ResourceOpenMaxRetries)
		{
			return false;
		}
		state->bPopupOpenPending = true;
		generation = state->Generation;
		popupTextureName = GetSurfaceSharedNames(BrowserSession).SharedPopupTextureName;
	}

	StartActiveTimer();
	ENQUEUE_RENDER_COMMAND(CefSlateOpenPopupTexture)(
		[state, popupTextureName, generation](FRHICommandListImmediate&) mutable {
			FTextureRHIRef importedTexture;
			if (ID3D12DynamicRHI* d3d12Rhi = GetID3D12DynamicRHI())
			{
				if (ID3D12Device* d3dDevice = d3d12Rhi->RHIGetDevice(0))
				{
					HANDLE sharedHandle = nullptr;
					if (SUCCEEDED(d3dDevice->OpenSharedHandleByName(*popupTextureName, GENERIC_ALL, &sharedHandle)) && sharedHandle)
					{
						ID3D12Resource* d3dResource = nullptr;
						const HRESULT hr = d3dDevice->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&d3dResource));
						CloseHandle(sharedHandle);
						if (SUCCEEDED(hr) && d3dResource)
						{
							const D3D12_RESOURCE_DESC desc = d3dResource->GetDesc();
							if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D && desc.MipLevels > 0)
							{
								importedTexture = d3d12Rhi->RHICreateTexture2DFromResource(
									PF_B8G8R8A8,
									ETextureCreateFlags::External | ETextureCreateFlags::ShaderResource,
									FClearValueBinding::None,
									d3dResource);
							}
							d3dResource->Release();
						}
					}
				}
			}

			FScopeLock lock(&state->Lock);
			if (state->Generation != generation)
			{
				return;
			}
			state->bPopupOpenPending = false;
			if (importedTexture.IsValid())
			{
				state->PopupTexture = importedTexture;
				state->OpenRetryCount = 0;
				state->bRetryPending = false;
			}
			else
			{
				++state->OpenRetryCount;
				state->bRetryPending = state->OpenRetryCount < CefWebUi::BrowserSurface::ResourceOpenMaxRetries;
			}
		});
	return false;
}

bool SCefBrowserSurface::HasPendingResourceWork() const
{
	if (bHasDeferredFrame || !ResourceState.IsValid())
	{
		return bHasDeferredFrame;
	}

	FScopeLock lock(&ResourceState->Lock);
	return ResourceState->bMainOpenPending || ResourceState->bPopupOpenPending || ResourceState->bFenceOpenPending ||
		(ResourceState->bRetryPending && ResourceState->OpenRetryCount < CefWebUi::BrowserSurface::ResourceOpenMaxRetries);
}

FTextureRHIRef SCefBrowserSurface::GetSharedTexture(uint32 InSlot) const
{
	if (!ResourceState.IsValid() || InSlot >= MaxSharedSlots)
	{
		return nullptr;
	}

	FScopeLock lock(&ResourceState->Lock);
	return ResourceState->MainTextures[InSlot];
}

FTextureRHIRef SCefBrowserSurface::GetSharedPopupTexture() const
{
	if (!ResourceState.IsValid())
	{
		return nullptr;
	}

	FScopeLock lock(&ResourceState->Lock);
	return ResourceState->PopupTexture;
}

uint64 SCefBrowserSurface::GetResourceGeneration() const
{
	if (!ResourceState.IsValid())
	{
		return 0;
	}

	FScopeLock lock(&ResourceState->Lock);
	return ResourceState->Generation;
}

void SCefBrowserSurface::ResetResourceState() const
{
	if (!ResourceState.IsValid())
	{
		return;
	}

	FTextureRHIRef oldMainTextures[MaxSharedSlots];
	FTextureRHIRef oldPopupTexture;
	TRefCountPtr<ID3D12Fence> oldFence;
	{
		FScopeLock lock(&ResourceState->Lock);
		ResourceState->Generation = CefWebUi::BrowserSurface::AdvanceResourceGeneration(ResourceState->Generation);
		ResourceState->LastFrameId = 0;
		ResourceState->OpenRetryCount = 0;
		ResourceState->bMainOpenPending = false;
		ResourceState->bPopupOpenPending = false;
		ResourceState->bFenceOpenPending = false;
		ResourceState->bRetryPending = false;
		for (uint32 Index = 0; Index < MaxSharedSlots; ++Index)
		{
			oldMainTextures[Index] = ResourceState->MainTextures[Index];
			ResourceState->MainTextures[Index] = nullptr;
		}
		oldPopupTexture = ResourceState->PopupTexture;
		ResourceState->PopupTexture = nullptr;
		oldFence = ResourceState->SharedGpuFence;
		ResourceState->SharedGpuFence = nullptr;
	}

	ENQUEUE_RENDER_COMMAND(CefSlateReleaseSharedResources)(
		[oldMain0 = oldMainTextures[0], oldMain1 = oldMainTextures[1], oldMain2 = oldMainTextures[2], oldPopupTexture, oldFence](FRHICommandListImmediate&) mutable {
			oldMain0.SafeRelease();
			oldMain1.SafeRelease();
			oldMain2.SafeRelease();
			oldPopupTexture.SafeRelease();
			oldFence.SafeRelease();
		});

	LastSubmittedFrameId = 0;
	LastSubmittedGeneration = 0;
}

void SCefBrowserSurface::ReleaseResources()
{
	UnbindFrameReaderDelegate();

	TSharedPtr<FCefBrowserSurfaceDrawer, ESPMode::ThreadSafe> drawer = CustomDrawer;
	if (drawer.IsValid())
	{
		ENQUEUE_RENDER_COMMAND(CefSlateReleaseDrawer)(
			[drawer](FRHICommandListImmediate&) mutable {
				drawer.Reset();
			});
		CustomDrawer.Reset();
	}

	ResetResourceState();
	bHasFrame = false;
	bHasDeferredFrame = false;
	DeferredRetryCount = 0;
	FrameReader.Reset();
	ControlWriter.Reset();
	ResourceState.Reset();
}

void SCefBrowserSurface::MaybeQueueAutoResize(const FGeometry& inAllottedGeometry, double inNowSec)
{
	if (!CefWebUi::BrowserSurface::EnableAutoResizeToWidget)
	{
		return;
	}

	const FSlateRect renderRect = inAllottedGeometry.GetRenderBoundingRect();
	const FIntPoint physicalSize = CefWebUi::BrowserSurface::ComputePhysicalTargetSize(
		FVector2D(renderRect.Right - renderRect.Left, renderRect.Bottom - renderRect.Top));
	const int32 rawWidth = physicalSize.X;
	const int32 rawHeight = physicalSize.Y;
	if (rawWidth < CefWebUi::BrowserSurface::AutoResizeMinDimensionPx ||
		rawHeight < CefWebUi::BrowserSurface::AutoResizeMinDimensionPx)
	{
		return;
	}

	const int32 targetWidth = FMath::Max(1, rawWidth);
	const int32 targetHeight = FMath::Max(1, rawHeight);
	const int32 minDeltaPx = FMath::Max(1, CefWebUi::BrowserSurface::AutoResizeMinDeltaPx);
	const bool bMeaningfulDelta =
		FMath::Abs(targetWidth - TargetBrowserWidth) >= minDeltaPx ||
		FMath::Abs(targetHeight - TargetBrowserHeight) >= minDeltaPx;
	if (!bMeaningfulDelta)
	{
		return;
	}

	TargetBrowserWidth = targetWidth;
	TargetBrowserHeight = targetHeight;
	LastAutoResizeObservedTimeSec = inNowSec;
	bAutoResizePending = true;
	bAwaitingAutoResizeApply = false;
	AutoResizeRetryCount = 0;
	bAutoResizeFailureLogged = false;
	UE_LOG(LogCefWebUiTelemetry, Log,
		TEXT("[CefAutoResize] detect target=%dx%d"),
		TargetBrowserWidth, TargetBrowserHeight);
}

void SCefBrowserSurface::MaybeSendAutoResize(double inNowSec)
{
	if (!bAutoResizePending && !bAwaitingAutoResizeApply)
	{
		return;
	}

	TSharedPtr<FCefControlWriter> controlWriter;
	if (!TryGetControlWriter(controlWriter))
	{
		return;
	}
	if (!controlWriter->IsOpen())
	{
		controlWriter->Open();
	}
	if (!controlWriter->IsOpen())
	{
		return;
	}

	if (bAutoResizePending)
	{
		const bool bThrottleElapsed =
			(LastAutoResizeSentTimeSec <= 0.0) ||
			((inNowSec - LastAutoResizeSentTimeSec) >= CefWebUi::BrowserSurface::AutoResizeThrottlePeriodSec);
		const bool bDebounceElapsed =
			(LastAutoResizeObservedTimeSec <= 0.0) ||
			((inNowSec - LastAutoResizeObservedTimeSec) >= CefWebUi::BrowserSurface::AutoResizeDebouncePeriodSec);
		if (!bThrottleElapsed || !bDebounceElapsed)
		{
			return;
		}

		controlWriter->Resize(
			static_cast<uint32>(FMath::Max(1, TargetBrowserWidth)),
			static_cast<uint32>(FMath::Max(1, TargetBrowserHeight)));
		LastSentBrowserWidth = TargetBrowserWidth;
		LastSentBrowserHeight = TargetBrowserHeight;
		LastAutoResizeSentTimeSec = inNowSec;
		LastAutoResizeAwaitStartTimeSec = inNowSec;
		bAutoResizePending = false;
		bAwaitingAutoResizeApply = true;
		UE_LOG(LogCefWebUiTelemetry, Log,
			TEXT("[CefAutoResize] send target=%dx%d"),
			LastSentBrowserWidth, LastSentBrowserHeight);
		return;
	}

	if (!bAwaitingAutoResizeApply)
	{
		return;
	}
	if (LastAutoResizeAwaitStartTimeSec <= 0.0)
	{
		LastAutoResizeAwaitStartTimeSec = inNowSec;
		return;
	}

	const double elapsedSec = inNowSec - LastAutoResizeAwaitStartTimeSec;
	if (elapsedSec < CefWebUi::BrowserSurface::AutoResizeApplyTimeoutSec)
	{
		return;
	}
	if (AutoResizeRetryCount >= CefWebUi::BrowserSurface::AutoResizeMaxRetries)
	{
		if (!bAutoResizeFailureLogged)
		{
			bAutoResizeFailureLogged = true;
			UE_LOG(LogCefWebUiTelemetry, Warning,
				TEXT("[CefAutoResize] apply timeout target=%dx%d retries=%d"),
				LastSentBrowserWidth, LastSentBrowserHeight, AutoResizeRetryCount);
		}
		return;
	}

	controlWriter->Resize(
		static_cast<uint32>(FMath::Max(1, LastSentBrowserWidth)),
		static_cast<uint32>(FMath::Max(1, LastSentBrowserHeight)));
	LastAutoResizeSentTimeSec = inNowSec;
	LastAutoResizeAwaitStartTimeSec = inNowSec;
	++AutoResizeRetryCount;
	UE_LOG(LogCefWebUiTelemetry, Warning,
		TEXT("[CefAutoResize] retry target=%dx%d retry=%d"),
		LastSentBrowserWidth, LastSentBrowserHeight, AutoResizeRetryCount);
}

void SCefBrowserSurface::HandleAppliedFrameSize(int32 inFrameWidth, int32 inFrameHeight) const
{
	if (inFrameWidth < CefWebUi::BrowserSurface::AutoResizeMinDimensionPx ||
		inFrameHeight < CefWebUi::BrowserSurface::AutoResizeMinDimensionPx)
	{
		return;
	}

	const bool bSizeChanged =
		(AppliedBrowserWidth != inFrameWidth) ||
		(AppliedBrowserHeight != inFrameHeight);
	AppliedBrowserWidth = inFrameWidth;
	AppliedBrowserHeight = inFrameHeight;
	if (bSizeChanged)
	{
		bSharedTexturesReopenPending = true;
	}
	if (bAwaitingAutoResizeApply &&
		AppliedBrowserWidth == LastSentBrowserWidth &&
		AppliedBrowserHeight == LastSentBrowserHeight)
	{
		bAwaitingAutoResizeApply = false;
		AutoResizeRetryCount = 0;
		bAutoResizeFailureLogged = false;
		LastAutoResizeAwaitStartTimeSec = 0.0;
		UE_LOG(LogCefWebUiTelemetry, Log,
			TEXT("[CefAutoResize] applied=%dx%d"),
			AppliedBrowserWidth, AppliedBrowserHeight);
	}
}

void SCefBrowserSurface::GetBrowserCoords(const FGeometry& inGeometry, const FVector2D& inScreenPosition, int32& outX, int32& outY) const
{
	const int32 browserWidth = FMath::Max(1, AppliedBrowserWidth);
	const int32 browserHeight = FMath::Max(1, AppliedBrowserHeight);

	const FVector2D absolutePos = inGeometry.GetAbsolutePosition();
	const FVector2D absoluteSize = inGeometry.GetAbsoluteSize();
	const float width = FMath::Max(absoluteSize.X, 1.0f);
	const float height = FMath::Max(absoluteSize.Y, 1.0f);

	float normalizedX = static_cast<float>((inScreenPosition.X - absolutePos.X) / width);
	float normalizedY = static_cast<float>((inScreenPosition.Y - absolutePos.Y) / height);
	normalizedX = FMath::Clamp(normalizedX, 0.0f, 1.0f);
	normalizedY = FMath::Clamp(normalizedY, 0.0f, 1.0f);

	outX = FMath::Clamp(FMath::RoundToInt(normalizedX * static_cast<float>(browserWidth - 1)), 0, browserWidth - 1);
	outY = FMath::Clamp(FMath::RoundToInt(normalizedY * static_cast<float>(browserHeight - 1)), 0, browserHeight - 1);
}

bool SCefBrowserSurface::SlateButtonToCef(const FKey& InKey, ECefMouseButton& OutButton)
{
	if (InKey == EKeys::LeftMouseButton)
	{
		OutButton = ECefMouseButton::Left;
		return true;
	}
	if (InKey == EKeys::MiddleMouseButton)
	{
		OutButton = ECefMouseButton::Middle;
		return true;
	}
	if (InKey == EKeys::RightMouseButton)
	{
		OutButton = ECefMouseButton::Right;
		return true;
	}
	return false;
}
