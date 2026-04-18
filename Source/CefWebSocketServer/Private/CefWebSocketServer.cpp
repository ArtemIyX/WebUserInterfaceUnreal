#include "CefWebSocketServer.h"

#include "Logging/CefWebSocketLog.h"

void FCefWebSocketServerModule::StartupModule()
{
	UE_LOG(LogCefWebSocket, Log, TEXT("CefWebSocketServer module started"));
}

void FCefWebSocketServerModule::ShutdownModule()
{
	UE_LOG(LogCefWebSocket, Log, TEXT("CefWebSocketServer module shutdown"));
}

IMPLEMENT_MODULE(FCefWebSocketServerModule, CefWebSocketServer)
