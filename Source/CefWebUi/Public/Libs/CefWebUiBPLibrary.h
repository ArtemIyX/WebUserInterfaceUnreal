/**
 * @file CefWebUi\Public\Libs\CefWebUiBPLibrary.h
 * @brief Declares CefWebUiBPLibrary for module CefWebUi\Public\Libs\CefWebUiBPLibrary.h.
 * @details Contains types and APIs used by the plugin runtime and gameplay-facing systems.
 */
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CefWebUiBPLibrary.generated.h"

#pragma region Forward Declarations
class APlayerController;
class UCefWebUiBrowserSession;
class UCefWebUiGameInstanceSubsystem;
#pragma endregion

UCLASS()
class CEFWEBUI_API UCefWebUiBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#pragma region Host
	/**
	 * @brief Returns absolute path to Host.exe.
	 */
	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	static FString GetHostExePath();
#pragma endregion

#pragma region Subsystem
	UFUNCTION(BlueprintCallable, Category="CefWebUi", meta=(WorldContext="WorldContextObject"))
	static UCefWebUiGameInstanceSubsystem* GetCefWebUiSubsystem(const UObject* InWorldContextObject);
#pragma endregion

#pragma region Session
	UFUNCTION(BlueprintCallable, Category="CefWebUi", meta=(WorldContext="WorldContextObject"))
	static UCefWebUiBrowserSession* GetOrCreateBrowserSession(
		const UObject* InWorldContextObject,
		FName InSessionId,
		TSubclassOf<UCefWebUiBrowserSession> InSessionClass = nullptr);

	UFUNCTION(BlueprintCallable, Category="CefWebUi", meta=(WorldContext="WorldContextObject"))
	static void DestroyBrowserSession(
		const UObject* InWorldContextObject,
		FName InSessionId);
#pragma endregion

#pragma region Viewport
	UFUNCTION(BlueprintCallable, Category="CefWebUi", meta=(WorldContext="WorldContextObject"))
	static UCefWebUiBrowserSession* ShowBrowserSessionInViewport(
		const UObject* InWorldContextObject,
		FName InSessionId,
		TSubclassOf<UCefWebUiBrowserSession> InSessionClass = nullptr,
		APlayerController* InPlayerController = nullptr,
		int32 InZOrder = 0,
		int32 InBrowserWidth = 1920,
		int32 InBrowserHeight = 1080);

	UFUNCTION(BlueprintCallable, Category="CefWebUi", meta=(WorldContext="WorldContextObject"))
	static void HideBrowserSessionFromViewport(
		const UObject* InWorldContextObject,
		FName InSessionId);
#pragma endregion
};

