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
UCLASS()
class CEFWEBUI_API UCefWebUiBrowserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCefWebUiBrowserWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Display")
	TObjectPtr<class UImage> DisplayImage;

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



private:
	void PollAndUpload();
	void EnsureTexture(uint32 InWidth, uint32 InHeight);

protected:
	UPROPERTY()
	TObjectPtr<UTexture2D> DisplayTexture;

	float TargetFPS = 60.0f;
	float AccumulatedTime = 0.0f;
	uint32 TextureWidth = 0;
	uint32 TextureHeight = 0;

	TWeakPtr<class FCefInputWriter> InputWriter;
	TWeakPtr<class FCefFrameReader> FrameReader;
};
