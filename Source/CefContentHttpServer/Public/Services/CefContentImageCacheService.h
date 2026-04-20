#pragma once

#include "CoreMinimal.h"

class UTexture2D;

class CEFCONTENTHTTPSERVER_API FCefContentImageCacheService final
{
public:
	bool GetImageByPackagePath(const FString& InPackagePath, UTexture2D*& OutImage);
	int32 GetCachedImageCount() const;
	void ClearCache();

private:
	FString NormalizeAssetPath(const FString& InPackagePath) const;

private:
	mutable FCriticalSection CacheMutex;
	TMap<FString, TStrongObjectPtr<UTexture2D>> CachedImagesByPath;
};
