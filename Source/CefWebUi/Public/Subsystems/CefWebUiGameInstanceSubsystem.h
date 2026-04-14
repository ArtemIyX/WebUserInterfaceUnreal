#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CefWebUiGameInstanceSubsystem.generated.h"

#pragma region Forward Declarations
class APlayerController;
class UCefWebUiBrowserSession;
class UCefWebUiBrowserWidget;
#pragma endregion

UCLASS(BlueprintType)
class CEFWEBUI_API UCefWebUiGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
#pragma region Lifecycle
	virtual void Deinitialize() override;
#pragma endregion

#pragma region Convenience
	UCefWebUiBrowserSession* GetOrCreateSession()
	{
		return GetOrCreateSession(NAME_None);
	}

	UCefWebUiBrowserSession* GetSession() const
	{
		return GetSession(NAME_None);
	}

	void DestroySession()
	{
		DestroySession(NAME_None);
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
#pragma endregion

#pragma region Session API
	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	UCefWebUiBrowserSession* GetOrCreateSession(FName SessionId);

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	UCefWebUiBrowserSession* GetSession(FName SessionId) const;

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void DestroySession(FName SessionId);

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	UCefWebUiBrowserWidget* GetSessionWidget(FName SessionId) const;

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	UCefWebUiBrowserWidget* CreateOrGetSessionWidget(
		FName SessionId,
		TSubclassOf<UCefWebUiBrowserWidget> WidgetClass,
		APlayerController* PlayerController,
		int32 ZOrder);
#pragma endregion

#pragma region Settings
	UFUNCTION(BlueprintPure, Category="CefWebUi")
	TSubclassOf<UCefWebUiBrowserWidget> GetDefaultWidgetClass() const { return DefaultWidgetClass; }

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void SetDefaultWidgetClass(TSubclassOf<UCefWebUiBrowserWidget> InWidgetClass);
#pragma endregion

private:
	FName NormalizeSessionId(FName SessionId) const;

private:
#pragma region State
	UPROPERTY(EditAnywhere, Category="CefWebUi")
	TSubclassOf<UCefWebUiBrowserWidget> DefaultWidgetClass;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UCefWebUiBrowserSession>> Sessions;
#pragma endregion
};
