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
class CEFWEBUI_API UCefWebUiBrowserSession : public UObject
{
	GENERATED_BODY()

public:
#pragma region Lifecycle
	UCefWebUiBrowserSession(const FObjectInitializer& ObjectInitializer);
	virtual void BeginDestroy() override;
	void Initialize(UCefWebUiGameInstanceSubsystem* InOwnerSubsystem, FName InSessionId);
#pragma endregion

#pragma region Widget
	UFUNCTION(BlueprintPure, Category="CefWebUi")
	FName GetSessionId() const { return SessionId; }

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void ShowInViewport(
		APlayerController* PlayerController,
		int32 ZOrder,
		int32 BrowserWidth = 1920,
		int32 BrowserHeight = 1080);

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void HideFromViewport();

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void Shutdown();

	UFUNCTION(BlueprintPure, Category="CefWebUi")
	bool IsShownInViewport() const;

	TSharedPtr<SCefBrowserSurface> GetSlateWidget() const { return BrowserSurfaceWidget; }
#pragma endregion

#pragma region Runtime Access
	TWeakPtr<FCefFrameReader> GetFrameReaderPtr() const;
	TWeakPtr<FCefInputWriter> GetInputWriterPtr() const;
	TWeakPtr<FCefControlWriter> GetControlWriterPtr() const;
#pragma endregion

#pragma region Control
	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void GoBack();

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void GoForward();

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void StopLoad();

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void Reload();

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void SetUrl(const FString& InUrl);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void SetPaused(bool bInPaused);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void SetHidden(bool bInHidden);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void SetFocus(bool bInFocus);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void SetZoomLevel(float InLevel);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void SetFrameRate(int32 InRate);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void ScrollTo(int32 InX, int32 InY);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void Resize(int32 InWidth, int32 InHeight);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void SetMuted(bool bInMuted);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void OpenDevTools();

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void CloseDevTools();

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void SetInputEnabled(bool bInEnabled);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void ExecuteJs(const FString& InScript);

	UFUNCTION(BlueprintCallable, Category="CefWebUi|Control")
	void ClearCookies();
#pragma endregion

#pragma region Loading

public:
	UFUNCTION(BlueprintCallable, Category="CefWebUi", meta=(AutoCreateRefTerm="Callback"))
	void BindWhenFinishedLoading(const FCefWebUiWhenFinishedLoadingDelegate& Callback);

	UFUNCTION(BlueprintPure, Category="CefWebUi")
	bool IsInitialLoadingFinished() const { return bInitialLoadingFinished; }


	void HandleWidgetLoadStateChanged(uint8 InState);
	void HandleConsoleLogMessage(ECefConsoleLogLevel InLevel, const FString& InMessage, const FString& InSource,
	                             int32 InLine);
#pragma endregion


#pragma region Events

	UPROPERTY(BlueprintAssignable, Category="CefWebUi")
	FCefWebUiFinishedLoadingEvent OnFinishedLoading;

	UPROPERTY(BlueprintAssignable, Category="CefWebUi")
	FCefWebUiConsoleMessageEvent OnConsoleMessage;

#pragma endregion

private:
#pragma region Runtime Internal
	void EnsureRuntimeStarted();
	void ShutdownRuntime();
	TSharedPtr<FCefControlWriter> GetOrOpenControlWriter();
	UGameViewportClient* GetGameViewportClient() const;
#pragma endregion

private:
#pragma region State
	TWeakObjectPtr<UCefWebUiGameInstanceSubsystem> OwnerSubsystem;
	FName SessionId = NAME_None;
	bool bInitialLoadingFinished = false;

	TSharedPtr<SCefBrowserSurface> BrowserSurfaceWidget;
	TSharedPtr<SWidget> ViewportWidgetHost;

	TArray<FCefWebUiWhenFinishedLoadingDelegate> PendingFinishedLoadingCallbacks;
	TUniquePtr<FCefWebUiRuntime> Runtime;
	TWeakPtr<FCefFrameReader> RuntimeFrameReader;
	TWeakPtr<FCefConsoleLogReader> RuntimeConsoleLogReader;
	FDelegateHandle LoadStateDelegateHandle;
	FDelegateHandle ConsoleLogDelegateHandle;
#pragma endregion
};
