#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Data/CefWebSocketDelegates.h"
#include "Data/CefWebSocketStructs.h"
#include "Data/CefWebSocketEnums.h"
#include "CefWebSocketServerBase.generated.h"

class UCefWebSocketClientBase;
class FCefWebSocketServerInstance;
class ICefWebSocketPacketCodec;

UCLASS(BlueprintType, Blueprintable)
class CEFWEBSOCKETSERVER_API UCefWebSocketServerBase : public UObject
{
	GENERATED_BODY()

public:
	UCefWebSocketServerBase(const FObjectInitializer& ObjectInitializer);
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

	UFUNCTION(BlueprintPure, Category = "CefWebSocket|Pipeline")
	ECefWebSocketPayloadFormat GetPayloadFormat() const { return PayloadFormat; }

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult SendToClientString(int64 InClientId, const FString& InMessage);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult SendToClientBytes(int64 InClientId, const TArray<uint8>& InBytes);

	ECefWebSocketSendResult SendToClientJson(int64 InClientId, const TSharedPtr<FJsonObject>& InJsonObject, const TSharedPtr<FJsonValue>& InJsonValue = nullptr);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult BroadcastString(const FString& InMessage);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult BroadcastBytes(const TArray<uint8>& InBytes);

	ECefWebSocketSendResult BroadcastJson(const TSharedPtr<FJsonObject>& InJsonObject, const TSharedPtr<FJsonValue>& InJsonValue = nullptr);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult BroadcastStringExcept(int64 InExcludedClientId, const FString& InMessage);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult BroadcastBytesExcept(int64 InExcludedClientId, const TArray<uint8>& InBytes);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult DisconnectClient(int64 InClientId, ECefWebSocketCloseReason InReason = ECefWebSocketCloseReason::Kicked);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	void StopServer();

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	TArray<UCefWebSocketClientBase*> GetClients() const;

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	UCefWebSocketClientBase* GetClient(int64 InClientId) const;

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	FCefWebSocketServerStats GetStats() const;

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket|Pipeline")
	void SetPayloadFormat(ECefWebSocketPayloadFormat InPayloadFormat);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket|Pipeline")
	void SetPipelineConfig(const FCefWebSocketPipelineConfig& InPipelineConfig);

	void SetPacketCodec(const TSharedPtr<ICefWebSocketPacketCodec>& InCodec);

	virtual void HandleClientBytes(UCefWebSocketClientBase* InClient, const TArray<uint8>& InData);
	virtual void HandleClientString(UCefWebSocketClientBase* InClient, const FString& InMessage);
#pragma endregion

protected:
#pragma region InternalApi
	friend class UCefWebSocketSubsystem;
	friend class FCefWebSocketServerInstance;
	bool StartServerInternal(FName InNameId, int32 InBoundPort, TSubclassOf<UCefWebSocketClientBase> InClientClass,
	                         ECefWebSocketPayloadFormat InPayloadFormat,
	                         const FCefWebSocketPipelineConfig& InPipelineConfig);
	void AttachInstance(TSharedPtr<FCefWebSocketServerInstance> InInstance);
	void NotifyClientConnected(const FCefWebSocketClientInfo& InClientInfo);
	void NotifyClientDisconnected(int64 InClientId, ECefWebSocketCloseReason InReason);
	void NotifyServerError(ECefWebSocketErrorCode InErrorCode, const FString& InMessage);
	void NotifyClientError(int64 InClientId, ECefWebSocketErrorCode InErrorCode, const FString& InMessage);
	void NotifyClientMessage(int64 InClientId, const TArray<uint8>& InPayload, bool bInBinary);
#pragma endregion

private:
#pragma region InternalState
	FName NameId = NAME_None;
	int32 BoundPort = 0;
	TSubclassOf<UCefWebSocketClientBase> ClientClass;
	ECefWebSocketPayloadFormat PayloadFormat = ECefWebSocketPayloadFormat::Binary;
	FCefWebSocketPipelineConfig PipelineConfig;
	TSharedPtr<ICefWebSocketPacketCodec> PacketCodec;
	mutable FCriticalSection ClientObjectsLock;
	TMap<int64, TObjectPtr<UCefWebSocketClientBase>> ClientObjects;
	TSharedPtr<FCefWebSocketServerInstance> Instance;
#pragma endregion
};
