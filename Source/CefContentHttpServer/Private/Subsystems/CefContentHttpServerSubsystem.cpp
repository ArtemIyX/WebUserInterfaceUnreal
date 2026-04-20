#include "Subsystems/CefContentHttpServerSubsystem.h"

#include "CefContentHttpServer.h"
#include "Services/CefContentImageCacheService.h"
#include "Services/ICefContentImageCacheService.h"
#include "Stats/CefContentHttpServerStats.h"

UCefContentHttpServerSubsystem::UCefContentHttpServerSubsystem()
{
}

void UCefContentHttpServerSubsystem::Initialize(FSubsystemCollectionBase& InCollection)
{
	Super::Initialize(InCollection);
}

void UCefContentHttpServerSubsystem::Deinitialize()
{
	UE_LOG(LogCefContentHttpServer, Log, TEXT("CefContentHttpServer subsystem deinitialize"));
	if (ImageCacher.IsValid())
	{
		ImageCacher->ClearCache();
		ImageCacher.Reset();
		UE_LOG(LogCefContentHttpServer, Log, TEXT("Image cacher released"));
	}
	SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, 0);

	Super::Deinitialize();
}

void UCefContentHttpServerSubsystem::SetImageCacher(TSharedPtr<ICefContentImageCacheService> InImageCacher)
{
	if (!InImageCacher.IsValid())
	{
		UE_LOG(LogCefContentHttpServer, Warning, TEXT("SetImageCacher called with invalid cacher"));
		if (ImageCacher.IsValid())
		{
			ImageCacher->ClearCache();
		}
		ImageCacher.Reset();
		SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, 0);
		return;
	}

	if (ImageCacher.IsValid() && ImageCacher.Get() != InImageCacher.Get())
	{
		UE_LOG(LogCefContentHttpServer, Log, TEXT("Replacing existing image cacher"));
		ImageCacher->ClearCache();
	}

	ImageCacher = MoveTemp(InImageCacher);
	const int32 cachedCount = ImageCacher->GetCachedImageCount();
	SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, static_cast<uint32>(FMath::Max(cachedCount, 0)));
	UE_LOG(LogCefContentHttpServer, Log, TEXT("Image cacher set. Current cached images: %d"), cachedCount);
}

const TSharedPtr<ICefContentImageCacheService>& UCefContentHttpServerSubsystem::GetImageCacher() const
{
	return ImageCacher;
}

void UCefContentHttpServerSubsystem::InitDefaultImageCacher()
{
	UE_LOG(LogCefContentHttpServer, Log, TEXT("InitDefaultImageCacher called"));
	SetImageCacher(MakeShared<FCefContentImageCacheService>());
}

UTexture2D* UCefContentHttpServerSubsystem::GetImageByPackagePath(const FString& InPackagePath, bool& bOutSuccess)
{
	bOutSuccess = false;
	if (!ImageCacher.IsValid())
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("GetImageByPackagePath failed: image cacher is not initialized"));
		return nullptr;
	}

	UTexture2D* image = nullptr;
	bOutSuccess = ImageCacher->GetImageByPackagePath(InPackagePath, image);
	if (!bOutSuccess)
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("GetImageByPackagePath failed for '%s'"), *InPackagePath);
	}
	return image;
}

void UCefContentHttpServerSubsystem::ClearCachedImages()
{
	if (!ImageCacher.IsValid())
	{
		UE_LOG(LogCefContentHttpServer, Warning, TEXT("ClearCachedImages skipped: image cacher is not initialized"));
		SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, 0);
		return;
	}

	ImageCacher->ClearCache();
	SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, 0);
}

int32 UCefContentHttpServerSubsystem::GetCachedImageCount() const
{
	if (!ImageCacher.IsValid())
	{
		return 0;
	}

	return ImageCacher->GetCachedImageCount();
}
