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

    UWorld* const World = InWorldContextObject->GetWorld();
    if (!World)
    {
        UE_LOG(LogCefContentHttpServer, Error, TEXT("GetCefContentHttpServerSubsystem failed: world is null"));
        return nullptr;
    }

    UGameInstance* const GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        UE_LOG(LogCefContentHttpServer, Error, TEXT("GetCefContentHttpServerSubsystem failed: game instance is null"));
        return nullptr;
    }

    return GameInstance->GetSubsystem<UCefContentHttpServerSubsystem>();
}

bool UCefContentHttpServerBPLibrary::InitDefaultImageCacher(const UObject* InWorldContextObject)
{
    UCefContentHttpServerSubsystem* const Subsystem = GetCefContentHttpServerSubsystem(InWorldContextObject);
    if (!Subsystem)
    {
        UE_LOG(LogCefContentHttpServer, Error, TEXT("InitDefaultImageCacher failed: subsystem is null"));
        return false;
    }

    Subsystem->InitDefaultImageCacher();
    UE_LOG(LogCefContentHttpServer, Log, TEXT("InitDefaultImageCacher succeeded"));
    return true;
}
