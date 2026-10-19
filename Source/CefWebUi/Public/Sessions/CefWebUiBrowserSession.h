/**
 * @file CefWebUi\Public\Sessions\CefWebUiBrowserSession.h
 * @brief Declares CefWebUiBrowserSession for module CefWebUi\Public\Sessions\CefWebUiBrowserSession.h.
 * @details Contains types and APIs used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Data/CefConsoleLogLevel.h"
#include "Services/CefWebUiRuntime.h"
#include "Templates/UniquePtr.h"
#include "Widgets/SWidget.h"
#include "UObject/Object.h"
#include "CefWebUiBrowserSession.generated.h"

#pragma region Forward Declarations
class APlayerController;
class UCefWebUiGameInstanceSubsystem;
class FCefInputWriter;
class FCefFrameReader;
class FCefControlWriter;
class FCefConsoleLogReader;
class SCefBrowserSurface;
class UGameViewportClient;
#pragma endregion

#pragma region Delegates
DECLARE_DYNAMIC_DELEGATE_OneParam(FCefWebUiWhenFinishedLoadingDelegate, UCefWebUiBrowserSession*, Session);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCefWebUiFinishedLoadingEvent, UCefWebUiBrowserSession*, Session);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FCefWebUiConsoleMessageEvent,
	ECefConsoleLogLevel, Level, const FString&, Message, const FString&,
	Source, int32, Line);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCefWebUiConsoleMarkerEvent, const FString&, OrigLog, const FGameplayTag&, Marker);

DECLARE_DYNAMIC_DELEGATE_OneParam(FCefWebUiMarkerCallback, const FString&, Message);

DECLARE_DELEGATE_OneParam(FCefWebUiMarkerDelegate, const FString&);
#pragma endregion

UCLASS(BlueprintType)
/** @brief Type declaration. */
class CEFWEBUI_API UCefWebUiBrowserSession : public UObject
{
	GENERATED_BODY()

public:
	#pragma region Lifecycle
	/** @brief UCefWebUiBrowserSession API. */
	UCefWebUiBrowserSession(const FObjectInitializer& InObjectInitializer);
	virtual void BeginDestroy() override;
	/** @brief Initialize API. */
	void Initialize(UCefWebUiGameInstanceSubsystem* InOwnerSubsystem, FName InSessionId);
	#pragma endregion

public:
	#pragma region Markers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CefWebUi")
	TMap<FString, FGameplayTag> MarkerMap;

	/** @brief OnMarker API. */
	FCefWebUiMarkerDelegate& OnMarker(const FGameplayTag& InMarkerTag);

	/** @brief BindMarker API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Markers", meta=(AutoCreateRefTerm="Callback"))
	void BindMarker(const FGameplayTag& InMarkerTag, const FCefWebUiMarkerCallback& InCallback);

	/** @brief UnbindMarker API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Markers")
	void UnbindMarker(const FGameplayTag& InMarkerTag);
	#pragma endregion

	#pragma region Widget
	UFUNCTION(BlueprintPure, Category="CefWebUi")
	FName GetSessionId() const { return SessionId; }

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void ShowInViewport(
		APlayerController* InPlayerController,
		int32 InZOrder,
		int32 InBrowserWidth = 1920,
		int32 InBrowserHeight = 1080);

	/** @brief HideFromViewport API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void HideFromViewport();

	/** @brief Shutdown API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void Shutdown();

	/** @brief IsShownInViewport API. */
	UFUNCTION(BlueprintPure, Category="CefWebUi")
	bool IsShownInViewport() const;

	TSharedPtr<SCefBrowserSurface> GetSlateWidget() const { return BrowserSurfaceWidget; }
	#pragma endregion

	#pragma region Runtime Access
	/** @brief GetFrameReaderPtr API. */
	TWeakPtr<FCefFrameReader> GetFrameReaderPtr() const;

	/** @brief GetInputWriterPtr API. */
	TWeakPtr<FCefInputWriter> GetInputWriterPtr() const;

	/** @brief GetControlWriterPtr API. */
	TWeakPtr<FCefControlWriter> GetControlWriterPtr() const;
	#pragma endregion

	#pragma region Control
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief GoBack API. */
	void GoBack();

