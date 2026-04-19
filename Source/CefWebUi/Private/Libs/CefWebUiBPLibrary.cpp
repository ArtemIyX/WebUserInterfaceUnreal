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

UCefWebUiGameInstanceSubsystem* UCefWebUiBPLibrary::GetCefWebUiSubsystem(const UObject* InWorldContextObject)
{
	if (!IsValid(InWorldContextObject))
		return nullptr;

	UWorld* World = InWorldContextObject->GetWorld();
	if (!World)
		return nullptr;

	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
		return nullptr;

	return GI->GetSubsystem<UCefWebUiGameInstanceSubsystem>();
}

UCefWebUiBrowserSession* UCefWebUiBPLibrary::GetOrCreateBrowserSession(
	const UObject* InWorldContextObject,
	FName InSessionId,
	TSubclassOf<UCefWebUiBrowserSession> InSessionClass)
{
	if (UCefWebUiGameInstanceSubsystem* Subsystem = GetCefWebUiSubsystem(InWorldContextObject))
	{
		return Subsystem->GetOrCreateSession(InSessionId, InSessionClass);
	}
	return nullptr;
}

void UCefWebUiBPLibrary::DestroyBrowserSession(
	const UObject* InWorldContextObject,
	FName InSessionId)
{
	if (UCefWebUiGameInstanceSubsystem* Subsystem = GetCefWebUiSubsystem(InWorldContextObject))
	{
		Subsystem->DestroySession(InSessionId);
	}
}

UCefWebUiBrowserSession* UCefWebUiBPLibrary::ShowBrowserSessionInViewport(
	const UObject* InWorldContextObject,
	FName InSessionId,
	TSubclassOf<UCefWebUiBrowserSession> InSessionClass,
	APlayerController* InPlayerController,
	int32 InZOrder,
	int32 InBrowserWidth,
	int32 InBrowserHeight)
{
	UCefWebUiBrowserSession* session = GetOrCreateBrowserSession(InWorldContextObject, InSessionId, InSessionClass);
	if (!session)
	{
		return nullptr;
	}

	if (!InPlayerController)
	{
		if (UWorld* world = InWorldContextObject ? InWorldContextObject->GetWorld() : nullptr)
		{
			if (UGameInstance* gameInstance = world->GetGameInstance())
			{
				InPlayerController = gameInstance->GetFirstLocalPlayerController();
			}
		}
	}

	session->ShowInViewport(InPlayerController, InZOrder, InBrowserWidth, InBrowserHeight);
	return session;
}

void UCefWebUiBPLibrary::HideBrowserSessionFromViewport(
	const UObject* InWorldContextObject,
	FName InSessionId)
{
	if (UCefWebUiGameInstanceSubsystem* subsystem = GetCefWebUiSubsystem(InWorldContextObject))
	{
		subsystem->HideSessionFromViewport(InSessionId);
	}
}
