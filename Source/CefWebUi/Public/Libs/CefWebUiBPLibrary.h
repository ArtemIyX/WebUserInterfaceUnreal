// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CefWebUiBPLibrary.generated.h"

#pragma region Forward Declarations
class APlayerController;
class UCefWebUiBrowserSession;
class UCefWebUiBrowserWidget;
class UCefWebUiGameInstanceSubsystem;
class UCefWebUiSlateHostWidget;
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
	static UCefWebUiGameInstanceSubsystem* GetCefWebUiSubsystem(const UObject* WorldContextObject);
#pragma endregion

#pragma region Session
	UFUNCTION(BlueprintCallable, Category="CefWebUi", meta=(WorldContext="WorldContextObject"))
	static UCefWebUiBrowserSession* GetOrCreateBrowserSession(
		const UObject* WorldContextObject,
		FName SessionId);

	UFUNCTION(BlueprintCallable, Category="CefWebUi", meta=(WorldContext="WorldContextObject"))
	static void DestroyBrowserSession(
		const UObject* WorldContextObject,
		FName SessionId);
#pragma endregion

#pragma region Widget
	UFUNCTION(BlueprintCallable, Category="CefWebUi", meta=(WorldContext="WorldContextObject"))
	static UCefWebUiSlateHostWidget* CreateOrGetBrowserWidget(
		const UObject* WorldContextObject,
		FName SessionId,
		TSubclassOf<UCefWebUiSlateHostWidget> WidgetClass,
		APlayerController* PlayerController,
		int32 ZOrder);

	UFUNCTION(BlueprintCallable, Category="CefWebUi", meta=(WorldContext="WorldContextObject"))
	static UCefWebUiSlateHostWidget* GetBrowserWidget(
		const UObject* WorldContextObject,
		FName SessionId);
#pragma endregion

#pragma region Slate Widget
	UFUNCTION(BlueprintCallable, Category="CefWebUi", meta=(WorldContext="WorldContextObject"))
	static UCefWebUiSlateHostWidget* CreateBrowserSlateHostWidget(
		const UObject* WorldContextObject,
		FName SessionId,
		TSubclassOf<UCefWebUiSlateHostWidget> WidgetClass,
		APlayerController* PlayerController,
		int32 ZOrder,
		int32 BrowserWidth,
		int32 BrowserHeight);
#pragma endregion
};
