// Fill out your copyright notice in the Description page of Project Settings.


#include "Libs/CefWebUiBPLibrary.h"

#include "Sessions/CefWebUiBrowserSession.h"
#include "Interfaces/IPluginManager.h"
#include "Slate/CefWebUiSlateHostWidget.h"
#include "Subsystems/CefWebUiGameInstanceSubsystem.h"
#include "Widgets/CefWebUiBrowserWidget.h"
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
	FName SessionId)
{
	if (UCefWebUiGameInstanceSubsystem* Subsystem = GetCefWebUiSubsystem(WorldContextObject))
	{
		return Subsystem->GetOrCreateSession(SessionId);
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

UCefWebUiBrowserWidget* UCefWebUiBPLibrary::CreateOrGetBrowserWidget(
	const UObject* WorldContextObject,
	FName SessionId,
	TSubclassOf<UCefWebUiBrowserWidget> WidgetClass,
	APlayerController* PlayerController,
	int32 ZOrder)
{
	if (UCefWebUiGameInstanceSubsystem* Subsystem = GetCefWebUiSubsystem(WorldContextObject))
	{
		return Subsystem->CreateOrGetSessionWidget(SessionId, WidgetClass, PlayerController, ZOrder);
	}
	return nullptr;
}

UCefWebUiBrowserWidget* UCefWebUiBPLibrary::GetBrowserWidget(
	const UObject* WorldContextObject,
	FName SessionId)
{
	if (UCefWebUiGameInstanceSubsystem* Subsystem = GetCefWebUiSubsystem(WorldContextObject))
	{
		return Subsystem->GetSessionWidget(SessionId);
	}
	return nullptr;
}

UCefWebUiSlateHostWidget* UCefWebUiBPLibrary::CreateBrowserSlateHostWidget(
	const UObject* WorldContextObject,
	FName SessionId,
	TSubclassOf<UCefWebUiSlateHostWidget> WidgetClass,
	APlayerController* PlayerController,
	int32 ZOrder,
	int32 BrowserWidth,
	int32 BrowserHeight)
{
	UCefWebUiBrowserSession* session = GetOrCreateBrowserSession(WorldContextObject, SessionId);
	if (!session)
	{
		return nullptr;
	}

	if (!WidgetClass)
	{
		WidgetClass = UCefWebUiSlateHostWidget::StaticClass();
	}

	UWorld* world = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	UGameInstance* gameInstance = world ? world->GetGameInstance() : nullptr;
	if (!gameInstance)
	{
		return nullptr;
	}

	APlayerController* targetPlayerController = PlayerController ? PlayerController : gameInstance->GetFirstLocalPlayerController();
	if (!targetPlayerController)
	{
		return nullptr;
	}

	UCefWebUiSlateHostWidget* slateHostWidget = CreateWidget<UCefWebUiSlateHostWidget>(targetPlayerController, WidgetClass);
	if (!slateHostWidget)
	{
		return nullptr;
	}

	slateHostWidget->SetBrowserSession(session);
	slateHostWidget->SetBrowserSize(BrowserWidth, BrowserHeight);
	slateHostWidget->AddToViewport(ZOrder);
	return slateHostWidget;
}
