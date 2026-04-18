#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Data/CefWebSocketDelegates.h"
#include "Data/CefWebSocketStructs.h"
#include "Data/CefWebSocketEnums.h"
#include "CefWebSocketServerBase.generated.h"

class UCefWebSocketClientBase;
class FCefWebSocketServerInstance;

UCLASS(BlueprintType, Blueprintable)
class CEFWEBSOCKETSERVER_API UCefWebSocketServerBase : public UObject
{
	GENERATED_BODY()

public:
#pragma region Lifecycle
	virtual void BeginDestroy() override;
#pragma endregion

public:
#pragma region BlueprintEvents
	UPROPERTY(BlueprintAssignable, Category = "CefWebSocket|Events")
	FCefWebSocketClientConnectedDynamic OnClientConnected;

	UPROPERTY(BlueprintAssignable, Category = "CefWebSocket|Events")
	FCefWebSocketClientDisconnectedDynamic OnClientDisconnected;

	UPROPERTY(BlueprintAssignable, Category = "CefWebSocket|Events")
	FCefWebSocketServerErrorDynamic OnServerError;

	UPROPERTY(BlueprintAssignable, Category = "CefWebSocket|Events")
	FCefWebSocketClientErrorDynamic OnClientError;
#pragma endregion

public:
#pragma region PublicApi
	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	FName GetServerNameId() const { return NameId; }

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	int32 GetBoundPort() const { return BoundPort; }

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	bool IsRunning() const;

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult SendToClientString(int64 ClientId, const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult SendToClientBytes(int64 ClientId, const TArray<uint8>& Bytes);

	ECefWebSocketSendResult SendToClientJson(int64 ClientId, const TSharedPtr<FJsonObject>& JsonObject, const TSharedPtr<FJsonValue>& JsonValue = nullptr);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult BroadcastString(const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult BroadcastBytes(const TArray<uint8>& Bytes);

	ECefWebSocketSendResult BroadcastJson(const TSharedPtr<FJsonObject>& JsonObject, const TSharedPtr<FJsonValue>& JsonValue = nullptr);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult BroadcastStringExcept(int64 ExcludedClientId, const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult BroadcastBytesExcept(int64 ExcludedClientId, const TArray<uint8>& Bytes);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult DisconnectClient(int64 ClientId, ECefWebSocketCloseReason Reason = ECefWebSocketCloseReason::Kicked);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	void StopServer();

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	TArray<UCefWebSocketClientBase*> GetClients() const;

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	UCefWebSocketClientBase* GetClient(int64 ClientId) const;

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	FCefWebSocketServerStats GetStats() const;

	virtual void HandleClientBytes(UCefWebSocketClientBase* Client, const TArray<uint8>& Data);
	virtual void HandleClientString(UCefWebSocketClientBase* Client, const FString& Message);
#pragma endregion

protected:
#pragma region InternalApi
	friend class UCefWebSocketSubsystem;
	bool StartServerInternal(FName InNameId, int32 InBoundPort, TSubclassOf<UCefWebSocketClientBase> InClientClass);
	void AttachInstance(TSharedPtr<FCefWebSocketServerInstance> InInstance);
#pragma endregion

private:
#pragma region InternalState
	FName NameId = NAME_None;
	int32 BoundPort = 0;
	TSubclassOf<UCefWebSocketClientBase> ClientClass;
	TMap<int64, TObjectPtr<UCefWebSocketClientBase>> ClientObjects;
	TSharedPtr<FCefWebSocketServerInstance> Instance;
#pragma endregion
};
