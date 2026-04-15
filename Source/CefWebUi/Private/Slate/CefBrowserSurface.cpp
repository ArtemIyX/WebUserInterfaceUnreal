#include "Slate/CefBrowserSurface.h"

#include "CefWebUi.h"
#include "GlobalShader.h"
#include "ID3D12DynamicRHI.h"
#include "InputCoreTypes.h"
#include "PixelShaderUtils.h"
#include "RenderGraphUtils.h"
#include "RenderingThread.h"
#include "RHI.h"
#include "Sessions/CefWebUiBrowserSession.h"
#include "Services/CefControlWriter.h"
#include "Services/CefInputWriter.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <d3d12.h>
#include "Windows/HideWindowsPlatformTypes.h"

DECLARE_STATS_GROUP(TEXT("CefWebUiTelemetry"), STATGROUP_CefWebUiTelemetry, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("CefSlate: PollFrame"), STAT_CefSlate_PollFrame, STATGROUP_CefWebUiTelemetry);
DECLARE_CYCLE_STAT(TEXT("CefSlate: Draw"), STAT_CefSlate_Draw, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Consumed Frames"), STAT_CefTel_ConsumedFrames, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Frame Gaps"), STAT_CefTel_FrameGaps, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Input->Frame Latency Ms"), STAT_CefTel_InputToFrameLatencyMs, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Input->Frame Max Ms"), STAT_CefTel_InputToFrameLatencyMaxMs, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Fence Not Ready"), STAT_CefTel_FenceNotReady, STATGROUP_CefWebUiTelemetry);

namespace
{
constexpr bool   kEnableCadenceFeedback = false;
constexpr double kTelemetryLogPeriodSec = 2.0;
constexpr double kHostTuningPushPeriodSec = 0.5;
constexpr uint32 kHostMaxInFlightBeginFrames = 1;
constexpr uint32 kHostFlushIntervalFrames = 2;
constexpr uint32 kHostKeyframeIntervalUs = 150000;
constexpr uint32 kHostFrameRate = 60;
constexpr double kCursorUpdateRateHz = 120.0;
constexpr bool   kUseGpuFenceCheck = true;
}

class FCefSlateBlitPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCefSlateBlitPS);
	SHADER_USE_PARAMETER_STRUCT(FCefSlateBlitPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER(FVector2f, DestMin)
		SHADER_PARAMETER(FVector2f, DestSize)
		SHADER_PARAMETER(FVector2f, SrcMin)
		SHADER_PARAMETER(FVector2f, SrcSize)
		SHADER_PARAMETER(FVector2f, SrcTexSize)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(FCefSlateBlitPS, "/Plugin/CefWebUi/Private/CefSlateBlit.usf", "MainPS", SF_Pixel);

static FIntRect MakeIntersection(const FIntRect& a, const FIntRect& b)
{
	FIntRect r = a;
	r.Clip(b);
	return r;
}

static void AddCefSlateBlitPass(
	FRDGBuilder& graphBuilder,
	FRDGTextureRef inputTexture,
	FRDGTextureRef outputTexture,
	const FIntRect& destRect,
	const FIntRect& srcRect,
	const FIntPoint& srcExtent)
{
	if (!inputTexture || !outputTexture || destRect.Width() <= 0 || destRect.Height() <= 0 || srcRect.Width() <= 0 || srcRect.Height() <= 0)
	{
		return;
	}

	TShaderMapRef<FCefSlateBlitPS> pixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	FCefSlateBlitPS::FParameters* passParameters = graphBuilder.AllocParameters<FCefSlateBlitPS::FParameters>();
	passParameters->InputTexture = graphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(inputTexture));
	passParameters->InputSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
	passParameters->DestMin = FVector2f(static_cast<float>(destRect.Min.X), static_cast<float>(destRect.Min.Y));
	passParameters->DestSize = FVector2f(static_cast<float>(destRect.Width()), static_cast<float>(destRect.Height()));
	passParameters->SrcMin = FVector2f(static_cast<float>(srcRect.Min.X), static_cast<float>(srcRect.Min.Y));
	passParameters->SrcSize = FVector2f(static_cast<float>(srcRect.Width()), static_cast<float>(srcRect.Height()));
	passParameters->SrcTexSize = FVector2f(static_cast<float>(FMath::Max(1, srcExtent.X)), static_cast<float>(FMath::Max(1, srcExtent.Y)));
	passParameters->RenderTargets[0] = FRenderTargetBinding(outputTexture, ERenderTargetLoadAction::ELoad);

	FPixelShaderUtils::AddFullscreenPass(
		graphBuilder,
		GetGlobalShaderMap(GMaxRHIFeatureLevel),
		RDG_EVENT_NAME("CefSlateBlit"),
		pixelShader,
		passParameters,
		destRect);
}

