#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CefWebUiGameInstanceSubsystem.generated.h"

#pragma region Forward Declarations
class APlayerController;
class UCefWebUiBrowserSession;
#pragma endregion

UCLASS(BlueprintType)
class CEFWEBUI_API UCefWebUiGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UCefWebUiGameInstanceSubsystem();
public:
#pragma region Lifecycle
	virtual void Deinitialize() override;
#pragma endregion

#pragma region Convenience
	UCefWebUiBrowserSession* GetOrCreateSession()
	{
		return GetOrCreateSession(NAME_None, nullptr);
	}

	UCefWebUiBrowserSession* GetSession() const
	{
		return GetSession(NAME_None);
	}

	void DestroySession()
	{
		DestroySession(NAME_None);
	}

	void ShowSessionInViewport(
		APlayerController* InPlayerController = nullptr,
		int32 InZOrder = 0,
		int32 InBrowserWidth = 1920,
		int32 InBrowserHeight = 1080)
	{
		ShowSessionInViewport(NAME_None, InPlayerController, InZOrder, InBrowserWidth, InBrowserHeight);
	}

	void HideSessionFromViewport()
	{
		HideSessionFromViewport(NAME_None);
	}
#pragma endregion

#pragma region Session API
	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	UCefWebUiBrowserSession* GetOrCreateSession(
		FName InSessionId,
		TSubclassOf<UCefWebUiBrowserSession> InSessionClass = nullptr);

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	UCefWebUiBrowserSession* GetSession(FName InSessionId) const;

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void DestroySession(FName InSessionId);

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void ShowSessionInViewport(
		FName InSessionId,
		APlayerController* InPlayerController,
		int32 InZOrder,
		int32 InBrowserWidth,
		int32 InBrowserHeight);

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void HideSessionFromViewport(FName InSessionId);
#pragma endregion

#pragma region Settings
	UFUNCTION(BlueprintPure, Category="CefWebUi")
	TSubclassOf<UCefWebUiBrowserSession> GetDefaultSessionClass() const { return DefaultSessionClass; }

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void SetDefaultSessionClass(TSubclassOf<UCefWebUiBrowserSession> InSessionClass);
#pragma endregion

private:
	FName NormalizeSessionId(FName InSessionId) const;

private:
#pragma region State
	UPROPERTY(EditAnywhere, Category="CefWebUi")
	TSubclassOf<UCefWebUiBrowserSession> DefaultSessionClass;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UCefWebUiBrowserSession>> Sessions;
#pragma endregion
};
