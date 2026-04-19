#include "Debug/CefWebSocketDebugCommands.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "Logging/CefWebSocketLog.h"
#include "Server/CefWebSocketServerBase.h"
#include "Subsystems/CefWebSocketSubsystem.h"

void FCefWebSocketDebugCommands::Startup()
{
	IConsoleManager& consoleManager = IConsoleManager::Get();

	if (!CommandList)
	{
		CommandList = consoleManager.RegisterConsoleCommand(
			TEXT("ws.list"),
			TEXT("List websocket servers."),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FCefWebSocketDebugCommands::HandleList));
	}

	if (!CommandStop)
	{
		CommandStop = consoleManager.RegisterConsoleCommand(
			TEXT("ws.stop"),
			TEXT("Stop websocket server by name: ws.stop <name>."),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FCefWebSocketDebugCommands::HandleStop));
	}

	if (!CommandKick)
	{
		CommandKick = consoleManager.RegisterConsoleCommand(
			TEXT("ws.kick"),
			TEXT("Disconnect websocket client: ws.kick <server> <clientId>."),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FCefWebSocketDebugCommands::HandleKick));
	}

	if (!CommandStats)
	{
		CommandStats = consoleManager.RegisterConsoleCommand(
			TEXT("ws.stats"),
			TEXT("Print websocket server stats: ws.stats <name>."),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FCefWebSocketDebugCommands::HandleStats));
	}
}

void FCefWebSocketDebugCommands::Shutdown()
{
	IConsoleManager& consoleManager = IConsoleManager::Get();

	if (CommandList)
	{
		consoleManager.UnregisterConsoleObject(CommandList);
		CommandList = nullptr;
	}
	if (CommandStop)
	{
		consoleManager.UnregisterConsoleObject(CommandStop);
		CommandStop = nullptr;
	}
	if (CommandKick)
	{
		consoleManager.UnregisterConsoleObject(CommandKick);
		CommandKick = nullptr;
	}
	if (CommandStats)
	{
		consoleManager.UnregisterConsoleObject(CommandStats);
		CommandStats = nullptr;
	}
}

UCefWebSocketSubsystem* FCefWebSocketDebugCommands::ResolveSubsystem() const
{
	if (!GEngine)
	{
		return nullptr;
	}

	for (const FWorldContext& worldContext : GEngine->GetWorldContexts())
	{
		UGameInstance* gameInstance = worldContext.OwningGameInstance;
		if (!gameInstance)
		{
			continue;
		}

		if (UCefWebSocketSubsystem* subsystem = gameInstance->GetSubsystem<UCefWebSocketSubsystem>())
		{
			return subsystem;
		}
	}

	return nullptr;
}

void FCefWebSocketDebugCommands::HandleList(const TArray<FString>& InArgs) const
{
	(void)InArgs;
	UCefWebSocketSubsystem* subsystem = ResolveSubsystem();
	if (!subsystem)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.list: subsystem not available"));
		return;
	}

	const TArray<UCefWebSocketServerBase*> servers = subsystem->GetAllServers();
	UE_LOG(LogCefWebSocketServer, Log, TEXT("ws.list: %d server(s)"), servers.Num());
	for (UCefWebSocketServerBase* server : servers)
	{
		if (!server)
		{
			continue;
		}
		const FCefWebSocketServerStats stats = server->GetStats();
		UE_LOG(
			LogCefWebSocketServer,
			Log,
			TEXT("  name=%s port=%d running=%s format=%d clients=%d queue=%lld inbound=%lld send=%lld write=%lld"),
			*server->GetServerNameId().ToString(),
			server->GetBoundPort(),
			server->IsRunning() ? TEXT("true") : TEXT("false"),
			static_cast<int32>(server->GetPayloadFormat()),
			stats.ActiveClients,
			stats.QueueDepth,
			stats.InInboundQueueDepth,
			stats.InSendQueueDepth,
			stats.InWriteQueueDepth);
	}
}

void FCefWebSocketDebugCommands::HandleStop(const TArray<FString>& InArgs) const
{
	if (InArgs.Num() < 1)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.stop usage: ws.stop <name>"));
		return;
	}

	UCefWebSocketSubsystem* subsystem = ResolveSubsystem();
	if (!subsystem)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.stop: subsystem not available"));
		return;
	}

	const FName nameId(*InArgs[0]);
	const bool bStopped = subsystem->StopServer(nameId);
	UE_LOG(LogCefWebSocketServer, Log, TEXT("ws.stop: name=%s stopped=%s"), *nameId.ToString(), bStopped ? TEXT("true") : TEXT("false"));
}

void FCefWebSocketDebugCommands::HandleKick(const TArray<FString>& InArgs) const
{
	if (InArgs.Num() < 2)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.kick usage: ws.kick <server> <clientId>"));
		return;
	}

	UCefWebSocketSubsystem* subsystem = ResolveSubsystem();
	if (!subsystem)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.kick: subsystem not available"));
		return;
	}

	const FName nameId(*InArgs[0]);
	UCefWebSocketServerBase* server = subsystem->GetServer(nameId);
	if (!server)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.kick: server not found for '%s'"), *nameId.ToString());
		return;
	}

	int64 clientId = 0;
	if (!LexTryParseString(clientId, *InArgs[1]))
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.kick: invalid client id '%s'"), *InArgs[1]);
		return;
	}

	const ECefWebSocketSendResult result = server->DisconnectClient(clientId, ECefWebSocketCloseReason::Kicked);
	UE_LOG(LogCefWebSocketServer, Log, TEXT("ws.kick: server=%s client=%lld result=%d"), *nameId.ToString(), clientId, static_cast<int32>(result));
}

void FCefWebSocketDebugCommands::HandleStats(const TArray<FString>& InArgs) const
{
	if (InArgs.Num() < 1)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.stats usage: ws.stats <name>"));
		return;
	}

	UCefWebSocketSubsystem* subsystem = ResolveSubsystem();
	if (!subsystem)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.stats: subsystem not available"));
		return;
	}

	const FName nameId(*InArgs[0]);
	UCefWebSocketServerBase* server = subsystem->GetServer(nameId);
	if (!server)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.stats: server not found for '%s'"), *nameId.ToString());
		return;
	}

	const FCefWebSocketServerStats stats = server->GetStats();
	UE_LOG(
		LogCefWebSocketServer,
		Log,
		TEXT("ws.stats: name=%s format=%d clients=%d rx=%lld tx=%lld rx_s=%.2f tx_s=%.2f dropped=%lld queue=%lld avg_queue=%.2f inbound=%lld handle=%lld send=%lld write=%lld"),
		*nameId.ToString(),
		static_cast<int32>(server->GetPayloadFormat()),
		stats.ActiveClients,
		stats.RxBytes,
		stats.TxBytes,
		stats.RxBytesPerSec,
		stats.TxBytesPerSec,
		stats.DroppedMessages,
		stats.QueueDepth,
		stats.AvgQueueDepth,
		stats.InInboundQueueDepth,
		stats.InHandleQueueDepth,
		stats.InSendQueueDepth,
		stats.InWriteQueueDepth);
}
