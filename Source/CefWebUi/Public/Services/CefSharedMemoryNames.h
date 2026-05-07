#pragma once

#include "CoreMinimal.h"

struct FCefSharedMemoryNames
{
	FString FrameMapName;
	FString InputMapName;
	FString ControlMapName;
	FString ConsoleMapName;
	FString FrameReadyEventName;
	FString InputReadyEventName;
	FString ControlReadyEventName;
	FString ConsoleReadyEventName;
};

FString BuildCefSessionScope(const FString& InSessionId);
FCefSharedMemoryNames BuildCefSharedMemoryNames(const FString& InSessionScope);
