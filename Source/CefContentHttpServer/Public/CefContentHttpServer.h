#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

CEFCONTENTHTTPSERVER_API DECLARE_LOG_CATEGORY_EXTERN(LogCefContentHttpServer, Log, All);

class FCefContentHttpServerModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
