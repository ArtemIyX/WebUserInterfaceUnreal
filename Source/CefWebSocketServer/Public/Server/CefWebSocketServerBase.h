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
#include "Pipeline/CefWebSocketPipelineTypes.h"
#include "CefWebSocketServerBase.generated.h"

class UCefWebSocketSubsystem;
class UCefWebSocketClientBase;
class FCefWebSocketServerInstance;
class ICefWebSocketPacketCodec;

/** @brief Type declaration. */
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
	/** Fired after a client connection has been accepted and its client object initialized. */
	UPROPERTY(BlueprintAssignable, Category = "CefWebSocket|Events")
	FCefWebSocketClientConnectedDynamic OnClientConnected;

	/** Fired after a client has disconnected from the server. */
	UPROPERTY(BlueprintAssignable, Category = "CefWebSocket|Events")
	FCefWebSocketClientDisconnectedDynamic OnClientDisconnected;

	/** Fired when the server reports a server-level error. */
	UPROPERTY(BlueprintAssignable, Category = "CefWebSocket|Events")
	FCefWebSocketServerErrorDynamic OnServerError;

	/** Fired when the server reports an error associated with a specific client. */
	UPROPERTY(BlueprintAssignable, Category = "CefWebSocket|Events")
	FCefWebSocketClientErrorDynamic OnClientError;
#pragma endregion

public:
#pragma region PublicApi
	/** @brief Returns the identifier assigned to this server instance. @return Server identifier, or NAME_None before initialization. */
	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	FName GetServerNameId() const { return NameId; }

	/** @brief Returns the configured port. @return Requested or assigned TCP port, or zero before configuration. */
	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	int32 GetBoundPort() const { return BoundPort; }

	/**
	 * @brief Reports whether the underlying WebSocket server is currently running.
	 * @return true when the runtime server exists and is running; otherwise false.
	 */
	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	bool IsRunning() const;

	/** @brief Returns the payload format applied to outgoing and incoming WebSocket messages. @return Current payload format. */
	UFUNCTION(BlueprintPure, Category = "CefWebSocket|Pipeline")
	ECefWebSocketPayloadFormat GetPayloadFormat() const { return PayloadFormat; }

	/**
	 * @brief Sends a UTF-8 string message to one connected client.
	 * @param InClientId Identifier of the destination client.
	 * @param InMessage Message text to send.
	 * @return The send result, including invalid-client, disconnected, queue, and transport failures.
	 */
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult SendToClientString(int64 InClientId, const FString& InMessage);

	/**
	 * @brief Sends binary data to one connected client.
	 * @param InClientId Identifier of the destination client.
	 * @param InBytes Payload bytes to send.
	 * @return The send result. The payload may still be transformed by the configured packet codec or pipeline.
	 */
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult SendToClientBytes(int64 InClientId, const TArray<uint8>& InBytes);

	/**
	 * @brief Serializes and sends a JSON object or value to one connected client.
	 * @param InClientId Identifier of the destination client.
	 * @param InJsonObject Object to serialize. May be null when InJsonValue is supplied.
	 * @param InJsonValue Value to serialize when InJsonObject is null.
	 * @return SerializeFailed when neither input is valid or serialization fails; otherwise the underlying send result.
	 */
	ECefWebSocketSendResult SendToClientJson(int64 InClientId, const TSharedPtr<FJsonObject>& InJsonObject, const TSharedPtr<FJsonValue>& InJsonValue = nullptr);

	/**
	 * @brief Sends a UTF-8 string message to every connected client.
	 * @param InMessage Message text to broadcast.
	 * @return The aggregate broadcast send result, or InvalidServer when the server is not running.
	 */
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult BroadcastString(const FString& InMessage);

	/**
	 * @brief Sends binary data to every connected client.
	 * @param InBytes Payload bytes to broadcast.
	 * @return The aggregate broadcast send result, or InvalidServer when the server is not running.
	 */
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult BroadcastBytes(const TArray<uint8>& InBytes);

	/**
	 * @brief Serializes and broadcasts a JSON object or value to every connected client.
	 * @param InJsonObject Object to serialize. May be null when InJsonValue is supplied.
	 * @param InJsonValue Value to serialize when InJsonObject is null.
	 * @return SerializeFailed when neither input is valid or serialization fails; otherwise the underlying broadcast result.
	 */
	ECefWebSocketSendResult BroadcastJson(const TSharedPtr<FJsonObject>& InJsonObject, const TSharedPtr<FJsonValue>& InJsonValue = nullptr);

	/**
	 * @brief Sends a UTF-8 string message to every connected client except one.
	 * @param InExcludedClientId Client identifier excluded from delivery.
	 * @param InMessage Message text to broadcast.
	 * @return The broadcast send result, or InvalidServer when the server is not running.
	 */
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult BroadcastStringExcept(int64 InExcludedClientId, const FString& InMessage);

	/**
	 * @brief Sends binary data to every connected client except one.
	 * @param InExcludedClientId Client identifier excluded from delivery.
	 * @param InBytes Payload bytes to broadcast.
	 * @return The broadcast send result, or InvalidServer when the server is not running.
	 */
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult BroadcastBytesExcept(int64 InExcludedClientId, const TArray<uint8>& InBytes);

	/**
	 * @brief Disconnects a connected client and reports the requested close reason.
	 * @param InClientId Identifier of the client to disconnect.
	 * @param InReason Close reason sent to the client and used by the server runtime.
	 * @return Ok when the disconnect was queued; otherwise the relevant server, client, or transport failure.
	 */
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult DisconnectClient(int64 InClientId, ECefWebSocketCloseReason InReason = ECefWebSocketCloseReason::Kicked);

	/**
	 * @brief Stops the underlying server if it is running.
	 * @note This is safe to call when the server is already stopped.
	 */
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	void StopServer();

	/**
	 * @brief Returns the currently known client objects.
	 * @return A snapshot array of connected or still-being-cleaned-up client objects. The caller does not own the objects.
	 */
	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	TArray<UCefWebSocketClientBase*> GetClients() const;

	/**
	 * @brief Looks up a client by identifier.
	 * @param InClientId Identifier returned by the client object and connection event.
	 * @return The matching client object, or nullptr when no client is registered with that identifier.
	 */
	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	UCefWebSocketClientBase* GetClient(int64 InClientId) const;

	/**
	 * @brief Returns runtime connection and traffic statistics for this server.
	 * @return A value snapshot of the server statistics.
	 */
	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	FCefWebSocketServerStats GetStats() const;

	/**
	 * @brief Sets the wire payload format used by the server pipeline.
	 * @param InPayloadFormat New payload format.
	 * @note The setting is forwarded to the active runtime server and affects subsequent messages.
	 */
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket|Pipeline")
	void SetPayloadFormat(ECefWebSocketPayloadFormat InPayloadFormat);

	/**
	 * @brief Replaces the server pipeline configuration.
	 * @param InPipelineConfig Configuration controlling processing stages for WebSocket payloads.
	 * @note The configuration is applied to the active runtime server when one exists and is also retained for the next start.
	 */
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket|Pipeline")
	void SetPipelineConfig(const FCefWebSocketPipelineConfig& InPipelineConfig);

	/**
	 * @brief Sets the optional packet codec used to transform WebSocket payloads.
	 * @param InCodec Codec shared by the server runtime. Passing an invalid pointer disables the codec.
	 * @note The codec is retained by shared ownership and is applied to subsequent server traffic.
	 */
	void SetPacketCodec(const TSharedPtr<ICefWebSocketPacketCodec>& InCodec);

