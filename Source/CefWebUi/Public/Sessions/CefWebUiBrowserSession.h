#pragma once

#include "CoreMinimal.h"
#include "Services/CefWebUiRuntime.h"
#include "Templates/UniquePtr.h"
#include "UObject/Object.h"
#include "CefWebUiBrowserSession.generated.h"

class APlayerController;
class UCefWebUiBrowserWidget;
class UCefWebUiGameInstanceSubsystem;
class FCefInputWriter;
class FCefFrameReader;
class FCefControlWriter;


DECLARE_DYNAMIC_DELEGATE_OneParam(FCefWebUiWhenFinishedLoadingDelegate, UCefWebUiBrowserSession*, Session);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCefWebUiFinishedLoadingEvent, UCefWebUiBrowserSession*, Session);

UCLASS(BlueprintType)
class CEFWEBUI_API UCefWebUiBrowserSession : public UObject
{
	GENERATED_BODY()

public:

	virtual void BeginDestroy() override;

	void Initialize(UCefWebUiGameInstanceSubsystem* InOwnerSubsystem, FName InSessionId);

	UFUNCTION(BlueprintPure, Category="CefWebUi")
	FName GetSessionId() const { return SessionId; }

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	UCefWebUiBrowserWidget* GetWidget() const;

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	UCefWebUiBrowserWidget* CreateOrGetWidget(
		TSubclassOf<UCefWebUiBrowserWidget> WidgetClass,
		APlayerController* PlayerController,
		int32 ZOrder);

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void DestroyWidget();

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void Shutdown();

	TWeakPtr<FCefFrameReader> GetFrameReaderPtr() const;
	TWeakPtr<FCefInputWriter> GetInputWriterPtr() const;
	TWeakPtr<FCefControlWriter> GetControlWriterPtr() const;

	UFUNCTION(BlueprintCallable, Category="CefWebUi", meta=(AutoCreateRefTerm="Callback"))
	void BindWhenFinishedLoading(const FCefWebUiWhenFinishedLoadingDelegate& Callback);

	UFUNCTION(BlueprintPure, Category="CefWebUi")
	bool IsInitialLoadingFinished() const { return bInitialLoadingFinished; }

	UPROPERTY(BlueprintAssignable, Category="CefWebUi")
	FCefWebUiFinishedLoadingEvent OnFinishedLoading;

	void OnWidgetDestroyed(UCefWebUiBrowserWidget* InWidget);
	void HandleWidgetLoadStateChanged(uint8 InState);

private:
	void EnsureRuntimeStarted();
	void ShutdownRuntime();

private:
	TWeakObjectPtr<UCefWebUiGameInstanceSubsystem> OwnerSubsystem;
	FName SessionId = NAME_None;
	bool bInitialLoadingFinished = false;

	UPROPERTY(Transient)
	TObjectPtr<UCefWebUiBrowserWidget> Widget = nullptr;

	TArray<FCefWebUiWhenFinishedLoadingDelegate> PendingFinishedLoadingCallbacks;
	TUniquePtr<FCefWebUiRuntime> Runtime;
	TWeakPtr<FCefFrameReader> RuntimeFrameReader;
	FDelegateHandle LoadStateDelegateHandle;
};
