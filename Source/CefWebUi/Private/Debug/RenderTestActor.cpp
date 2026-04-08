// ARenderTestActor.cpp

#include "Debug/RenderTestActor.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "Debug/WBP_RenderTest.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ARenderTestActor::ARenderTestActor(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;
}

void ARenderTestActor::BeginPlay()
{
    Super::BeginPlay();

    CreateTexture();

    if (!WidgetClass)
        return;

    APlayerController* pc = UGameplayStatics::GetPlayerController(this, 0);
    if (!pc)
        return;

    Widget = CreateWidget<UWBP_RenderTest>(pc, WidgetClass);
    if (!Widget)
        return;

    Widget->SetDisplayTexture(DisplayTexture);
    Widget->AddToViewport();
}

void ARenderTestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Widget)
    {
        Widget->RemoveFromParent();
        Widget = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void ARenderTestActor::Tick(float InDeltaTime)
{
    Super::Tick(InDeltaTime);

    ElapsedTime += InDeltaTime;
    UploadCheckerboard(ElapsedTime);
}

void ARenderTestActor::CreateTexture()
{
    DisplayTexture = UTexture2D::CreateTransient(TextureWidth, TextureHeight, PF_B8G8R8A8);
    DisplayTexture->Filter = TF_Nearest;
    DisplayTexture->UpdateResource();
}

void ARenderTestActor::UploadCheckerboard(float InTime)
{
    if (!DisplayTexture || !DisplayTexture->GetResource())
        return;

    const int32 width  = TextureWidth;
    const int32 height = TextureHeight;

    TArray<uint8> pixels;
    pixels.SetNumUninitialized(width * height * 4);

    {
        SCOPE_CYCLE_COUNTER(STAT_RenderTest_PixelGen);

        uint8* dst = pixels.GetData();
        for (int32 y = 0; y < height; ++y)
        {
            for (int32 x = 0; x < width; ++x)
            {
                const float fx = static_cast<float>(x) / width;
                const float fy = static_cast<float>(y) / height;

                const float r = 0.5f + 0.5f * FMath::Sin(fx * 10.f + InTime * 2.f);
                const float g = 0.5f + 0.5f * FMath::Sin(fy * 10.f + InTime * 2.5f);
                const float b = 0.5f + 0.5f * FMath::Sin((fx + fy) * 8.f - InTime * 3.f);

                dst[0] = static_cast<uint8>(b * 255);
                dst[1] = static_cast<uint8>(g * 255);
                dst[2] = static_cast<uint8>(r * 255);
                dst[3] = 255;
                dst += 4;
            }
        }
    }

    FTextureResource* resource = DisplayTexture->GetResource();

    ENQUEUE_RENDER_COMMAND(UploadCheckerboard)(
        [resource, pixels = MoveTemp(pixels), width, height](FRHICommandListImmediate& RHICmdList)
        {
            SCOPE_CYCLE_COUNTER(STAT_RenderTest_RHIUpload);

            FRHITexture* rhi = resource->GetTextureRHI();
            if (!rhi)
                return;

            uint32 stride = 0;
            void* data = RHICmdList.LockTexture2D(rhi, 0, RLM_WriteOnly, stride, false);
            if (data)
                FMemory::Memcpy(data, pixels.GetData(), pixels.Num());
            RHICmdList.UnlockTexture2D(rhi, 0, false);
        }
    );
}