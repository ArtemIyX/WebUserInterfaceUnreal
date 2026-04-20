/**
 * @file CefWebUi\Public\Sessions\CefWebUiBrowserSession.h
 * @brief Declares CefWebUiBrowserSession for module CefWebUi\Public\Sessions\CefWebUiBrowserSession.h.
 * @details Contains types and APIs used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
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

#pragma region Widget
	UFUNCTION(BlueprintPure, Category="CefWebUi")
	FName GetSessionId() const { return SessionId; }

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void ShowInViewport(
		APlayerController* InPlayerController,
		int32 InZOrder,
		int32 InBrowserWidth = 1920,
		int32 InBrowserHeight = 1080);

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	/** @brief HideFromViewport API. */
	void HideFromViewport();

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	/** @brief Shutdown API. */
	void Shutdown();

	UFUNCTION(BlueprintPure, Category="CefWebUi")
	/** @brief IsShownInViewport API. */
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

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief GoForward API. */
	void GoForward();

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief StopLoad API. */
	void StopLoad();

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief Reload API. */
	void Reload();

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief SetUrl API. */
	void SetUrl(const FString& InUrl);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief SetPaused API. */
	void SetPaused(bool bInPaused);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief SetHidden API. */
	void SetHidden(bool bInHidden);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief SetFocus API. */
	void SetFocus(bool bInFocus);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief SetZoomLevel API. */
	void SetZoomLevel(float InLevel);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief SetFrameRate API. */
	void SetFrameRate(int32 InRate);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief ScrollTo API. */
	void ScrollTo(int32 InX, int32 InY);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief Resize API. */
	void Resize(int32 InWidth, int32 InHeight);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief SetMuted API. */
	void SetMuted(bool bInMuted);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief OpenDevTools API. */
	void OpenDevTools();

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief CloseDevTools API. */
	void CloseDevTools();

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief SetInputEnabled API. */
	void SetInputEnabled(bool bInEnabled);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief ExecuteJs API. */
	void ExecuteJs(const FString& InScript);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief OpenLocalFile API. */
	void OpenLocalFile(const FString& InLocalFilePath);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief LoadHtmlString API. */
	void LoadHtmlString(const FString& InHtml);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	/** @brief ClearCookies API. */
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
	void HandleWidgetLoadStateChanged(uint8 InState);
	void HandleConsoleLogMessage(ECefConsoleLogLevel InLevel, const FString& InMessage, const FString& InSource,
	                             int32 InLine);
#pragma endregion


#pragma region Events

	UPROPERTY(BlueprintAssignable, Category="CefWebUi")
	/** @brief OnFinishedLoading state. */
	FCefWebUiFinishedLoadingEvent OnFinishedLoading;

	UPROPERTY(BlueprintAssignable, Category="CefWebUi")
	/** @brief OnConsoleMessage state. */
	FCefWebUiConsoleMessageEvent OnConsoleMessage;

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
#pragma endregion
};

