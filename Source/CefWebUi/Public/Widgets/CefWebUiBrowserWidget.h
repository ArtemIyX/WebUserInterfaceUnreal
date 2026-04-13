// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RHI.h"
#include "RHIResources.h"
#include "Services/CefFrameReader.h"
#include "CefWebUiBrowserWidget.generated.h"

enum class ECefLoadState : uint8;
enum class ECefMouseButton : uint8;

UCLASS(Blueprintable, BlueprintType)
class CEFWEBUI_API UCefWebUiBrowserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCefWebUiBrowserWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

private:
	void ResetRuntimeState();
	bool TryAcquireFrame(FCefSharedFrame& OutFrame, bool& bOutIdleRecoveryFullCopy, double NowSec);
	void UpdateCadenceFeedback(double NowSec, bool bIdleRecoveryFullCopy);
	void UpdateFrameGapTelemetry(const FCefSharedFrame& Frame, bool bIdleRecoveryFullCopy);
	bool ResolveUseDirtyRects(const FCefSharedFrame& Frame, double NowSec);
	void UpdateCopyTelemetry(const FCefSharedFrame& Frame, bool bUseDirtyRects);
	void MaybeLogAndResetTelemetry(double NowSec);
	void MaybeUpdateCursor(const FCefSharedFrame& Frame, double NowSec);
	bool EnsureGpuFence();
	void WaitForProducerGpuFence(uint64 FenceValue);
	void PollAndUpload();
	void EnsureSharedRHI();
	void EnsureRenderTarget(uint32 InWidth, uint32 InHeight);

private:
	static constexpr uint32 MaxSharedSlots = 3;
	uint32 TextureWidth = 0;
	uint32 TextureHeight = 0;
	uint32 SharedSlotCount = 2;
	double LastConsumerFrameTimeSec = 0.0;
	double LastCadenceSentTimeSec = 0.0;
	double LastUploadTimeSec = 0.0;
	double LastIdleRecoveryCopyTimeSec = 0.0;
	double LastSafetyFullCopyTimeSec = 0.0;
	double LastInputEventTimeSec = 0.0;
	double LastCursorUpdateTimeSec = 0.0;
	double GpuFenceAutoBlockUntilSec = 0.0;
	double LastTelemetryLogTimeSec = 0.0;
	uint32 SmoothedCadenceUs = 0;
	uint64 LastSeenFrameId = 0;
	uint32 PendingForceFullFrames = 0;
	uint32 GpuFenceTimeoutBurst = 0;
	ECefCustomCursorType LastCursorType = ECefCustomCursorType::CT_NONE;
	bool bLastUploadUsedDirty = false;
	bool bHasLastUploadedFrame = false;
	FCefSharedFrame LastUploadedFrame;
	uint32 TelemetryConsumedFrames = 0;
	uint32 TelemetryFrameGapCount = 0;
	uint32 TelemetryForcedFullCount = 0;
	uint32 TelemetryFullCopyCount = 0;
	uint32 TelemetryDirtyCopyCount = 0;
	uint32 TelemetryDirtyRectCountSum = 0;
	uint32 TelemetryDirtyRectAreaSum = 0;

	void* LastSharedHandle[MaxSharedSlots] = { nullptr };
	FTextureRHIRef SharedTextureRHI[MaxSharedSlots];
	void* SharedGpuFence = nullptr;      // ID3D12Fence*
	void* GpuFenceWaitEvent = nullptr;   // HANDLE

	TWeakPtr<class FCefInputWriter> InputWriter;
	TWeakPtr<class FCefFrameReader> FrameReader;
	TWeakPtr<class FCefControlWriter> ControlWriter;

protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Display")
	TObjectPtr<class UImage> DisplayImage;

protected:
	UPROPERTY()
	TObjectPtr<class UTextureRenderTarget2D> RenderTarget;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CefBrowser")
	int32 BrowserWidth = 1920;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CefBrowser")
	int32 BrowserHeight = 1080;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CefBrowser|Texture")
	uint8 bAutoGenerateMips:1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CefBrowser|Texture")
	TEnumAsByte<enum TextureFilter> RenderFilter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CefBrowser|Texture")
	uint8 bSRGB : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CefBrowser|Texture")
	TEnumAsByte<TextureCompressionSettings> RenderCompression;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CefBrowser|Texture")
	ETextureMipLoadOptions RenderMipLoadOptions;
	
	UFUNCTION()
	virtual void OnLoadStateChanged(uint8 InState);

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent) override;
	virtual bool NativeSupportsKeyboardFocus() const override { return true; }

	UFUNCTION(BlueprintCallable, meta=(DisplayName="Go Back"), Category="CefWebUI|Control")
	virtual void BP_GoBack();
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Go Forward"), Category="CefWebUI|Control")
	virtual void BP_GoForward();
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Stop Load"), Category="CefWebUI|Control")
	virtual void BP_StopLoad();
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Reload"), Category="CefWebUI|Control")
	virtual void BP_Reload();
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set URL"), Category="CefWebUI|Control")
	virtual void BP_SetURL(const FString& InURL);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Paused"), Category="CefWebUI|Control")
	virtual void BP_SetPaused(bool bInPaused);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Hidden"), Category="CefWebUI|Control")
	virtual void BP_SetHidden(bool bInHidden);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Focus"), Category="CefWebUI|Control")
	virtual void BP_SetFocus(bool bInFocus);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Zoom Level"), Category="CefWebUI|Control")
	virtual void BP_SetZoomLevel(float InLevel);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Frame Rate"), Category="CefWebUI|Control")
	virtual void BP_SetFrameRate(int32 InRate);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Scroll To"), Category="CefWebUI|Control")
	virtual void BP_ScrollTo(int32 InX, int32 InY);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Resize"), Category="CefWebUI|Control")
	virtual void BP_Resize(int32 InWidth, int32 InHeight);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Muted"), Category="CefWebUI|Control")
	virtual void BP_SetMuted(bool bInMuted);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Open Dev Tools"), Category="CefWebUI|Control")
	virtual void BP_OpenDevTools();
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Close Dev Tools"), Category="CefWebUI|Control")
	virtual void BP_CloseDevTools();
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Set Input Enabled"), Category="CefWebUI|Control")
	virtual void BP_SetInputEnabled(bool bInEnabled);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Execute JS"), Category="CefWebUI|Control")
	virtual void BP_ExecuteJS(const FString& InScript);
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Clear Cookies"), Category="CefWebUI|Control")
	virtual void BP_ClearCookies();

private:
	void GetBrowserCoords(const FGeometry& InGeometry, const FVector2D& InScreenPosition,
	                      int32& OutX, int32& OutY) const;
	static bool SlateButtonToCef(const FKey& InKey, ECefMouseButton& OutButton);

public:
	TSharedRef<class FCefInputWriter> GetInputWriter() const;
	TSharedRef<class FCefFrameReader> GetFrameReader() const;
	TSharedRef<class FCefControlWriter> GetControlWriter() const;
};
