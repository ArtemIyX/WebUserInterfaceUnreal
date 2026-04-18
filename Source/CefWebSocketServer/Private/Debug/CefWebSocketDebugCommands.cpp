#include "Debug/CefWebSocketDebugCommands.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "Logging/CefWebSocketLog.h"
#include "Server/CefWebSocketServerBase.h"
#include "Subsystems/CefWebSocketSubsystem.h"

void FCefWebSocketDebugCommands::Startup()
{
	IConsoleManager& ConsoleManager = IConsoleManager::Get();

	if (!CommandList)
	{
		CommandList = ConsoleManager.RegisterConsoleCommand(
			TEXT("ws.list"),
			TEXT("List websocket servers."),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FCefWebSocketDebugCommands::HandleList));
	}

	if (!CommandStop)
	{
		CommandStop = ConsoleManager.RegisterConsoleCommand(
			TEXT("ws.stop"),
			TEXT("Stop websocket server by name: ws.stop <name>."),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FCefWebSocketDebugCommands::HandleStop));
	}

	if (!CommandKick)
	{
		CommandKick = ConsoleManager.RegisterConsoleCommand(
			TEXT("ws.kick"),
			TEXT("Disconnect websocket client: ws.kick <server> <clientId>."),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FCefWebSocketDebugCommands::HandleKick));
	}

	if (!CommandStats)
	{
		CommandStats = ConsoleManager.RegisterConsoleCommand(
			TEXT("ws.stats"),
			TEXT("Print websocket server stats: ws.stats <name>."),
			FConsoleCommandWithArgsDelegate::CreateRaw(this, &FCefWebSocketDebugCommands::HandleStats));
	}
}

void FCefWebSocketDebugCommands::Shutdown()
{
	IConsoleManager& ConsoleManager = IConsoleManager::Get();

	if (CommandList)
	{
		ConsoleManager.UnregisterConsoleObject(CommandList);
		CommandList = nullptr;
	}
	if (CommandStop)
	{
		ConsoleManager.UnregisterConsoleObject(CommandStop);
		CommandStop = nullptr;
	}
	if (CommandKick)
	{
		ConsoleManager.UnregisterConsoleObject(CommandKick);
		CommandKick = nullptr;
	}
	if (CommandStats)
	{
		ConsoleManager.UnregisterConsoleObject(CommandStats);
		CommandStats = nullptr;
	}
}

UCefWebSocketSubsystem* FCefWebSocketDebugCommands::ResolveSubsystem() const
{
	if (!GEngine)
	{
		return nullptr;
	}

	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		UGameInstance* GameInstance = WorldContext.OwningGameInstance;
		if (!GameInstance)
		{
			continue;
		}

		if (UCefWebSocketSubsystem* Subsystem = GameInstance->GetSubsystem<UCefWebSocketSubsystem>())
		{
			return Subsystem;
		}
	}

	return nullptr;
}

void FCefWebSocketDebugCommands::HandleList(const TArray<FString>& Args) const
{
	(void)Args;
	UCefWebSocketSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.list: subsystem not available"));
		return;
	}

	const TArray<UCefWebSocketServerBase*> Servers = Subsystem->GetAllServers();
	UE_LOG(LogCefWebSocketServer, Log, TEXT("ws.list: %d server(s)"), Servers.Num());
	for (UCefWebSocketServerBase* Server : Servers)
	{
		if (!Server)
		{
			continue;
		}
		const FCefWebSocketServerStats Stats = Server->GetStats();
		UE_LOG(
			LogCefWebSocketServer,
			Log,
			TEXT("  name=%s port=%d running=%s clients=%d queue=%lld"),
			*Server->GetServerNameId().ToString(),
			Server->GetBoundPort(),
			Server->IsRunning() ? TEXT("true") : TEXT("false"),
			Stats.ActiveClients,
			Stats.QueueDepth);
	}
}

void FCefWebSocketDebugCommands::HandleStop(const TArray<FString>& Args) const
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.stop usage: ws.stop <name>"));
		return;
	}

	UCefWebSocketSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.stop: subsystem not available"));
		return;
	}

	const FName NameId(*Args[0]);
	const bool bStopped = Subsystem->StopServer(NameId);
	UE_LOG(LogCefWebSocketServer, Log, TEXT("ws.stop: name=%s stopped=%s"), *NameId.ToString(), bStopped ? TEXT("true") : TEXT("false"));
}

void FCefWebSocketDebugCommands::HandleKick(const TArray<FString>& Args) const
{
	if (Args.Num() < 2)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.kick usage: ws.kick <server> <clientId>"));
		return;
	}

	UCefWebSocketSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.kick: subsystem not available"));
		return;
	}

	const FName NameId(*Args[0]);
	UCefWebSocketServerBase* Server = Subsystem->GetServer(NameId);
	if (!Server)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.kick: server not found for '%s'"), *NameId.ToString());
		return;
	}

	int64 ClientId = 0;
	if (!LexTryParseString(ClientId, *Args[1]))
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.kick: invalid client id '%s'"), *Args[1]);
		return;
	}

	const ECefWebSocketSendResult Result = Server->DisconnectClient(ClientId, ECefWebSocketCloseReason::Kicked);
	UE_LOG(LogCefWebSocketServer, Log, TEXT("ws.kick: server=%s client=%lld result=%d"), *NameId.ToString(), ClientId, static_cast<int32>(Result));
}

void FCefWebSocketDebugCommands::HandleStats(const TArray<FString>& Args) const
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.stats usage: ws.stats <name>"));
		return;
	}

	UCefWebSocketSubsystem* Subsystem = ResolveSubsystem();
	if (!Subsystem)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.stats: subsystem not available"));
		return;
	}

	const FName NameId(*Args[0]);
	UCefWebSocketServerBase* Server = Subsystem->GetServer(NameId);
	if (!Server)
	{
		UE_LOG(LogCefWebSocketServer, Warning, TEXT("ws.stats: server not found for '%s'"), *NameId.ToString());
		return;
	}

	const FCefWebSocketServerStats Stats = Server->GetStats();
	UE_LOG(
		LogCefWebSocketServer,
		Log,
		TEXT("ws.stats: name=%s clients=%d rx=%lld tx=%lld rx_s=%.2f tx_s=%.2f dropped=%lld queue=%lld avg_queue=%.2f"),
		*NameId.ToString(),
		Stats.ActiveClients,
		Stats.RxBytes,
		Stats.TxBytes,
		Stats.RxBytesPerSec,
		Stats.TxBytesPerSec,
		Stats.DroppedMessages,
		Stats.QueueDepth,
		Stats.AvgQueueDepth);
}
