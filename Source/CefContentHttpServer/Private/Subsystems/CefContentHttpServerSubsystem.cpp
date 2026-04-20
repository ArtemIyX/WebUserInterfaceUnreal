#include "Subsystems/CefContentHttpServerSubsystem.h"

#include "CefContentHttpServer.h"
#include "Services/CefContentImageCacheService.h"
#include "Stats/CefContentHttpServerStats.h"

void UCefContentHttpServerSubsystem::Deinitialize()
{
	UE_LOG(LogCefContentHttpServer, Log, TEXT("CefContentHttpServer subsystem deinitialize"));
	Super::Deinitialize();
}

UTexture2D* UCefContentHttpServerSubsystem::GetImageByPackagePath(const FString& InPackagePath, bool& bOutSuccess)
{
	bOutSuccess = false;
	if (!FCefContentHttpServerModule::IsAvailable())
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("GetImageByPackagePath failed: module is not available"));
		return nullptr;
	}

	FCefContentImageCacheService* const imageCacher = FCefContentHttpServerModule::Get().GetImageCacher();
	if (!imageCacher)
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("GetImageByPackagePath failed: image cacher is not available"));
		return nullptr;
	}

	UTexture2D* image = nullptr;
	bOutSuccess = imageCacher->GetImageByPackagePath(InPackagePath, image);
	if (!bOutSuccess)
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("GetImageByPackagePath failed for '%s'"), *InPackagePath);
	}
	return image;
}

void UCefContentHttpServerSubsystem::ClearCachedImages()
{
	if (!FCefContentHttpServerModule::IsAvailable())
	{
		UE_LOG(LogCefContentHttpServer, Warning, TEXT("ClearCachedImages skipped: module is not available"));
		SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, 0);
		return;
	}

	FCefContentImageCacheService* const imageCacher = FCefContentHttpServerModule::Get().GetImageCacher();
	if (!imageCacher)
	{
		UE_LOG(LogCefContentHttpServer, Warning, TEXT("ClearCachedImages skipped: image cacher is not available"));
		SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, 0);
		return;
	}

	imageCacher->ClearCache();
}

int32 UCefContentHttpServerSubsystem::GetCachedImageCount() const
{
	if (!FCefContentHttpServerModule::IsAvailable())
	{
		return 0;
	}

	FCefContentImageCacheService* const imageCacher = FCefContentHttpServerModule::Get().GetImageCacher();
	if (!imageCacher)
	{
		return 0;
	}

	return imageCacher->GetCachedImageCount();
}
