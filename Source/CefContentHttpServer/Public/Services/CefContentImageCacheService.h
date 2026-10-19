/**
 * @file CefContentHttpServer/Public/Services/CefContentImageCacheService.h
 * @brief Image asset cache service.
 */
#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/** @brief Caches textures loaded from package/object paths. */
class CEFCONTENTHTTPSERVER_API FCefContentImageCacheService final
{
public:
	/**
	 * @brief Loads image by asset path and caches it.
	 * @param InPackagePath Asset path string.
	 * @param OutImage Loaded texture pointer.
	 * @return True when image is loaded or found in cache.
	 */
	bool GetImageByPackagePath(const FString& InPackagePath, UTexture2D*& OutImage);
	/** @brief Returns cached image count. */
	int32 GetCachedImageCount() const;
	/** @brief Clears cached images. */
	void ClearCache();

private:
	/** @brief Normalizes package path to object path format. */
	FString NormalizeAssetPath(const FString& InPackagePath) const;

private:
	mutable FCriticalSection CacheMutex;
	TMap<FString, TStrongObjectPtr<UTexture2D>> CachedImagesByPath;
};
