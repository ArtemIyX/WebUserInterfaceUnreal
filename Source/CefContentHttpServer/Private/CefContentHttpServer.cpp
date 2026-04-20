#include "CefContentHttpServer.h"

#include "Services/CefContentImageCacheService.h"
#include "Stats/CefContentHttpServerStats.h"

#define LOCTEXT_NAMESPACE "FCefContentHttpServerModule"

DEFINE_LOG_CATEGORY(LogCefContentHttpServer);

FCefContentHttpServerModule& FCefContentHttpServerModule::Get()
{
	return FModuleManager::LoadModuleChecked<FCefContentHttpServerModule>(TEXT("CefContentHttpServer"));
}

bool FCefContentHttpServerModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded(TEXT("CefContentHttpServer"));
}

void FCefContentHttpServerModule::StartupModule()
{
	UE_LOG(LogCefContentHttpServer, Log, TEXT("CefContentHttpServer module startup"));
	ImageCacher = MakeShared<FCefContentImageCacheService>();
	SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, 0);
	UE_LOG(LogCefContentHttpServer, Log, TEXT("Image cacher created"));
}

void FCefContentHttpServerModule::ShutdownModule()
{
	UE_LOG(LogCefContentHttpServer, Log, TEXT("CefContentHttpServer module shutdown"));
	if (ImageCacher.IsValid())
	{
		ImageCacher->ClearCache();
		ImageCacher.Reset();
		UE_LOG(LogCefContentHttpServer, Log, TEXT("Image cacher destroyed"));
	}
	SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, 0);
}

FCefContentImageCacheService* FCefContentHttpServerModule::GetImageCacher() const
{
	return ImageCacher.Get();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCefContentHttpServerModule, CefContentHttpServer)
