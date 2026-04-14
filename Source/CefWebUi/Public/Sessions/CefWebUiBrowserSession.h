#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CefWebUiBrowserSession.generated.h"

class APlayerController;
class UCefWebUiBrowserWidget;
class UCefWebUiGameInstanceSubsystem;

DECLARE_DYNAMIC_DELEGATE_OneParam(FCefWebUiWhenFinishedLoadingDelegate, UCefWebUiBrowserSession*, Session);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCefWebUiFinishedLoadingEvent, UCefWebUiBrowserSession*, Session);

UCLASS(BlueprintType)
class CEFWEBUI_API UCefWebUiBrowserSession : public UObject
{
	GENERATED_BODY()

public:
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

	UFUNCTION(BlueprintCallable, Category="CefWebUi", meta=(AutoCreateRefTerm="Callback"))
	void BindWhenFinishedLoading(const FCefWebUiWhenFinishedLoadingDelegate& Callback);

	UFUNCTION(BlueprintPure, Category="CefWebUi")
	bool IsInitialLoadingFinished() const { return bInitialLoadingFinished; }

	UPROPERTY(BlueprintAssignable, Category="CefWebUi")
	FCefWebUiFinishedLoadingEvent OnFinishedLoading;

	void OnWidgetDestroyed(UCefWebUiBrowserWidget* InWidget);
	void HandleWidgetLoadStateChanged(uint8 InState);

private:
	TWeakObjectPtr<UCefWebUiGameInstanceSubsystem> OwnerSubsystem;
	FName SessionId = NAME_None;
	bool bInitialLoadingFinished = false;

	UPROPERTY(Transient)
	TObjectPtr<UCefWebUiBrowserWidget> Widget = nullptr;

	TArray<FCefWebUiWhenFinishedLoadingDelegate> PendingFinishedLoadingCallbacks;
};
