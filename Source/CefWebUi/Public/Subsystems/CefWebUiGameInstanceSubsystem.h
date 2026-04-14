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
		APlayerController* PlayerController = nullptr,
		int32 ZOrder = 0,
		int32 BrowserWidth = 1920,
		int32 BrowserHeight = 1080)
	{
		ShowSessionInViewport(NAME_None, PlayerController, ZOrder, BrowserWidth, BrowserHeight);
	}

	void HideSessionFromViewport()
	{
		HideSessionFromViewport(NAME_None);
	}
#pragma endregion

#pragma region Session API
	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	UCefWebUiBrowserSession* GetOrCreateSession(
		FName SessionId,
		TSubclassOf<UCefWebUiBrowserSession> SessionClass = nullptr);

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	UCefWebUiBrowserSession* GetSession(FName SessionId) const;

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void DestroySession(FName SessionId);

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void ShowSessionInViewport(
		FName SessionId,
		APlayerController* PlayerController,
		int32 ZOrder,
		int32 BrowserWidth,
		int32 BrowserHeight);

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void HideSessionFromViewport(FName SessionId);
#pragma endregion

#pragma region Settings
	UFUNCTION(BlueprintPure, Category="CefWebUi")
	TSubclassOf<UCefWebUiBrowserSession> GetDefaultSessionClass() const { return DefaultSessionClass; }

	UFUNCTION(BlueprintCallable, Category="CefWebUi")
	void SetDefaultSessionClass(TSubclassOf<UCefWebUiBrowserSession> InSessionClass);
#pragma endregion

private:
	FName NormalizeSessionId(FName SessionId) const;

private:
#pragma region State
	UPROPERTY(EditAnywhere, Category="CefWebUi")
	TSubclassOf<UCefWebUiBrowserSession> DefaultSessionClass;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UCefWebUiBrowserSession>> Sessions;
#pragma endregion
};
