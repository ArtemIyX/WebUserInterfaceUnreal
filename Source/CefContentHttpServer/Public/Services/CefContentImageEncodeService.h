/**
 * @file CefContentHttpServer/Public/Services/CefContentImageEncodeService.h
 * @brief Texture encoding utilities.
 */
#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/** @brief Encodes textures into HTTP-friendly binary payloads. */
class CEFCONTENTHTTPSERVER_API FCefContentImageEncodeService final
{
public:
	/** @brief Async completion callback for encoded bytes request. */
	using FOnEncodedImageReady = TFunction<void(bool, const TArray<uint8>&, const FString&)>;

	/**
	 * @brief Encodes texture into PNG bytes.
	 * @param InTexture Texture to encode.
	 * @param OutPngBytes Encoded PNG bytes.
	 * @param OutError Error text on failure.
	 * @return True on success.
	 */
	bool EncodeTextureToPngBytes(UTexture2D* InTexture, TArray<uint8>& OutPngBytes, FString& OutError) const;

	/**
	 * @brief Retrieves PNG bytes for asset path asynchronously with dedupe and cache.
	 * @param InAssetPath Asset path.
	 * @param InOnCompleted Completion callback.
	 */
	void RequestPngBytesByAssetPathAsync(const FString& InAssetPath, FOnEncodedImageReady InOnCompleted);

	/** @brief Clears encoded-bytes cache and in-flight request state. */
	void ClearEncodedCache();

	/** @brief Returns number of encoded images cached in memory. */
	int32 GetEncodedCacheCount() const;

private:
	struct FEncodedImageEntry
	{
		TArray<uint8> Bytes;
	};

	void CompletePendingRequests(const FString& InAssetPath, bool bInSuccess, const TArray<uint8>& InBytes, const FString& InError);

private:
	mutable FCriticalSection EncodedCacheMutex;
	TMap<FString, FEncodedImageEntry> EncodedImageCacheByPath;
	TMap<FString, TArray<FOnEncodedImageReady>> InFlightRequestsByPath;
};
