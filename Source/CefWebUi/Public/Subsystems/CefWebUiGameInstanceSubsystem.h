#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CefWebUiGameInstanceSubsystem.generated.h"

class APlayerController;
class UCefWebUiBrowserSession;
class UCefWebUiBrowserWidget;

UCLASS(BlueprintType)
class CEFWEBUI_API UCefWebUiGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UCefWebUiBrowserSession* GetOrCreateSession()
	{
		return GetOrCreateSession(NAME_None);
	}

	UCefWebUiBrowserSession* GetSession() const
	{
		return GetSession(NAME_None);
	}

	UCefWebUiBrowserWidget* GetSessionWidget() const
	{
		return GetSessionWidget(NAME_None);
	}

	UCefWebUiBrowserWidget* CreateOrGetSessionWidget(
		TSubclassOf<UCefWebUiBrowserWidget> WidgetClass = nullptr,
		APlayerController* PlayerController = nullptr,
		int32 ZOrder = 0)
	{
		return CreateOrGetSessionWidget(NAME_None, WidgetClass, PlayerController, ZOrder);
	}

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	UCefWebUiBrowserSession* GetOrCreateSession(FName SessionId);

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	UCefWebUiBrowserSession* GetSession(FName SessionId) const;

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	UCefWebUiBrowserWidget* GetSessionWidget(FName SessionId) const;

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	UCefWebUiBrowserWidget* CreateOrGetSessionWidget(
		FName SessionId,
		TSubclassOf<UCefWebUiBrowserWidget> WidgetClass,
		APlayerController* PlayerController,
		int32 ZOrder);

	UFUNCTION(BlueprintPure, Category="CefWebUi")
	TSubclassOf<UCefWebUiBrowserWidget> GetDefaultWidgetClass() const { return DefaultWidgetClass; }

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void SetDefaultWidgetClass(TSubclassOf<UCefWebUiBrowserWidget> InWidgetClass);

private:
	FName NormalizeSessionId(FName SessionId) const;

private:
	UPROPERTY(EditAnywhere, Category="CefWebUi")
	TSubclassOf<UCefWebUiBrowserWidget> DefaultWidgetClass;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UCefWebUiBrowserSession>> Sessions;
};
