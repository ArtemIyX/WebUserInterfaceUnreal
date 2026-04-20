/**
 * @file CefContentHttpServer/Private/Services/CefContentImageEncodeService.cpp
 * @brief Texture to PNG byte encoding implementation.
 */
#include "Services/CefContentImageEncodeService.h"

#include "CefContentHttpServer.h"
#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"

bool FCefContentImageEncodeService::EncodeTextureToPngBytes(UTexture2D* InTexture, TArray<uint8>& OutPngBytes, FString& OutError) const
{
	OutPngBytes.Reset();
	if (!InTexture)
	{
		OutError = TEXT("Texture is null");
		UE_LOG(LogCefContentHttpServer, Error, TEXT("EncodeTextureToPngBytes failed: texture is null"));
		return false;
	}

	FTexturePlatformData* const platformData = InTexture->GetPlatformData();
	if (!platformData || platformData->Mips.Num() <= 0)
	{
		OutError = TEXT("Texture has no platform data");
		UE_LOG(LogCefContentHttpServer, Error, TEXT("EncodeTextureToPngBytes failed: '%s' has no platform data"), *InTexture->GetName());
		return false;
	}

	FTexture2DMipMap& mip = platformData->Mips[0];
	const int32 width = mip.SizeX;
	const int32 height = mip.SizeY;
	if (width <= 0 || height <= 0)
	{
		OutError = TEXT("Texture dimensions are invalid");
		UE_LOG(LogCefContentHttpServer, Error, TEXT("EncodeTextureToPngBytes failed: '%s' invalid size %dx%d"), *InTexture->GetName(), width, height);
		return false;
	}

	const void* const mipData = mip.BulkData.LockReadOnly();
	if (!mipData)
	{
		OutError = TEXT("Failed to lock texture mip data");
		UE_LOG(LogCefContentHttpServer, Error, TEXT("EncodeTextureToPngBytes failed: cannot lock mip data for '%s'"), *InTexture->GetName());
		return false;
	}

	const int64 byteCount = static_cast<int64>(width) * static_cast<int64>(height) * 4;
	TArray<uint8> rawPixels;
	rawPixels.SetNumUninitialized(static_cast<int32>(byteCount));
	FMemory::Memcpy(rawPixels.GetData(), mipData, rawPixels.Num());
	mip.BulkData.Unlock();

	IImageWrapperModule& imageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	const TSharedPtr<IImageWrapper> imageWrapper = imageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	if (!imageWrapper.IsValid())
	{
		OutError = TEXT("Failed to create PNG wrapper");
		UE_LOG(LogCefContentHttpServer, Error, TEXT("EncodeTextureToPngBytes failed: png wrapper is invalid"));
		return false;
	}

	if (!imageWrapper->SetRaw(rawPixels.GetData(), rawPixels.Num(), width, height, ERGBFormat::BGRA, 8))
	{
		OutError = TEXT("SetRaw failed for PNG wrapper");
		UE_LOG(LogCefContentHttpServer, Error, TEXT("EncodeTextureToPngBytes failed: SetRaw failed for '%s'"), *InTexture->GetName());
		return false;
	}

	OutPngBytes = imageWrapper->GetCompressed();
	if (OutPngBytes.Num() <= 0)
	{
		OutError = TEXT("PNG compression produced no data");
		UE_LOG(LogCefContentHttpServer, Error, TEXT("EncodeTextureToPngBytes failed: no output bytes for '%s'"), *InTexture->GetName());
		return false;
	}

	UE_LOG(LogCefContentHttpServer, Verbose, TEXT("Encoded texture '%s' to png bytes: %d"), *InTexture->GetName(), OutPngBytes.Num());
	return true;
}
