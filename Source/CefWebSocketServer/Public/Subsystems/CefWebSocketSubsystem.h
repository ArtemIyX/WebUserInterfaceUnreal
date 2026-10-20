/**
 * @file CefWebSocketServer\Public\Subsystems\CefWebSocketSubsystem.h
 * @brief Declares the game-instance subsystem that owns Cef WebSocket servers.
 */
#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/CefWebSocketStructs.h"
#include "Data/CefWebSocketEnums.h"
#include "CefWebSocketSubsystem.generated.h"

class UCefWebSocketServerBase;
class UCefWebSocketClientBase;
class FCefWebSocketGameThreadEventTestAccess;

/** @brief Manages the lifetime and lookup of WebSocket server objects for a game instance. */
UCLASS()
class CEFWEBSOCKETSERVER_API UCefWebSocketSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UCefWebSocketSubsystem();
#pragma region Lifecycle
	/** @brief Determines whether this subsystem is created for the owning game instance. */
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	/** @brief Initializes server ownership and tick registration. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	/** @brief Stops owned servers and unregisters ticking. */
	virtual void Deinitialize() override;
	/** @brief Dispatches a bounded round-robin batch of queued server events. */
	virtual void Tick(float InDeltaTime) override;
	/** @brief Returns the stat identifier for subsystem event dispatch. */
	virtual TStatId GetStatId() const override;
	/** @brief Returns whether the initialized game instance may tick this subsystem. */
	virtual bool IsTickable() const override;
	/** @brief Keeps one global dispatch budget for the game instance. */
	virtual UWorld* GetTickableGameObjectWorld() const override;
#pragma endregion

public:
#pragma region PublicApi
	/** @brief Creates a server or returns the existing server with the same identifier. @param InOptions Server creation options. @param InServerClass Server UObject class. @param InClientClass Client UObject class. @param OutServer Receives the created or existing server. @return Creation result. */
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	FCefWebSocketServerCreateResult CreateOrGetServer(
		const FCefWebSocketServerCreateOptions& InOptions,
		TSubclassOf<UCefWebSocketServerBase> InServerClass,
		TSubclassOf<UCefWebSocketClientBase> InClientClass,
		UCefWebSocketServerBase*& OutServer);

	/** @brief Finds a server by logical identifier. @param InNameId Server identifier. @return Matching server, or nullptr. */
	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	UCefWebSocketServerBase* GetServer(FName InNameId) const;

	/** @brief Stops and removes a server. @param InNameId Server identifier. @return true when the server was found and stopped. */
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	bool StopServer(FName InNameId);

	/** @brief Stops and removes all servers owned by this game instance. */
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	void StopAllServers();

	/** @brief Returns all servers currently owned by this subsystem. */
	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	TArray<UCefWebSocketServerBase*> GetAllServers() const;
#pragma endregion

private:
#pragma region Internal
	/** @brief Grants the tests access to the non-public queue ownership seam. */
	friend class FCefWebSocketGameThreadEventTestAccess;

	/** @brief Resolves the requested port and reports whether it was adjusted. */
	bool TryResolvePort(int32 InRequestedPort, int32& OutResolvedPort, bool& bOutAdjusted) const;

	/** Servers owned by this game instance, keyed by logical identifier. */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UCefWebSocketServerBase>> Servers;

	/** @brief Starting index for the next round-robin dispatch pass. */
	int32 RoundRobinStartIndex = 0;
	/** @brief True after the game-instance subsystem has initialized. */
	bool bInitialized = false;
#pragma endregion
};

