/**
 * @file CefWebSocketServer\Public\Server\CefWebSocketServerBase.h
 * @brief Declares CefWebSocketServerBase for module CefWebSocketServer\Public\Server\CefWebSocketServerBase.h.
 * @details Contains websocket server components used by the plugin runtime and gameplay-facing systems.
 */
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
/** @brief Type declaration. */
class CEFWEBSOCKETSERVER_API UCefWebSocketServerBase : public UObject
{
	GENERATED_BODY()

public:
	/** @brief UCefWebSocketServerBase API. */
	UCefWebSocketServerBase(const FObjectInitializer& ObjectInitializer);
public:
#pragma region Lifecycle
	virtual void BeginDestroy() override;
#pragma endregion

public:
#pragma region BlueprintEvents
	UPROPERTY(BlueprintAssignable, Category = "CefWebSocket|Events")
	/** @brief OnClientConnected state. */
	FCefWebSocketClientConnectedDynamic OnClientConnected;

	UPROPERTY(BlueprintAssignable, Category = "CefWebSocket|Events")
	/** @brief OnClientDisconnected state. */
	FCefWebSocketClientDisconnectedDynamic OnClientDisconnected;

	UPROPERTY(BlueprintAssignable, Category = "CefWebSocket|Events")
	/** @brief OnServerError state. */
	FCefWebSocketServerErrorDynamic OnServerError;

	UPROPERTY(BlueprintAssignable, Category = "CefWebSocket|Events")
	/** @brief OnClientError state. */
	FCefWebSocketClientErrorDynamic OnClientError;
#pragma endregion

public:
#pragma region PublicApi
	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	FName GetServerNameId() const { return NameId; }

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	int32 GetBoundPort() const { return BoundPort; }

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	/** @brief IsRunning API. */
	bool IsRunning() const;

	UFUNCTION(BlueprintPure, Category = "CefWebSocket|Pipeline")
	ECefWebSocketPayloadFormat GetPayloadFormat() const { return PayloadFormat; }

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	/** @brief SendToClientString API. */
	ECefWebSocketSendResult SendToClientString(int64 InClientId, const FString& InMessage);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	/** @brief SendToClientBytes API. */
	ECefWebSocketSendResult SendToClientBytes(int64 InClientId, const TArray<uint8>& InBytes);

	/** @brief SendToClientJson API. */
	ECefWebSocketSendResult SendToClientJson(int64 InClientId, const TSharedPtr<FJsonObject>& InJsonObject, const TSharedPtr<FJsonValue>& InJsonValue = nullptr);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	/** @brief BroadcastString API. */
	ECefWebSocketSendResult BroadcastString(const FString& InMessage);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	/** @brief BroadcastBytes API. */
	ECefWebSocketSendResult BroadcastBytes(const TArray<uint8>& InBytes);

	/** @brief BroadcastJson API. */
	ECefWebSocketSendResult BroadcastJson(const TSharedPtr<FJsonObject>& InJsonObject, const TSharedPtr<FJsonValue>& InJsonValue = nullptr);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	/** @brief BroadcastStringExcept API. */
	ECefWebSocketSendResult BroadcastStringExcept(int64 InExcludedClientId, const FString& InMessage);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	/** @brief BroadcastBytesExcept API. */
	ECefWebSocketSendResult BroadcastBytesExcept(int64 InExcludedClientId, const TArray<uint8>& InBytes);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	/** @brief DisconnectClient API. */
	ECefWebSocketSendResult DisconnectClient(int64 InClientId, ECefWebSocketCloseReason InReason = ECefWebSocketCloseReason::Kicked);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	/** @brief StopServer API. */
	void StopServer();

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	/** @brief GetClients API. */
	TArray<UCefWebSocketClientBase*> GetClients() const;

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	/** @brief GetClient API. */
	UCefWebSocketClientBase* GetClient(int64 InClientId) const;

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	/** @brief GetStats API. */
	FCefWebSocketServerStats GetStats() const;

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket|Pipeline")
	/** @brief SetPayloadFormat API. */
	void SetPayloadFormat(ECefWebSocketPayloadFormat InPayloadFormat);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket|Pipeline")
	/** @brief SetPipelineConfig API. */
	void SetPipelineConfig(const FCefWebSocketPipelineConfig& InPipelineConfig);

	/** @brief SetPacketCodec API. */
	void SetPacketCodec(const TSharedPtr<ICefWebSocketPacketCodec>& InCodec);

	/** @brief HandleClientBytes API. */
	virtual void HandleClientBytes(UCefWebSocketClientBase* InClient, const TArray<uint8>& InData);
	/** @brief HandleClientString API. */
	virtual void HandleClientString(UCefWebSocketClientBase* InClient, const FString& InMessage);
#pragma endregion

protected:
#pragma region InternalApi
	/** @brief UCefWebSocketSubsystem state. */
	friend class UCefWebSocketSubsystem;
	/** @brief FCefWebSocketServerInstance state. */
	friend class FCefWebSocketServerInstance;
	bool StartServerInternal(FName InNameId, int32 InBoundPort, TSubclassOf<UCefWebSocketClientBase> InClientClass,
	                         const FCefWebSocketPipelineConfig& InPipelineConfig);
	/** @brief AttachInstance API. */
	void AttachInstance(TSharedPtr<FCefWebSocketServerInstance> InInstance);
	/** @brief NotifyClientConnected API. */
	void NotifyClientConnected(const FCefWebSocketClientInfo& InClientInfo);
	/** @brief NotifyClientDisconnected API. */
	void NotifyClientDisconnected(int64 InClientId, ECefWebSocketCloseReason InReason);
	/** @brief NotifyServerError API. */
	void NotifyServerError(ECefWebSocketErrorCode InErrorCode, const FString& InMessage);
	/** @brief NotifyClientError API. */
	void NotifyClientError(int64 InClientId, ECefWebSocketErrorCode InErrorCode, const FString& InMessage);
	/** @brief NotifyClientMessage API. */
	void NotifyClientMessage(int64 InClientId, const TArray<uint8>& InPayload, bool bInBinary);
#pragma endregion

private:
#pragma region InternalState
	/** @brief NameId state. */
	FName NameId = NAME_None;
	/** @brief BoundPort state. */
	int32 BoundPort = 0;
	/** @brief ClientClass state. */
	TSubclassOf<UCefWebSocketClientBase> ClientClass;
	/** @brief PayloadFormat state. */
	ECefWebSocketPayloadFormat PayloadFormat = ECefWebSocketPayloadFormat::Binary;
	/** @brief PipelineConfig state. */
	FCefWebSocketPipelineConfig PipelineConfig;
	/** @brief PacketCodec state. */
	TSharedPtr<ICefWebSocketPacketCodec> PacketCodec;
	/** @brief ClientObjectsLock state. */
	mutable FCriticalSection ClientObjectsLock;
	/** @brief ClientObjects state. */
	TMap<int64, TObjectPtr<UCefWebSocketClientBase>> ClientObjects;
	/** @brief Instance state. */
	TSharedPtr<FCefWebSocketServerInstance> Instance;
#pragma endregion
};

