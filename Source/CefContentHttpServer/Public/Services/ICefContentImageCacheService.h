#pragma once

#include "CoreMinimal.h"

class UTexture2D;

class CEFCONTENTHTTPSERVER_API ICefContentImageCacheService
{
public:
    virtual ~ICefContentImageCacheService() = default;

    virtual bool GetImageByPackagePath(const FString& InPackagePath, UTexture2D*& OutImage) = 0;
    virtual int32 GetCachedImageCount() const = 0;
    virtual void ClearCache() = 0;
};
