/**
 * @file CefWebUi\Public\Slate\CefBrowserSurface.h
 * @brief Declares CefBrowserSurface for module CefWebUi\Public\Slate\CefBrowserSurface.h.
 * @details Contains types and APIs used by the plugin runtime and gameplay-facing systems.
 */
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

/** @brief Type declaration. */
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

	/** @brief Construct API. */
	void Construct(const FArguments& InArgs);
#pragma endregion

#pragma region PublicApi
	/** @brief SetBrowserSession API. */
	void SetBrowserSession(TWeakObjectPtr<UCefWebUiBrowserSession> InBrowserSession);
	/** @brief SetBrowserSize API. */
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
	/** @brief HandleActiveTimer API. */
	EActiveTimerReturnType HandleActiveTimer(double InCurrentTime, float InDeltaTime);
	/** @brief HandleFrameReady API. */
	void HandleFrameReady();
	/** @brief UnbindFrameReaderDelegate API. */
	void UnbindFrameReaderDelegate();
	/** @brief TryGetFrameReader API. */
	bool TryGetFrameReader(TSharedPtr<FCefFrameReader>& OutFrameReader) const;
	/** @brief TryGetInputWriter API. */
	bool TryGetInputWriter(TSharedPtr<FCefInputWriter>& OutInputWriter) const;
	/** @brief TryGetControlWriter API. */
	bool TryGetControlWriter(TSharedPtr<FCefControlWriter>& OutControlWriter) const;
	/** @brief PollLatestFrame API. */
	void PollLatestFrame() const;
	/** @brief MaybePushHostTuning API. */
	void MaybePushHostTuning(double InNowSec) const;
	/** @brief MaybeUpdateCadenceFeedback API. */
	void MaybeUpdateCadenceFeedback(double InNowSec) const;
	/** @brief UpdateFrameTelemetry API. */
	void UpdateFrameTelemetry(const FCefSharedFrame& InFrame) const;
	/** @brief MaybeUpdateCursor API. */
	void MaybeUpdateCursor(const FCefSharedFrame& InFrame, double InNowSec) const;
	/** @brief MaybeLogAndResetTelemetry API. */
	void MaybeLogAndResetTelemetry(double InNowSec) const;
	/** @brief MarkInputEvent API. */
	void MarkInputEvent();
	/** @brief EnsureSharedRhi API. */
	void EnsureSharedRhi() const;
	/** @brief EnsurePopupPlaneRhi API. */
	bool EnsurePopupPlaneRhi() const;
	/** @brief EnsureSharedGpuFence API. */
	bool EnsureSharedGpuFence() const;
	/** @brief IsFrameGpuReady API. */
	bool IsFrameGpuReady(const FCefSharedFrame& InFrame) const;
	/** @brief MaybeQueueAutoResize API. */
	void MaybeQueueAutoResize(const FGeometry& InAllottedGeometry, double InNowSec);
	/** @brief MaybeSendAutoResize API. */
	void MaybeSendAutoResize(double InNowSec);
	/** @brief HandleAppliedFrameSize API. */
	void HandleAppliedFrameSize(int32 InFrameWidth, int32 InFrameHeight) const;
	/** @brief ReleaseResources API. */
	void ReleaseResources();
	/** @brief GetBrowserCoords API. */
	void GetBrowserCoords(const FGeometry& InGeometry, const FVector2D& InScreenPosition, int32& OutX, int32& OutY) const;
	/** @brief SlateButtonToCef API. */
	static bool SlateButtonToCef(const FKey& InKey, ECefMouseButton& OutButton);
#pragma endregion

