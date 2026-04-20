/**
 * @file CefWebUi\Public\Debug\RenderTestActor.h
 * @brief Declares RenderTestActor for module CefWebUi\Public\Debug\RenderTestActor.h.
 * @details Contains types and APIs used by the plugin runtime and gameplay-facing systems.
 */
// ARenderTestActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RenderTestActor.generated.h"

DECLARE_CYCLE_STAT(TEXT("RenderTest: Pixel Generation"), STAT_RenderTest_PixelGen, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("RenderTest: RHI Upload"), STAT_RenderTest_RHIUpload, STATGROUP_Game);

UCLASS()
/** @brief Type declaration. */
class CEFWEBUI_API ARenderTestActor : public AActor
{
	GENERATED_BODY()

public:
	/** @brief ARenderTestActor API. */
	ARenderTestActor(const FObjectInitializer& InObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type InEndPlayReason) override;
	virtual void Tick(float InDeltaTime) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Config")
	/** @brief WidgetClass state. */
	TSubclassOf<class UWBP_RenderTest> WidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Config")
	/** @brief TextureWidth state. */
	int32 TextureWidth = 512;

	UPROPERTY(EditDefaultsOnly, Category="Config")
	/** @brief TextureHeight state. */
	int32 TextureHeight = 512;

private:
	/** @brief CreateTexture API. */
	void CreateTexture();
	/** @brief UploadCheckerboard API. */
	void UploadCheckerboard(float InTime);

private:
	/** @brief Widget state. */
	TObjectPtr<class UWBP_RenderTest> Widget;
	/** @brief DisplayTexture state. */
	TObjectPtr<UTexture2D>            DisplayTexture;
	/** @brief ElapsedTime state. */
	float                             ElapsedTime = 0.f;
};
