/**
 * @file CefContentHttpServer/Private/CefContentHttpServer.cpp
 * @brief Module lifecycle and service creation/destruction.
 */
#include "CefContentHttpServer.h"

#include "Services/CefContentImageCacheService.h"
#include "Services/CefContentImageEncodeService.h"
#include "Services/CefContentHttpServerRuntimeService.h"
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
	ImageEncoder = MakeShared<FCefContentImageEncodeService>();
	HttpServerService = MakeShared<FCefContentHttpServerRuntimeService>();
	SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, 0);
	UE_LOG(LogCefContentHttpServer, Log, TEXT("Module services created"));
}

void FCefContentHttpServerModule::ShutdownModule()
{
	UE_LOG(LogCefContentHttpServer, Log, TEXT("CefContentHttpServer module shutdown"));
	if (HttpServerService.IsValid())
	{
		HttpServerService->Stop();
		HttpServerService.Reset();
		UE_LOG(LogCefContentHttpServer, Log, TEXT("Http server service destroyed"));
	}
	if (ImageCacher.IsValid())
	{
		ImageCacher->ClearCache();
		ImageCacher.Reset();
		UE_LOG(LogCefContentHttpServer, Log, TEXT("Image cache service destroyed"));
	}
	if (ImageEncoder.IsValid())
	{
		ImageEncoder.Reset();
		UE_LOG(LogCefContentHttpServer, Log, TEXT("Image encode service destroyed"));
	}
	SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, 0);
}

FCefContentImageCacheService* FCefContentHttpServerModule::GetImageCacher() const
{
	return ImageCacher.Get();
}

FCefContentImageEncodeService* FCefContentHttpServerModule::GetImageEncoder() const
{
	return ImageEncoder.Get();
}

FCefContentHttpServerRuntimeService* FCefContentHttpServerModule::GetHttpServerService() const
{
	return HttpServerService.Get();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCefContentHttpServerModule, CefContentHttpServer)