private:
#pragma region InternalState
	/** @brief MaxSharedSlots state. */
	static constexpr uint32 MaxSharedSlots = 3;

	/** @brief BrowserSession state. */
	TWeakObjectPtr<UCefWebUiBrowserSession> BrowserSession;
	/** @brief DesiredBrowserWidth state. */
	int32 DesiredBrowserWidth = 1920;
	/** @brief DesiredBrowserHeight state. */
	int32 DesiredBrowserHeight = 1080;
	/** @brief AppliedBrowserWidth state. */
	mutable int32 AppliedBrowserWidth = 1920;
	/** @brief AppliedBrowserHeight state. */
	mutable int32 AppliedBrowserHeight = 1080;
	/** @brief TargetBrowserWidth state. */
	int32 TargetBrowserWidth = 0;
	/** @brief TargetBrowserHeight state. */
	int32 TargetBrowserHeight = 0;
	/** @brief LastSentBrowserWidth state. */
	int32 LastSentBrowserWidth = 0;
	/** @brief LastSentBrowserHeight state. */
	int32 LastSentBrowserHeight = 0;
	/** @brief bAutoResizePending state. */
	bool bAutoResizePending = false;
	/** @brief bAwaitingAutoResizeApply state. */
	mutable bool bAwaitingAutoResizeApply = false;
	/** @brief AutoResizeRetryCount state. */
	mutable int32 AutoResizeRetryCount = 0;
	/** @brief bAutoResizeFailureLogged state. */
	mutable bool bAutoResizeFailureLogged = false;
	/** @brief LastAutoResizeObservedTimeSec state. */
	double LastAutoResizeObservedTimeSec = 0.0;
	/** @brief LastAutoResizeSentTimeSec state. */
	double LastAutoResizeSentTimeSec = 0.0;
	/** @brief LastAutoResizeAwaitStartTimeSec state. */
	mutable double LastAutoResizeAwaitStartTimeSec = 0.0;

	/** @brief FrameReader state. */
	mutable TWeakPtr<FCefFrameReader> FrameReader;
	/** @brief ControlWriter state. */
	mutable TWeakPtr<FCefControlWriter> ControlWriter;
	/** @brief FrameReadyDelegateHandle state. */
	mutable FDelegateHandle FrameReadyDelegateHandle;
	/** @brief LastFrame state. */
	mutable FCefSharedFrame LastFrame;
	/** @brief DeferredFrame state. */
	mutable FCefSharedFrame DeferredFrame;
	/** @brief bHasFrame state. */
	mutable bool bHasFrame = false;
	/** @brief bHasDeferredFrame state. */
	mutable bool bHasDeferredFrame = false;
	/** @brief DeferredRetryCount state. */
	mutable uint32 DeferredRetryCount = 0;
	/** @brief SharedSlotCount state. */
	mutable uint32 SharedSlotCount = 2;
	mutable FTextureRHIRef SharedTextureRHI[MaxSharedSlots];
	/** @brief SharedPopupTextureRHI state. */
	mutable FTextureRHIRef SharedPopupTextureRHI;
	/** @brief SharedGpuFence state. */
	mutable ID3D12Fence* SharedGpuFence = nullptr;
	/** @brief bTriedOpenSharedGpuFence state. */
	mutable bool bTriedOpenSharedGpuFence = false;
	/** @brief bSharedTexturesReopenPending state. */
	mutable bool bSharedTexturesReopenPending = false;
	/** @brief CustomDrawer state. */
	mutable TSharedPtr<FCefBrowserSurfaceDrawer, ESPMode::ThreadSafe> CustomDrawer;
	/** @brief LastConsumerFrameTimeSec state. */
	mutable double LastConsumerFrameTimeSec = 0.0;
	/** @brief LastCadenceSentTimeSec state. */
	mutable double LastCadenceSentTimeSec = 0.0;
	/** @brief LastHostTuningPushTimeSec state. */
	mutable double LastHostTuningPushTimeSec = 0.0;
	/** @brief LastTelemetryLogTimeSec state. */
	mutable double LastTelemetryLogTimeSec = 0.0;
	/** @brief LastInputEventTimeSec state. */
	mutable double LastInputEventTimeSec = 0.0;
	/** @brief LastInputToFrameLatencyMs state. */
	mutable double LastInputToFrameLatencyMs = 0.0;
	/** @brief LastCursorUpdateTimeSec state. */
	mutable double LastCursorUpdateTimeSec = 0.0;
	/** @brief SmoothedCadenceUs state. */
	mutable uint32 SmoothedCadenceUs = 0;
	/** @brief LastSeenFrameId state. */
	mutable uint64 LastSeenFrameId = 0;
	/** @brief InputEventSerial state. */
	mutable uint64 InputEventSerial = 0;
	/** @brief LastLatencyMeasuredInputSerial state. */
	mutable uint64 LastLatencyMeasuredInputSerial = 0;
	/** @brief LastLatencyFrameId state. */
	mutable uint64 LastLatencyFrameId = 0;
	/** @brief TelemetryFenceNotReadyCount state. */
	mutable uint32 TelemetryFenceNotReadyCount = 0;
	/** @brief LastCursorType state. */
	mutable ECefCustomCursorType LastCursorType = ECefCustomCursorType::CT_NONE;
	/** @brief TelemetryConsumedFrames state. */
	mutable uint32 TelemetryConsumedFrames = 0;
	/** @brief TelemetryFrameGapCount state. */
	mutable uint32 TelemetryFrameGapCount = 0;
	/** @brief TelemetryInputLatencySamples state. */
	mutable uint32 TelemetryInputLatencySamples = 0;
	/** @brief TelemetryInputLatencyMsSum state. */
	mutable uint32 TelemetryInputLatencyMsSum = 0;
	/** @brief TelemetryInputLatencyMsMax state. */
	mutable uint32 TelemetryInputLatencyMsMax = 0;
	/** @brief TelemetryPaintCalls state. */
	mutable uint32 TelemetryPaintCalls = 0;
	/** @brief TelemetryTickCalls state. */
	mutable uint32 TelemetryTickCalls = 0;
	/** @brief TelemetryTimerCalls state. */
	mutable uint32 TelemetryTimerCalls = 0;
	/** @brief TelemetryPollSuccess state. */
	mutable uint32 TelemetryPollSuccess = 0;
#pragma endregion
};

