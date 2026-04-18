#include "CefWebSocketServer.h"

#include "Debug/CefWebSocketDebugCommands.h"
#include "Logging/CefWebSocketLog.h"

void FCefWebSocketServerModule::StartupModule()
{
	DebugCommands = MakeUnique<FCefWebSocketDebugCommands>();
	DebugCommands->Startup();
	UE_LOG(LogCefWebSocket, Log, TEXT("CefWebSocketServer module started"));
}

void FCefWebSocketServerModule::ShutdownModule()
{
	if (DebugCommands)
	{
		DebugCommands->Shutdown();
		DebugCommands.Reset();
	}
	UE_LOG(LogCefWebSocket, Log, TEXT("CefWebSocketServer module shutdown"));
}

IMPLEMENT_MODULE(FCefWebSocketServerModule, CefWebSocketServer)
