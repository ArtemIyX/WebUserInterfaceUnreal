/**
 * @file CefWebSocketServer\Public\CefWebSocketServer.h
 * @brief Declares CefWebSocketServer for module CefWebSocketServer\Public\CefWebSocketServer.h.
 * @details Contains websocket server components used by the plugin runtime and gameplay-facing systems.
 */
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