	/** @brief GoForward API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void GoForward();

	/** @brief StopLoad API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void StopLoad();

	/** @brief Reload API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void Reload();

	/** @brief SetUrl API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void SetUrl(const FString& InUrl);

	/** @brief SetPaused API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void SetPaused(bool bInPaused);

	/** @brief SetHidden API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void SetHidden(bool bInHidden);

	/** @brief SetFocus API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void SetFocus(bool bInFocus);

	/** @brief SetZoomLevel API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void SetZoomLevel(float InLevel);

	/** @brief SetFrameRate API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void SetFrameRate(int32 InRate);

	/** @brief ScrollTo API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void ScrollTo(int32 InX, int32 InY);

	/** @brief Resize API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void Resize(int32 InWidth, int32 InHeight);

	/** @brief SetMuted API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void SetMuted(bool bInMuted);

	/** @brief OpenDevTools API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void OpenDevTools();

	/** @brief CloseDevTools API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void CloseDevTools();

	/** @brief SetInputEnabled API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void SetInputEnabled(bool bInEnabled);

	/** @brief ExecuteJs API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void ExecuteJs(const FString& InScript);

	/** @brief OpenLocalFile API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void OpenLocalFile(const FString& InLocalFilePath);

	/** @brief LoadHtmlString API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void LoadHtmlString(const FString& InHtml);

	/** @brief ClearCookies API. */
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void ClearCookies();
	#pragma endregion

	#pragma region Loading

public:
	UFUNCTION(BlueprintCallable, Category="CefWebUi", meta=(AutoCreateRefTerm="Callback"))
	/** @brief BindWhenFinishedLoading API. */
	void BindWhenFinishedLoading(const FCefWebUiWhenFinishedLoadingDelegate& InCallback);

	UFUNCTION(BlueprintPure, Category="CefWebUi")
	bool IsInitialLoadingFinished() const { return bInitialLoadingFinished; }


	/** @brief HandleWidgetLoadStateChanged API. */
	virtual void HandleWidgetLoadStateChanged(uint8 InState);

	/**
	 * @brief Handle web browser console messages (probably called by JS)
	 * @note Game Thread
	 */
	virtual void HandleConsoleLogMessage(ECefConsoleLogLevel InLevel, const FString& InMessage, const FString& InSource,
		int32 InLine);
	#pragma endregion


	#pragma region Events

	/** @brief OnFinishedLoading state. */
	UPROPERTY(BlueprintAssignable, Category="CefWebUi|Events")
	FCefWebUiFinishedLoadingEvent OnFinishedLoading;

	/** @brief OnConsoleMessage state. */
	UPROPERTY(BlueprintAssignable, Category="CefWebUi|Events")
	FCefWebUiConsoleMessageEvent OnConsoleMessage;

	UPROPERTY(BlueprintAssignable, Category="CefWebUi|Events")
	FCefWebUiConsoleMarkerEvent OnConsoleMarker;
	#pragma endregion

private:
	#pragma region Runtime Internal
	/** @brief EnsureRuntimeStarted API. */
	void EnsureRuntimeStarted();
	/** @brief ShutdownRuntime API. */
	void ShutdownRuntime();
	/** @brief GetOrOpenControlWriter API. */
	TSharedPtr<FCefControlWriter> GetOrOpenControlWriter();
	/** @brief GetGameViewportClient API. */
	UGameViewportClient* GetGameViewportClient() const;
	#pragma endregion

private:
	#pragma region State
	/** @brief OwnerSubsystem state. */
	TWeakObjectPtr<UCefWebUiGameInstanceSubsystem> OwnerSubsystem;
	/** @brief SessionId state. */
	FName SessionId = NAME_None;
	/** @brief bInitialLoadingFinished state. */
	bool bInitialLoadingFinished = false;

	/** @brief BrowserSurfaceWidget state. */
	TSharedPtr<SCefBrowserSurface> BrowserSurfaceWidget;
	/** @brief ViewportWidgetHost state. */
	TSharedPtr<SWidget> ViewportWidgetHost;

	/** @brief PendingFinishedLoadingCallbacks state. */
	TArray<FCefWebUiWhenFinishedLoadingDelegate> PendingFinishedLoadingCallbacks;
	/** @brief Runtime state. */
	TUniquePtr<FCefWebUiRuntime> Runtime;
	/** @brief RuntimeFrameReader state. */
	TWeakPtr<FCefFrameReader> RuntimeFrameReader;
	/** @brief RuntimeConsoleLogReader state. */
	TWeakPtr<FCefConsoleLogReader> RuntimeConsoleLogReader;
	/** @brief LoadStateDelegateHandle state. */
	FDelegateHandle LoadStateDelegateHandle;
	/** @brief ConsoleLogDelegateHandle state. */
	FDelegateHandle ConsoleLogDelegateHandle;

	/** @brief MarkerDelegates state. */
	TMap<FGameplayTag, FCefWebUiMarkerDelegate> MarkerDelegates;
	/** @brief BlueprintMarkerDelegates state. */
	UPROPERTY()
	TMap<FGameplayTag, FCefWebUiMarkerCallback> BlueprintMarkerDelegates;
	/** @brief InvalidMarkerDelegate state. */
	FCefWebUiMarkerDelegate InvalidMarkerDelegate;
	#pragma endregion
};
