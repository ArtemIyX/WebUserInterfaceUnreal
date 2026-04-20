#include "Services/CefContentImageCacheService.h"

#include "CefContentHttpServer.h"
#include "Misc/PackageName.h"
#include "Stats/CefContentHttpServerStats.h"
#include "UObject/SoftObjectPath.h"
#include "Engine/Texture2D.h"

FString FCefContentImageCacheService::NormalizeAssetPath(const FString& InPackagePath) const
{
    FString AssetPath = InPackagePath.TrimStartAndEnd();
    if (AssetPath.IsEmpty())
    {
        return AssetPath;
    }

    if (!AssetPath.Contains(TEXT(".")))
    {
        const FString AssetName = FPackageName::GetShortName(AssetPath);
        if (!AssetName.IsEmpty())
        {
            AssetPath = FString::Printf(TEXT("%s.%s"), *AssetPath, *AssetName);
        }
    }

    return AssetPath;
}

bool FCefContentImageCacheService::GetImageByPackagePath(const FString& InPackagePath, UTexture2D*& OutImage)
{
    OutImage = nullptr;
    const FString NormalizedPath = NormalizeAssetPath(InPackagePath);
    if (NormalizedPath.IsEmpty())
    {
        UE_LOG(LogCefContentHttpServer, Error, TEXT("GetImageByPackagePath failed: input path is empty"));
        return false;
    }

    {
        FScopeLock Lock(&CacheMutex);
        if (const TStrongObjectPtr<UTexture2D>* CachedImage = CachedImagesByPath.Find(NormalizedPath))
        {
            if (CachedImage->IsValid())
            {
                OutImage = CachedImage->Get();
                UE_LOG(LogCefContentHttpServer, Verbose, TEXT("Cache hit for '%s'"), *NormalizedPath);
                return true;
            }

            UE_LOG(LogCefContentHttpServer, Warning, TEXT("Stale cache entry for '%s', removing"), *NormalizedPath);
            CachedImagesByPath.Remove(NormalizedPath);
            SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, static_cast<uint32>(CachedImagesByPath.Num()));
        }
    }

    FSoftObjectPath SoftPath(NormalizedPath);
    UE_LOG(LogCefContentHttpServer, Verbose, TEXT("Cache miss for '%s'. Loading asset"), *NormalizedPath);
    if (!SoftPath.IsValid())
    {
        UE_LOG(LogCefContentHttpServer, Error, TEXT("Invalid image asset path '%s'"), *NormalizedPath);
        return false;
    }

    UObject* const LoadedObject = SoftPath.TryLoad();
    if (!LoadedObject)
    {
        UE_LOG(LogCefContentHttpServer, Error, TEXT("Failed to load image from path '%s'"), *NormalizedPath);
        return false;
    }

    UTexture2D* const LoadedTexture = Cast<UTexture2D>(LoadedObject);
    if (!LoadedTexture)
    {
        UE_LOG(LogCefContentHttpServer, Error, TEXT("Loaded asset '%s' is not UTexture2D (actual class: %s)"), *NormalizedPath, *LoadedObject->GetClass()->GetName());
        return false;
    }

    {
        FScopeLock Lock(&CacheMutex);
        CachedImagesByPath.Add(NormalizedPath, TStrongObjectPtr<UTexture2D>(LoadedTexture));
        SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, static_cast<uint32>(CachedImagesByPath.Num()));
        UE_LOG(LogCefContentHttpServer, Log, TEXT("Cached image '%s'. Cached images: %d"), *NormalizedPath, CachedImagesByPath.Num());
    }

    OutImage = LoadedTexture;
    return true;
}

int32 FCefContentImageCacheService::GetCachedImageCount() const
{
    FScopeLock Lock(&CacheMutex);
    return CachedImagesByPath.Num();
}

void FCefContentImageCacheService::ClearCache()
{
    FScopeLock Lock(&CacheMutex);
    const int32 NumCachedBeforeClear = CachedImagesByPath.Num();
    CachedImagesByPath.Reset();
    SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, 0);
    UE_LOG(LogCefContentHttpServer, Log, TEXT("Image cache cleared. Removed %d cached images"), NumCachedBeforeClear);
}
