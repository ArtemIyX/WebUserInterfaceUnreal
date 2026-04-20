#include "CefContentHttpServer.h"

#include "Stats/CefContentHttpServerStats.h"

#define LOCTEXT_NAMESPACE "FCefContentHttpServerModule"

DEFINE_LOG_CATEGORY(LogCefContentHttpServer);

void FCefContentHttpServerModule::StartupModule()
{
    UE_LOG(LogCefContentHttpServer, Log, TEXT("CefContentHttpServer module startup"));
    SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, 0);
}

void FCefContentHttpServerModule::ShutdownModule()
{
    UE_LOG(LogCefContentHttpServer, Log, TEXT("CefContentHttpServer module shutdown"));
    SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, 0);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCefContentHttpServerModule, CefContentHttpServer)