class FCefBrowserSurfaceDrawer : public ICustomSlateElement, public TSharedFromThis<FCefBrowserSurfaceDrawer, ESPMode::ThreadSafe>
{
public:
	void EnqueueUpdate(
		const FTextureRHIRef& inMainTexture,
		const FTextureRHIRef& inPopupTexture,
		const FCefSharedFrame& inFrame,
		const FIntRect& inDestRect)
	{
		TSharedRef<FCefBrowserSurfaceDrawer, ESPMode::ThreadSafe> self = AsShared();
		const FIntPoint sourceSize(
			FMath::Max(1, static_cast<int32>(inFrame.Width)),
			FMath::Max(1, static_cast<int32>(inFrame.Height)));
		const bool popupPlane = ((inFrame.Flags & CefFrameFlag_PopupPlane) != 0);
		const bool popupVisible = inFrame.bPopupVisible;
		const FIntRect popupRectangle(
			inFrame.PopupRect.X,
			inFrame.PopupRect.Y,
			inFrame.PopupRect.X + inFrame.PopupRect.W,
			inFrame.PopupRect.Y + inFrame.PopupRect.H);

		ENQUEUE_RENDER_COMMAND(CefSlateDrawerUpdate)(
			[self, inMainTexture, inPopupTexture, inDestRect, sourceSize, popupRectangle, popupVisible, popupPlane](FRHICommandListImmediate&)
			{
				self->mainTexture = inMainTexture;
				self->popupTexture = inPopupTexture;
				self->destRect = inDestRect;
				self->srcSize = sourceSize;
				self->popupRect = popupRectangle;
				self->bPopupVisible = popupVisible;
				self->bPopupPlane = popupPlane;
			});
	}

	virtual void Draw_RenderThread(FRDGBuilder& graphBuilder, const FDrawPassInputs& inputs) override
	{
		if (!mainTexture.IsValid() || !inputs.OutputTexture)
		{
			return;
		}

		const FIntRect outputRect(FIntPoint::ZeroValue, inputs.OutputTexture->Desc.Extent);
		FIntRect clippedDest = MakeIntersection(MakeIntersection(destRect, inputs.SceneViewRect), outputRect);
		if (clippedDest.Width() <= 0 || clippedDest.Height() <= 0)
		{
			return;
		}

		FRDGTextureRef mainTex = RegisterExternalTexture(graphBuilder, mainTexture.GetReference(), TEXT("CefSlateMainTex"), ERDGTextureFlags::ForceImmediateFirstBarrier);
		const FIntRect srcRect(0, 0, srcSize.X, srcSize.Y);
		AddCefSlateBlitPass(graphBuilder, mainTex, inputs.OutputTexture, clippedDest, srcRect, srcSize);

		if (!bPopupPlane || !bPopupVisible || !popupTexture.IsValid() || popupRect.Width() <= 0 || popupRect.Height() <= 0)
		{
			return;
		}

		const float sx = static_cast<float>(FMath::Max(1, destRect.Width())) / static_cast<float>(FMath::Max(1, srcSize.X));
		const float sy = static_cast<float>(FMath::Max(1, destRect.Height())) / static_cast<float>(FMath::Max(1, srcSize.Y));
		FIntRect popupDest(
			destRect.Min.X + FMath::RoundToInt(static_cast<float>(popupRect.Min.X) * sx),
			destRect.Min.Y + FMath::RoundToInt(static_cast<float>(popupRect.Min.Y) * sy),
			destRect.Min.X + FMath::RoundToInt(static_cast<float>(popupRect.Max.X) * sx),
			destRect.Min.Y + FMath::RoundToInt(static_cast<float>(popupRect.Max.Y) * sy));
		popupDest = MakeIntersection(MakeIntersection(popupDest, clippedDest), outputRect);
		if (popupDest.Width() <= 0 || popupDest.Height() <= 0)
		{
			return;
		}

		FRDGTextureRef popupTex = RegisterExternalTexture(graphBuilder, popupTexture.GetReference(), TEXT("CefSlatePopupTex"), ERDGTextureFlags::ForceImmediateFirstBarrier);
		AddCefSlateBlitPass(graphBuilder, popupTex, inputs.OutputTexture, popupDest, popupRect, srcSize);
	}

private:
	FTextureRHIRef mainTexture;
	FTextureRHIRef popupTexture;
	FIntRect destRect;
	FIntRect popupRect;
	FIntPoint srcSize = FIntPoint::ZeroValue;
	bool bPopupVisible = false;
	bool bPopupPlane = false;
};

