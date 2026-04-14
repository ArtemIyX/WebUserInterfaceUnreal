// Fill out your copyright notice in the Description page of Project Settings.


#include "Libs/CefWebUiBPLibrary.h"

#include "Sessions/CefWebUiBrowserSession.h"
#include "Interfaces/IPluginManager.h"
#include "Subsystems/CefWebUiGameInstanceSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"


FString UCefWebUiBPLibrary::GetHostExePath()
{
#if WITH_EDITOR
	FString pluginDir = IPluginManager::Get().FindPlugin(TEXT("CefWebUi"))->GetBaseDir();
	return FPaths::ConvertRelativePathToFull(
		FPaths::Combine(pluginDir, TEXT("Source"), TEXT("ThirdParty"), TEXT("Cef"), TEXT("Host.exe"))
	);
#else
	return FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("Cef"), TEXT("Host.exe"))
	);
#endif
}

UCefWebUiGameInstanceSubsystem* UCefWebUiBPLibrary::GetCefWebUiSubsystem(const UObject* WorldContextObject)
{
	if (!IsValid(WorldContextObject))
		return nullptr;

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
		return nullptr;

	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
		return nullptr;

	return GI->GetSubsystem<UCefWebUiGameInstanceSubsystem>();
}

UCefWebUiBrowserSession* UCefWebUiBPLibrary::GetOrCreateBrowserSession(
	const UObject* WorldContextObject,
	FName SessionId,
	TSubclassOf<UCefWebUiBrowserSession> SessionClass)
{
	if (UCefWebUiGameInstanceSubsystem* Subsystem = GetCefWebUiSubsystem(WorldContextObject))
	{
		return Subsystem->GetOrCreateSession(SessionId, SessionClass);
	}
	return nullptr;
}

void UCefWebUiBPLibrary::DestroyBrowserSession(
	const UObject* WorldContextObject,
	FName SessionId)
{
	if (UCefWebUiGameInstanceSubsystem* Subsystem = GetCefWebUiSubsystem(WorldContextObject))
	{
		Subsystem->DestroySession(SessionId);
	}
}

UCefWebUiBrowserSession* UCefWebUiBPLibrary::ShowBrowserSessionInViewport(
	const UObject* WorldContextObject,
	FName SessionId,
	TSubclassOf<UCefWebUiBrowserSession> SessionClass,
	APlayerController* PlayerController,
	int32 ZOrder,
	int32 BrowserWidth,
	int32 BrowserHeight)
{
	UCefWebUiBrowserSession* session = GetOrCreateBrowserSession(WorldContextObject, SessionId, SessionClass);
	if (!session)
	{
		return nullptr;
	}

	if (!PlayerController)
	{
		if (UWorld* world = WorldContextObject ? WorldContextObject->GetWorld() : nullptr)
		{
			if (UGameInstance* gameInstance = world->GetGameInstance())
			{
				PlayerController = gameInstance->GetFirstLocalPlayerController();
			}
		}
	}

	session->ShowInViewport(PlayerController, ZOrder, BrowserWidth, BrowserHeight);
	return session;
}

void UCefWebUiBPLibrary::HideBrowserSessionFromViewport(
	const UObject* WorldContextObject,
	FName SessionId)
{
	if (UCefWebUiGameInstanceSubsystem* subsystem = GetCefWebUiSubsystem(WorldContextObject))
	{
		subsystem->HideSessionFromViewport(SessionId);
	}
}
