#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/CefWebSocketStructs.h"
#include "Data/CefWebSocketEnums.h"
#include "CefWebSocketSubsystem.generated.h"

class UCefWebSocketServerBase;
class UCefWebSocketClientBase;

UCLASS()
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
		const FCefWebSocketServerCreateOptions& Options,
		TSubclassOf<UCefWebSocketServerBase> ServerClass,
		TSubclassOf<UCefWebSocketClientBase> ClientClass,
		UCefWebSocketServerBase*& OutServer);

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	UCefWebSocketServerBase* GetServer(FName NameId) const;

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	bool StopServer(FName NameId);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	void StopAllServers();

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	TArray<UCefWebSocketServerBase*> GetAllServers() const;
#pragma endregion

private:
#pragma region Internal
	bool TryResolvePort(int32 RequestedPort, int32& OutResolvedPort, bool& bOutAdjusted) const;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UCefWebSocketServerBase>> Servers;
#pragma endregion
};
