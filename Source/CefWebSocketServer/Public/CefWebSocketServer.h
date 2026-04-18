#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FCefWebSocketServerModule : public IModuleInterface
{
public:
#pragma region Lifecycle
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
#pragma endregion

private:
#pragma region InternalState
	TUniquePtr<class FCefWebSocketDebugCommands> DebugCommands;
#pragma endregion
};
