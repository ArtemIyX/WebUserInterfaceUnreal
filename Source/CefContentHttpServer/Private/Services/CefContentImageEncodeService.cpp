/**
 * @file CefContentHttpServer/Private/Services/CefContentImageEncodeService.cpp
 * @brief Texture to PNG byte encoding implementation.
 */
#include "Services/CefContentImageEncodeService.h"

#include "CefContentHttpServer.h"
#include "Async/Async.h"
#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Services/CefContentImageCacheService.h"

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

void FCefContentImageEncodeService::RequestPngBytesByAssetPathAsync(const FString& InAssetPath, FOnEncodedImageReady InOnCompleted)
{
	FString normalizedPath = InAssetPath;
	normalizedPath.TrimStartAndEndInline();
	if (normalizedPath.IsEmpty())
	{
		InOnCompleted(false, TArray<uint8>(), TEXT("Asset path is empty"));
		return;
	}

	{
		FScopeLock lock(&EncodedCacheMutex);
		if (const FEncodedImageEntry* cachedEntry = EncodedImageCacheByPath.Find(normalizedPath))
		{
			InOnCompleted(true, cachedEntry->Bytes, FString());
			return;
		}

		if (TArray<FOnEncodedImageReady>* inFlightCallbacks = InFlightRequestsByPath.Find(normalizedPath))
		{
			inFlightCallbacks->Add(MoveTemp(InOnCompleted));
			return;
		}

		TArray<FOnEncodedImageReady> callbacks;
		callbacks.Add(MoveTemp(InOnCompleted));
		InFlightRequestsByPath.Add(normalizedPath, MoveTemp(callbacks));
	}

	AsyncTask(ENamedThreads::GameThread, [this, normalizedPath]() {
		if (!FCefContentHttpServerModule::IsAvailable())
		{
			CompletePendingRequests(normalizedPath, false, TArray<uint8>(), TEXT("Module is not available"));
			return;
		}

		FCefContentImageCacheService* const imageCacher = FCefContentHttpServerModule::Get().GetImageCacher();
		if (!imageCacher)
		{
			CompletePendingRequests(normalizedPath, false, TArray<uint8>(), TEXT("Image cache service is unavailable"));
			return;
		}

		UTexture2D* image = nullptr;
		if (!imageCacher->GetImageByPackagePath(normalizedPath, image) || !image)
		{
			CompletePendingRequests(normalizedPath, false, TArray<uint8>(), FString::Printf(TEXT("Image not found: '%s'"), *normalizedPath));
			return;
		}

		Async(EAsyncExecution::ThreadPool, [this, normalizedPath, image]() {
			TArray<uint8> pngBytes;
			FString error;
			const bool bSuccess = EncodeTextureToPngBytes(image, pngBytes, error);
			if (!bSuccess)
			{
				CompletePendingRequests(normalizedPath, false, TArray<uint8>(), error);
				return;
			}

			{
				FScopeLock lock(&EncodedCacheMutex);
				FEncodedImageEntry& cachedEntry = EncodedImageCacheByPath.FindOrAdd(normalizedPath);
				cachedEntry.Bytes = pngBytes;
			}

			CompletePendingRequests(normalizedPath, true, pngBytes, FString());
		});
	});
}

void FCefContentImageEncodeService::ClearEncodedCache()
{
	FScopeLock lock(&EncodedCacheMutex);
	EncodedImageCacheByPath.Reset();
	InFlightRequestsByPath.Reset();
}

int32 FCefContentImageEncodeService::GetEncodedCacheCount() const
{
	FScopeLock lock(&EncodedCacheMutex);
	return EncodedImageCacheByPath.Num();
}

void FCefContentImageEncodeService::CompletePendingRequests(const FString& InAssetPath, bool bInSuccess, const TArray<uint8>& InBytes, const FString& InError)
{
	TArray<FOnEncodedImageReady> callbacks;
	{
		FScopeLock lock(&EncodedCacheMutex);
		TArray<FOnEncodedImageReady>* pendingCallbacks = InFlightRequestsByPath.Find(InAssetPath);
		if (!pendingCallbacks)
		{
			return;
		}
		callbacks = MoveTemp(*pendingCallbacks);
		InFlightRequestsByPath.Remove(InAssetPath);
	}

	for (FOnEncodedImageReady& callback : callbacks)
	{
		callback(bInSuccess, InBytes, InError);
	}
}
