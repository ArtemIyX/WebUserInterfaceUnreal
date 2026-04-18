#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FCefWebSocketServerModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
