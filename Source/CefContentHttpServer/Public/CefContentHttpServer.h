#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

CEFCONTENTHTTPSERVER_API DECLARE_LOG_CATEGORY_EXTERN(LogCefContentHttpServer, Log, All);

class FCefContentHttpServerModule : public IModuleInterface
{
public:
	static FCefContentHttpServerModule& Get();
	static bool IsAvailable();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	class FCefContentImageCacheService* GetImageCacher() const;
	class FCefContentImageEncodeService* GetImageEncoder() const;
	class FCefContentHttpServerRuntimeService* GetHttpServerService() const;

private:
	TSharedPtr<class FCefContentImageCacheService> ImageCacher;
	TSharedPtr<class FCefContentImageEncodeService> ImageEncoder;
	TSharedPtr<class FCefContentHttpServerRuntimeService> HttpServerService;
};
