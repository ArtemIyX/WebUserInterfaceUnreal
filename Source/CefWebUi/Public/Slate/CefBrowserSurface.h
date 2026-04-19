#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "Rendering/RenderingCommon.h"
#include "Widgets/SLeafWidget.h"
#include "Services/CefFrameReader.h"
#include "Services/CefInputWriter.h"

struct ID3D12Fence;

class FCefInputWriter;
class FCefFrameReader;
class FCefControlWriter;
class UCefWebUiBrowserSession;
class FCefBrowserSurfaceDrawer;

class CEFWEBUI_API SCefBrowserSurface : public SLeafWidget
{
public:
#pragma region Lifecycle
	virtual ~SCefBrowserSurface() override;

	SLATE_BEGIN_ARGS(SCefBrowserSurface)
		: _BrowserWidth(1920)
		, _BrowserHeight(1080)
	{}
		SLATE_ARGUMENT(TWeakObjectPtr<UCefWebUiBrowserSession>, BrowserSession)
		SLATE_ARGUMENT(int32, BrowserWidth)
		SLATE_ARGUMENT(int32, BrowserHeight)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
#pragma endregion

#pragma region PublicApi
	void SetBrowserSession(TWeakObjectPtr<UCefWebUiBrowserSession> InBrowserSession);
	void SetBrowserSize(int32 InBrowserWidth, int32 InBrowserHeight);
#pragma endregion

#pragma region SWidget
	virtual int32 OnPaint(
		const FPaintArgs& InArgs,
		const FGeometry& InAllottedGeometry,
		const FSlateRect& InMyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 InLayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bInParentEnabled) const override;
	virtual void Tick(const FGeometry& InAllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	virtual FVector2D ComputeDesiredSize(float InLayoutScaleMultiplier) const override;
	virtual bool ComputeVolatility() const override { return true; }
	virtual bool SupportsKeyboardFocus() const override { return true; }

	virtual FReply OnMouseMove(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& InMyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnKeyUp(const FGeometry& InMyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnKeyChar(const FGeometry& InMyGeometry, const FCharacterEvent& InCharacterEvent) override;
#pragma endregion

private:
#pragma region InternalMethods
	EActiveTimerReturnType HandleActiveTimer(double InCurrentTime, float InDeltaTime);
	void HandleFrameReady();
	void UnbindFrameReaderDelegate();
	bool TryGetFrameReader(TSharedPtr<FCefFrameReader>& OutFrameReader) const;
	bool TryGetInputWriter(TSharedPtr<FCefInputWriter>& OutInputWriter) const;
	bool TryGetControlWriter(TSharedPtr<FCefControlWriter>& OutControlWriter) const;
	void PollLatestFrame() const;
	void MaybePushHostTuning(double InNowSec) const;
	void MaybeUpdateCadenceFeedback(double InNowSec) const;
	void UpdateFrameTelemetry(const FCefSharedFrame& InFrame) const;
	void MaybeUpdateCursor(const FCefSharedFrame& InFrame, double InNowSec) const;
	void MaybeLogAndResetTelemetry(double InNowSec) const;
	void MarkInputEvent();
	void EnsureSharedRhi() const;
	bool EnsurePopupPlaneRhi() const;
	bool EnsureSharedGpuFence() const;
	bool IsFrameGpuReady(const FCefSharedFrame& InFrame) const;
	void ReleaseResources();
	void GetBrowserCoords(const FGeometry& InGeometry, const FVector2D& InScreenPosition, int32& OutX, int32& OutY) const;
	static bool SlateButtonToCef(const FKey& InKey, ECefMouseButton& OutButton);
#pragma endregion

private:
#pragma region InternalState
	static constexpr uint32 MaxSharedSlots = 3;

	TWeakObjectPtr<UCefWebUiBrowserSession> BrowserSession;
	int32 BrowserWidth = 1920;
	int32 BrowserHeight = 1080;

	mutable TWeakPtr<FCefFrameReader> FrameReader;
	mutable TWeakPtr<FCefControlWriter> ControlWriter;
	mutable FDelegateHandle FrameReadyDelegateHandle;
	mutable FCefSharedFrame LastFrame;
	mutable FCefSharedFrame DeferredFrame;
	mutable bool bHasFrame = false;
	mutable bool bHasDeferredFrame = false;
	mutable uint32 DeferredRetryCount = 0;
	mutable uint32 SharedSlotCount = 2;
	mutable FTextureRHIRef SharedTextureRHI[MaxSharedSlots];
	mutable FTextureRHIRef SharedPopupTextureRHI;
	mutable ID3D12Fence* SharedGpuFence = nullptr;
	mutable bool bTriedOpenSharedGpuFence = false;
	mutable TSharedPtr<FCefBrowserSurfaceDrawer, ESPMode::ThreadSafe> CustomDrawer;
	mutable double LastConsumerFrameTimeSec = 0.0;
	mutable double LastCadenceSentTimeSec = 0.0;
	mutable double LastHostTuningPushTimeSec = 0.0;
	mutable double LastTelemetryLogTimeSec = 0.0;
	mutable double LastInputEventTimeSec = 0.0;
	mutable double LastInputToFrameLatencyMs = 0.0;
	mutable double LastCursorUpdateTimeSec = 0.0;
	mutable uint32 SmoothedCadenceUs = 0;
	mutable uint64 LastSeenFrameId = 0;
	mutable uint64 InputEventSerial = 0;
	mutable uint64 LastLatencyMeasuredInputSerial = 0;
	mutable uint64 LastLatencyFrameId = 0;
	mutable uint32 TelemetryFenceNotReadyCount = 0;
	mutable ECefCustomCursorType LastCursorType = ECefCustomCursorType::CT_NONE;
	mutable uint32 TelemetryConsumedFrames = 0;
	mutable uint32 TelemetryFrameGapCount = 0;
	mutable uint32 TelemetryInputLatencySamples = 0;
	mutable uint32 TelemetryInputLatencyMsSum = 0;
	mutable uint32 TelemetryInputLatencyMsMax = 0;
	mutable uint32 TelemetryPaintCalls = 0;
	mutable uint32 TelemetryTickCalls = 0;
	mutable uint32 TelemetryTimerCalls = 0;
	mutable uint32 TelemetryPollSuccess = 0;
#pragma endregion
};
