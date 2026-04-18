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
	void HandleList(const TArray<FString>& Args) const;
	void HandleStop(const TArray<FString>& Args) const;
	void HandleKick(const TArray<FString>& Args) const;
	void HandleStats(const TArray<FString>& Args) const;
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
#pragma endregion
};