SCefBrowserSurface::~SCefBrowserSurface()
{
	ReleaseResources();
}

void SCefBrowserSurface::Construct(const FArguments& inArgs)
{
	BrowserSession = inArgs._BrowserSession;
	BrowserWidth = FMath::Max(1, inArgs._BrowserWidth);
	BrowserHeight = FMath::Max(1, inArgs._BrowserHeight);
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
	TelemetryFenceNotReadyCount = 0;
	SET_DWORD_STAT(STAT_CefTel_ConsumedFrames, 0);
	SET_DWORD_STAT(STAT_CefTel_FrameGaps, 0);
	SET_DWORD_STAT(STAT_CefTel_InputToFrameLatencyMs, 0);
	SET_DWORD_STAT(STAT_CefTel_InputToFrameLatencyMaxMs, 0);
	SET_DWORD_STAT(STAT_CefTel_FenceNotReady, 0);
}

void SCefBrowserSurface::SetBrowserSession(TWeakObjectPtr<UCefWebUiBrowserSession> inBrowserSession)
{
	BrowserSession = inBrowserSession;
	UnbindFrameReaderDelegate();
	FrameReader.Reset();
	ControlWriter.Reset();
}

void SCefBrowserSurface::SetBrowserSize(int32 inBrowserWidth, int32 inBrowserHeight)
{
	BrowserWidth = FMath::Max(1, inBrowserWidth);
	BrowserHeight = FMath::Max(1, inBrowserHeight);
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
	if (!SharedTextureRHI[slot].IsValid())
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
		EnsurePopupPlaneRhi();
	}

	const FSlateRect renderRect = allottedGeometry.GetRenderBoundingRect();
	const FIntRect destRect(
		FMath::RoundToInt(renderRect.Left),
		FMath::RoundToInt(renderRect.Top),
		FMath::RoundToInt(renderRect.Right),
		FMath::RoundToInt(renderRect.Bottom));
	CustomDrawer->EnqueueUpdate(
		SharedTextureRHI[slot],
		SharedPopupTextureRHI,
		LastFrame,
		destRect);

	FSlateDrawElement::MakeCustom(outDrawElements, layerId, CustomDrawer);
	return layerId + 1;
}

FVector2D SCefBrowserSurface::ComputeDesiredSize(float layoutScaleMultiplier) const
{
	return FVector2D(static_cast<float>(BrowserWidth), static_cast<float>(BrowserHeight));
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

	const uint32 modifiers = keyEvent.GetModifierKeys().IsLeftControlDown() ? 0x0002u : 0u;
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

	const uint32 modifiers = keyEvent.GetModifierKeys().IsLeftControlDown() ? 0x0002u : 0u;
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

	inputWriter->WriteChar(static_cast<uint16>(characterEvent.GetCharacter()));
	return FReply::Handled();
}

void SCefBrowserSurface::HandleFrameReady()
{
	Invalidate(EInvalidateWidgetReason::Paint);
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

	if (LastHostTuningPushTimeSec > 0.0 && (nowSec - LastHostTuningPushTimeSec) < kHostTuningPushPeriodSec)
	{
		return;
	}

	controlWriter->SetFrameRate(kHostFrameRate);
	controlWriter->SetMaxInFlightBeginFrames(kHostMaxInFlightBeginFrames);
	controlWriter->SetFlushIntervalFrames(kHostFlushIntervalFrames);
	controlWriter->SetKeyframeIntervalUs(kHostKeyframeIntervalUs);
	LastHostTuningPushTimeSec = nowSec;
}

