// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CefWebUiBrowserWidget.generated.h"

enum class ECefLoadState : uint8;
enum class ECefMouseButton : uint8;
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class CEFWEBUI_API UCefWebUiBrowserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCefWebUiBrowserWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

private:
#pragma region LifeCycle
	void PollAndUpload();

protected:
	void EnsureTexture(uint32 InWidth, uint32 InHeight);
#pragma endregion

private:
#pragma region ServiceVariables
	float AccumulatedTime = 0.0f;
	uint32 TextureWidth = 0;
	uint32 TextureHeight = 0;

	TWeakPtr<class FCefInputWriter> InputWriter;
	TWeakPtr<class FCefFrameReader> FrameReader;
	TWeakPtr<class FCefControlWriter> ControlWriter;

#pragma endregion

protected:
#pragma region Bindings
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Display")
	TObjectPtr<class UImage> DisplayImage;

#pragma endregion

protected:
#pragma region Cached
	UPROPERTY()
	TObjectPtr<UTexture2D> DisplayTexture;

#pragma endregion

protected:
#pragma region Config

	/**
	 * @brief Browser render target width in pixels. Must match CEF Host resolution.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CefBrowser")
	int32 BrowserWidth = 1920;

	/**
	 * @brief Browser render target height in pixels. Must match CEF Host resolution.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CefBrowser")
	int32 BrowserHeight = 1080;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CefBrowser")
	float TargetFPS = 60.0f;
#pragma region Handlers
	UFUNCTION()
	virtual void OnLoadStateChanged(uint8 InState);

#pragma endregion
#pragma endregion Config

public:
#pragma region UUserWidget overrides


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

#pragma endregion UUserWidget overrides

#pragma region Blueprint Browser Control
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
#pragma endregion

private:
#pragma region Helpers

	/**
	 * @brief Converts a Slate pointer event position to browser pixel coordinates.
	 * @param InGeometry Widget geometry used for local space conversion.
	 * @param InScreenPosition Screen-space position from the pointer event.
	 * @param OutX Browser X pixel coordinate.
	 * @param OutY Browser Y pixel coordinate.
	 */
	void GetBrowserCoords(const FGeometry& InGeometry, const FVector2D& InScreenPosition,
	                      int32& OutX, int32& OutY) const;

	/**
	 * @brief Maps a Slate FKey mouse button to ECefMouseButton.
	 * @param InKey Slate key (EKeys::LeftMouseButton etc).
	 * @param OutButton CEF mouse button enum.
	 * @return False if the key is not a recognized mouse button.
	 */
	static bool SlateButtonToCef(const FKey& InKey, ECefMouseButton& OutButton);

#pragma endregion Helpers

public:
#pragma region Getters
	TSharedRef<class FCefInputWriter> GetInputWriter() const;
	TSharedRef<class FCefFrameReader> GetFrameReader() const;
	TSharedRef<class FCefControlWriter> GetControlWriter() const;

#pragma endregion
};
