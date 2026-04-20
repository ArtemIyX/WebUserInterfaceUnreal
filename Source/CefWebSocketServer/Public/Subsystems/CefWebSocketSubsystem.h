/**
 * @file CefWebSocketServer\Public\Subsystems\CefWebSocketSubsystem.h
 * @brief Declares CefWebSocketSubsystem for module CefWebSocketServer\Public\Subsystems\CefWebSocketSubsystem.h.
 * @details Contains subsystem APIs and lifecycle entry points used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/CefWebSocketStructs.h"
#include "Data/CefWebSocketEnums.h"
#include "CefWebSocketSubsystem.generated.h"

class UCefWebSocketServerBase;
class UCefWebSocketClientBase;

UCLASS()
/** @brief Type declaration. */
class CEFWEBSOCKETSERVER_API UCefWebSocketSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
#pragma region Lifecycle
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
#pragma endregion

public:
#pragma region PublicApi
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	FCefWebSocketServerCreateResult CreateOrGetServer(
		const FCefWebSocketServerCreateOptions& InOptions,
		TSubclassOf<UCefWebSocketServerBase> InServerClass,
		TSubclassOf<UCefWebSocketClientBase> InClientClass,
		UCefWebSocketServerBase*& OutServer);

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	/** @brief GetServer API. */
	UCefWebSocketServerBase* GetServer(FName InNameId) const;

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	/** @brief StopServer API. */
	bool StopServer(FName InNameId);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	/** @brief StopAllServers API. */
	void StopAllServers();

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	/** @brief GetAllServers API. */
	TArray<UCefWebSocketServerBase*> GetAllServers() const;
#pragma endregion

private:
#pragma region Internal
	/** @brief TryResolvePort API. */
	bool TryResolvePort(int32 InRequestedPort, int32& OutResolvedPort, bool& bOutAdjusted) const;

	UPROPERTY(Transient)
	/** @brief Servers state. */
	TMap<FName, TObjectPtr<UCefWebSocketServerBase>> Servers;
#pragma endregion
};

