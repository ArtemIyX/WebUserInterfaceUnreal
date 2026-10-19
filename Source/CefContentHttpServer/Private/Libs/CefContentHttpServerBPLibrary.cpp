/**
 * @file CefContentHttpServer/Private/Libs/CefContentHttpServerBPLibrary.cpp
 * @brief Blueprint helper implementation.
 */
#include "Libs/CefContentHttpServerBPLibrary.h"

#include "CefContentHttpServer.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Subsystems/CefContentHttpServerSubsystem.h"

UCefContentHttpServerSubsystem* UCefContentHttpServerBPLibrary::GetCefContentHttpServerSubsystem(const UObject* InWorldContextObject)
{
	if (!IsValid(InWorldContextObject))
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("GetCefContentHttpServerSubsystem failed: world context object is invalid"));
		return nullptr;
	}

	UWorld* const world = InWorldContextObject->GetWorld();
	if (!world)
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("GetCefContentHttpServerSubsystem failed: world is null"));
		return nullptr;
	}

	UGameInstance* const gameInstance = world->GetGameInstance();
	if (!gameInstance)
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("GetCefContentHttpServerSubsystem failed: game instance is null"));
		return nullptr;
	}

	return gameInstance->GetSubsystem<UCefContentHttpServerSubsystem>();
}