void SCefBrowserSurface::MaybeUpdateCadenceFeedback(double nowSec) const
{
	if (!kEnableCadenceFeedback)
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

	const double hz = kCursorUpdateRateHz;
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
	const double logPeriodSec = kTelemetryLogPeriodSec;
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
		TEXT("[CefSlateTelemetry] frames=%u gaps=%u fence_not_ready=%u input_latency_avg_ms=%u input_latency_max_ms=%u last_latency_frame_id=%llu"),
		TelemetryConsumedFrames,
		TelemetryFrameGapCount,
		TelemetryFenceNotReadyCount,
		avgInputLatencyMs,
		TelemetryInputLatencyMsMax,
		static_cast<unsigned long long>(LastLatencyFrameId));

	LastTelemetryLogTimeSec = nowSec;
	TelemetryConsumedFrames = 0;
	TelemetryFrameGapCount = 0;
	TelemetryInputLatencySamples = 0;
	TelemetryInputLatencyMsSum = 0;
	TelemetryInputLatencyMsMax = 0;
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
		if (!ready && DeferredRetryCount == 0)
		{
			++DeferredRetryCount;
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
			return;
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

	SharedSlotCount = FMath::Clamp(LastFrame.SlotCount, 1u, MaxSharedSlots);
	EnsureSharedRhi();
	if ((LastFrame.Flags & CefFrameFlag_PopupPlane) != 0)
	{
		EnsurePopupPlaneRhi();
	}
}

bool SCefBrowserSurface::EnsureSharedGpuFence() const
{
	if (SharedGpuFence)
	{
		return true;
	}
	if (bTriedOpenSharedGpuFence)
	{
		return false;
	}
	bTriedOpenSharedGpuFence = true;

	ID3D12DynamicRHI* d3d12Rhi = GetID3D12DynamicRHI();
	if (!d3d12Rhi)
	{
		return false;
	}

	ID3D12Device* d3dDevice = d3d12Rhi->RHIGetDevice(0);
	if (!d3dDevice)
	{
		return false;
	}

	HANDLE sharedHandle = nullptr;
	HRESULT hr = d3dDevice->OpenSharedHandleByName(L"Global\\CEFHost_SharedFence", GENERIC_ALL, &sharedHandle);
	if (FAILED(hr) || !sharedHandle)
	{
		return false;
	}

	ID3D12Fence* fence = nullptr;
	hr = d3dDevice->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&fence));
	CloseHandle(sharedHandle);
	if (FAILED(hr) || !fence)
	{
		return false;
	}

	SharedGpuFence = fence;
	return true;
}

bool SCefBrowserSurface::IsFrameGpuReady(const FCefSharedFrame& frame) const
{
	if (!kUseGpuFenceCheck)
	{
		return true;
	}
	if (frame.GpuFenceValue == 0)
	{
		return true;
	}
	if (!EnsureSharedGpuFence())
	{
		return true;
	}

	return SharedGpuFence->GetCompletedValue() >= frame.GpuFenceValue;
}

void SCefBrowserSurface::EnsureSharedRhi() const
{
	bool bReady = true;
	for (uint32 i = 0; i < SharedSlotCount; ++i)
	{
		if (!SharedTextureRHI[i].IsValid())
		{
			bReady = false;
			break;
		}
	}
	if (bReady)
	{
		return;
	}

	ENQUEUE_RENDER_COMMAND(CefSlateOpenSharedTextures)(
		[this](FRHICommandListImmediate&)
		{
			ID3D12DynamicRHI* d3d12Rhi = GetID3D12DynamicRHI();
			if (!d3d12Rhi)
			{
				return;
			}

			ID3D12Device* d3dDevice = d3d12Rhi->RHIGetDevice(0);
			if (!d3dDevice)
			{
				return;
			}

			for (uint32 i = 0; i < SharedSlotCount; ++i)
			{
				if (SharedTextureRHI[i].IsValid())
				{
					continue;
				}

				wchar_t name[64];
				swprintf(name, 64, L"Global\\CEFHost_SharedTex_%u", i);
				HANDLE sharedHandle = nullptr;
				HRESULT hr = d3dDevice->OpenSharedHandleByName(name, GENERIC_ALL, &sharedHandle);
				if (FAILED(hr) || !sharedHandle)
				{
					return;
				}

				ID3D12Resource* d3dResource = nullptr;
				hr = d3dDevice->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&d3dResource));
				CloseHandle(sharedHandle);
				if (FAILED(hr) || !d3dResource)
				{
					return;
				}

				SharedTextureRHI[i] = d3d12Rhi->RHICreateTexture2DFromResource(
					PF_B8G8R8A8,
					ETextureCreateFlags::External | ETextureCreateFlags::ShaderResource,
					FClearValueBinding::None,
					d3dResource);
				d3dResource->Release();
			}
		});
	FlushRenderingCommands();
}

