/**
 * @file CefContentHttpServer/Public/CefContentHttpServer.h
 * @brief Module entry and shared service accessors.
 */
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/** @brief Log category for CefContentHttpServer. */
CEFCONTENTHTTPSERVER_API DECLARE_LOG_CATEGORY_EXTERN(LogCefContentHttpServer, Log, All);

/** @brief Module root for content HTTP services. */
class FCefContentHttpServerModule : public IModuleInterface
{
public:
	/** @brief Loads and returns module singleton. */
	static FCefContentHttpServerModule& Get();
	/** @brief Returns true if module is currently loaded. */
	static bool IsAvailable();

	/** @brief Creates module-owned services. */
	virtual void StartupModule() override;
	/** @brief Stops and releases module-owned services. */
	virtual void ShutdownModule() override;

	/** @brief Returns module image cache service, may be null during shutdown. */
	class FCefContentImageCacheService* GetImageCacher() const;
	/** @brief Returns module image encoder service, may be null during shutdown. */
	class FCefContentImageEncodeService* GetImageEncoder() const;
	/** @brief Returns module HTTP runtime service, may be null during shutdown. */
	class FCefContentHttpServerRuntimeService* GetHttpServerService() const;

private:
	TSharedPtr<class FCefContentImageCacheService> ImageCacher;
	TSharedPtr<class FCefContentImageEncodeService> ImageEncoder;
	TSharedPtr<class FCefContentHttpServerRuntimeService> HttpServerService;
};
