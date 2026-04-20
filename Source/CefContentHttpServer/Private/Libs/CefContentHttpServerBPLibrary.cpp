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

bool UCefContentHttpServerBPLibrary::InitDefaultImageCacher(const UObject* InWorldContextObject)
{
	UCefContentHttpServerSubsystem* const subsystem = GetCefContentHttpServerSubsystem(InWorldContextObject);
	if (!subsystem)
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("InitDefaultImageCacher failed: subsystem is null"));
		return false;
	}

	subsystem->InitDefaultImageCacher();
	UE_LOG(LogCefContentHttpServer, Log, TEXT("InitDefaultImageCacher succeeded"));
	return true;
}