bool SCefBrowserSurface::EnsurePopupPlaneRhi() const
{
	if (SharedPopupTextureRHI.IsValid())
	{
		return true;
	}

	ENQUEUE_RENDER_COMMAND(CefSlateOpenPopupTexture)(
		[this](FRHICommandListImmediate&)
		{
			if (SharedPopupTextureRHI.IsValid())
			{
				return;
			}

			ID3D12DynamicRHI* d3d12Rhi = GetID3D12DynamicRHI();
			if (!d3d12Rhi)
			{
				return;
			}

			ID3D12Device* d3dDevice = d3d12Rhi->RHIGetDevice(0);
			if (!d3dDevice)
			{
				return;
			}

			HANDLE sharedHandle = nullptr;
			HRESULT hr = d3dDevice->OpenSharedHandleByName(L"Global\\CEFHost_SharedPopupTex", GENERIC_ALL, &sharedHandle);
			if (FAILED(hr) || !sharedHandle)
			{
				return;
			}

			ID3D12Resource* d3dResource = nullptr;
			hr = d3dDevice->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&d3dResource));
			CloseHandle(sharedHandle);
			if (FAILED(hr) || !d3dResource)
			{
				return;
			}

			SharedPopupTextureRHI = d3d12Rhi->RHICreateTexture2DFromResource(
				PF_B8G8R8A8,
				ETextureCreateFlags::External | ETextureCreateFlags::ShaderResource,
				FClearValueBinding::None,
				d3dResource);
			d3dResource->Release();
		});
	FlushRenderingCommands();
	return SharedPopupTextureRHI.IsValid();
}

void SCefBrowserSurface::ReleaseResources()
{
	UnbindFrameReaderDelegate();

	TSharedPtr<FCefBrowserSurfaceDrawer, ESPMode::ThreadSafe> drawer = CustomDrawer;
	if (drawer.IsValid())
	{
		ENQUEUE_RENDER_COMMAND(CefSlateReleaseDrawer)(
			[drawer](FRHICommandListImmediate&) mutable
			{
				drawer.Reset();
			});
		CustomDrawer.Reset();
	}

	FTextureRHIRef oldMain0 = SharedTextureRHI[0];
	FTextureRHIRef oldMain1 = SharedTextureRHI[1];
	FTextureRHIRef oldMain2 = SharedTextureRHI[2];
	FTextureRHIRef oldPopup = SharedPopupTextureRHI;
	ENQUEUE_RENDER_COMMAND(CefSlateReleaseSharedTextures)(
		[oldMain0, oldMain1, oldMain2, oldPopup](FRHICommandListImmediate&) mutable
		{
			oldMain0.SafeRelease();
			oldMain1.SafeRelease();
			oldMain2.SafeRelease();
			oldPopup.SafeRelease();
		});

	for (uint32 i = 0; i < MaxSharedSlots; ++i)
	{
		SharedTextureRHI[i] = nullptr;
	}
	SharedPopupTextureRHI = nullptr;
	bHasFrame = false;
	bHasDeferredFrame = false;
	DeferredRetryCount = 0;
	FrameReader.Reset();
	ControlWriter.Reset();
	if (SharedGpuFence)
	{
		SharedGpuFence->Release();
		SharedGpuFence = nullptr;
	}
	bTriedOpenSharedGpuFence = false;
}

void SCefBrowserSurface::GetBrowserCoords(const FGeometry& inGeometry, const FVector2D& inScreenPosition, int32& outX, int32& outY) const
{
	const FVector2D localPos = inGeometry.AbsoluteToLocal(inScreenPosition);
	const FVector2D localSize = inGeometry.GetLocalSize();
	const float width = FMath::Max(localSize.X, 1.0f);
	const float height = FMath::Max(localSize.Y, 1.0f);
	outX = FMath::Clamp(FMath::RoundToInt(localPos.X / width * BrowserWidth), 0, BrowserWidth - 1);
	outY = FMath::Clamp(FMath::RoundToInt(localPos.Y / height * BrowserHeight), 0, BrowserHeight - 1);
}

bool SCefBrowserSurface::SlateButtonToCef(const FKey& inKey, ECefMouseButton& outButton)
{
	if (inKey == EKeys::LeftMouseButton)
	{
		outButton = ECefMouseButton::Left;
		return true;
	}
	if (inKey == EKeys::MiddleMouseButton)
	{
		outButton = ECefMouseButton::Middle;
		return true;
	}
	if (inKey == EKeys::RightMouseButton)
	{
		outButton = ECefMouseButton::Right;
		return true;
	}
	return false;
}