protected:
	/**
	 * @brief Handles a binary message delivered by the server runtime.
	 * @param InClient Client that sent the message.
	 * @param InData Decoded binary payload.
	 * @note Override this to implement application-level binary message handling. The base implementation is a no-op.
	 */
	virtual void HandleClientBytes(UCefWebSocketClientBase* InClient, const TArray<uint8>& InData);
	/**
	 * @brief Handles a text message delivered by the server runtime.
	 * @param InClient Client that sent the message.
	 * @param InMessage Decoded text payload.
	 * @note Override this to implement application-level text message handling. The base implementation is a no-op.
	 */
	virtual void HandleClientString(UCefWebSocketClientBase* InClient, const FString& InMessage);
	
	/**
	 * @brief Notifies the derived server object that the runtime server has initialized.
	 * @param InCefSubsystem WebSocket subsystem that owns the initialized runtime.
	 * @note This is called by the subsystem before normal client traffic is processed. The base implementation is a no-op.
	 */
	virtual void ServerInitialized(UCefWebSocketSubsystem* InCefSubsystem);
#pragma endregion

protected:
#pragma region InternalApi
	/** @brief UCefWebSocketSubsystem state. */
	friend class UCefWebSocketSubsystem;
	/** @brief FCefWebSocketServerInstance state. */
	friend class FCefWebSocketServerInstance;
	/**
	 * @brief Starts the internal server runtime with the supplied configuration.
	 * @param InNameId Stable identifier used to identify this server.
	 * @param InBoundPort Requested local TCP port.
	 * @param InClientClass UObject class used to create client instances.
	 * @param InPipelineConfig Payload pipeline configuration.
	 * @return true when startup succeeds; otherwise false and the runtime reports the startup error.
	 * @note This is an internal entry point. Callers should normally use the WebSocket subsystem rather than invoking it directly.
	 */
	bool StartServerInternal(FName InNameId, int32 InBoundPort, TSubclassOf<UCefWebSocketClientBase> InClientClass,
	                         const FCefWebSocketPipelineConfig& InPipelineConfig);
	/**
	 * @brief Attaches an already-created runtime server instance to this UObject.
	 * @param InInstance Runtime instance to associate with this server.
	 * @note This transfers the shared reference and is intended for internal subsystem coordination.
	 */
	void AttachInstance(const TSharedPtr<FCefWebSocketServerInstance>& InInstance);
	/**
	 * @brief Updates server state and emits OnClientConnected for a newly connected client.
	 * @param InClientInfo Identity and connection metadata for the client.
	 */
	void NotifyClientConnected(const FCefWebSocketClientInfo& InClientInfo);
	/**
	 * @brief Updates server state and emits OnClientDisconnected for a disconnected client.
	 * @param InClientId Identifier of the disconnected client.
	 * @param InReason Reason reported for the disconnection.
	 */
	void NotifyClientDisconnected(int64 InClientId, ECefWebSocketCloseReason InReason);
	/**
	 * @brief Emits OnServerError for an error reported by the runtime server.
	 * @param InErrorCode Runtime error code.
	 * @param InMessage Human-readable error description.
	 */
	void NotifyServerError(ECefWebSocketErrorCode InErrorCode, const FString& InMessage);
	/**
	 * @brief Emits OnClientError for an error associated with a client.
	 * @param InClientId Identifier of the affected client.
	 * @param InErrorCode Runtime error code.
	 * @param InMessage Human-readable error description.
	 */
	void NotifyClientError(int64 InClientId, ECefWebSocketErrorCode InErrorCode, const FString& InMessage);
	/**
	 * @brief Routes a runtime client message to the appropriate text or binary handler.
	 * @param InClientId Identifier of the sending client.
	 * @param InPayload Message bytes, regardless of the original wire representation.
	 * @param bInBinary true for binary data; false for text data encoded as bytes.
	 */
	void NotifyClientMessage(int64 InClientId, const TArray<uint8>& InPayload, bool bInBinary);
	
	/**
	 * @brief Receives a newly connected client notification after base bookkeeping.
	 * @param InClientInfo Identity and connection metadata for the client.
	 * @note The base implementation does nothing. Override to add server-specific connection handling.
	 */
	virtual void HandleClientConnected(const FCefWebSocketClientInfo& InClientInfo);
	/**
	 * @brief Receives a client disconnection notification after base bookkeeping.
	 * @param InClientId Identifier of the disconnected client.
	 * @param InReason Reason reported for the disconnection.
	 * @note The base implementation does nothing. Override to add server-specific cleanup.
	 */
	virtual void HandleClientDisconnected(int64 InClientId, ECefWebSocketCloseReason InReason);
	/**
	 * @brief Receives a server-level runtime error after the error delegate is notified.
	 * @param InErrorCode Runtime error code.
	 * @param InMessage Human-readable error description.
	 * @note The base implementation does nothing. Override to add application logging or recovery.
	 */
	virtual void HandleServerError(ECefWebSocketErrorCode InErrorCode, const FString& InMessage);
	/**
	 * @brief Receives a client-specific runtime error after the error delegate is notified.
	 * @param InClientId Identifier of the affected client.
	 * @param InErrorCode Runtime error code.
	 * @param InMessage Human-readable error description.
	 * @note The base implementation does nothing. Override to add application logging or recovery.
	 */
	virtual void HandleClientError(int64 InClientId, ECefWebSocketErrorCode InErrorCode, const FString& InMessage);
	/**
	 * @brief Receives a client message after the runtime has identified its wire type.
	 * @param InClientId Identifier of the sending client.
	 * @param InPayload Message bytes.
	 * @param bInBinary true for binary data; false for text data.
	 * @note The base implementation does nothing. Override to implement application protocol handling.
	 */
	virtual void HandleClientMessage(int64 InClientId, const TArray<uint8>& InPayload, bool bInBinary);
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
	UPROPERTY()
	TMap<int64, TObjectPtr<UCefWebSocketClientBase>> ClientObjects;
	/** @brief Messages received before their game-thread client object was created. */
	TMap<int64, TArray<FCefWebSocketInboundPacket>> PendingClientMessages;
	/** @brief Instance state. */
	TSharedPtr<FCefWebSocketServerInstance> Instance;
#pragma endregion
};

