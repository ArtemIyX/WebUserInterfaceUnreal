// ARenderTestActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RenderTestActor.generated.h"

DECLARE_CYCLE_STAT(TEXT("RenderTest: Pixel Generation"), STAT_RenderTest_PixelGen, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("RenderTest: RHI Upload"), STAT_RenderTest_RHIUpload, STATGROUP_Game);

UCLASS()
class CEFWEBUI_API ARenderTestActor : public AActor
{
	GENERATED_BODY()

public:
	ARenderTestActor(const FObjectInitializer& InObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type InEndPlayReason) override;
	virtual void Tick(float InDeltaTime) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Config")
	TSubclassOf<class UWBP_RenderTest> WidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Config")
	int32 TextureWidth = 512;

	UPROPERTY(EditDefaultsOnly, Category="Config")
	int32 TextureHeight = 512;

private:
	void CreateTexture();
	void UploadCheckerboard(float InTime);

private:
	TObjectPtr<class UWBP_RenderTest> Widget;
	TObjectPtr<UTexture2D>            DisplayTexture;
	float                             ElapsedTime = 0.f;
};