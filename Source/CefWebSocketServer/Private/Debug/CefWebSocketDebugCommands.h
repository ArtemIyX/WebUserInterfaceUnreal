#pragma once

#include "CoreMinimal.h"

class IConsoleObject;

class FCefWebSocketDebugCommands
{
public:
#pragma region Lifecycle
	FCefWebSocketDebugCommands() = default;
	~FCefWebSocketDebugCommands() = default;

	void Startup();
	void Shutdown();
#pragma endregion

private:
#pragma region Commands
	void HandleList(const TArray<FString>& InArgs) const;
	void HandleStop(const TArray<FString>& InArgs) const;
	void HandleKick(const TArray<FString>& InArgs) const;
	void HandleStats(const TArray<FString>& InArgs) const;
	void HandleClients(const TArray<FString>& InArgs) const;
	void HandleCVars(const TArray<FString>& InArgs) const;
	void HandleBenchSend(const TArray<FString>& InArgs) const;
#pragma endregion

private:
#pragma region Helpers
	class UCefWebSocketSubsystem* ResolveSubsystem() const;
#pragma endregion

private:
#pragma region InternalState
	IConsoleObject* CommandList = nullptr;
	IConsoleObject* CommandStop = nullptr;
	IConsoleObject* CommandKick = nullptr;
	IConsoleObject* CommandStats = nullptr;
	IConsoleObject* CommandClients = nullptr;
	IConsoleObject* CommandCVars = nullptr;
	IConsoleObject* CommandBenchSend = nullptr;
#pragma endregion
};
