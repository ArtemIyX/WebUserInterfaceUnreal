#pragma once

#include "CoreMinimal.h"
#include "Services/ICefContentImageCacheService.h"

class FCefContentImageCacheService final : public ICefContentImageCacheService
{
public:
	virtual bool GetImageByPackagePath(const FString& InPackagePath, UTexture2D*& OutImage) override;
	virtual int32 GetCachedImageCount() const override;
	virtual void ClearCache() override;

private:
	FString NormalizeAssetPath(const FString& InPackagePath) const;

private:
	mutable FCriticalSection CacheMutex;
	TMap<FString, TStrongObjectPtr<UTexture2D>> CachedImagesByPath;
};
