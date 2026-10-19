/**
 * @file CefWebSocketServer\Public\Server\CefWebSocketClientBase.h
 * @brief Declares the UObject wrapper for a connected WebSocket client.
 */
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Data/CefWebSocketStructs.h"
#include "Data/CefWebSocketEnums.h"
#include "CefWebSocketClientBase.generated.h"

class UCefWebSocketServerBase;

/** @brief UObject representation of one connected WebSocket client. */
UCLASS(BlueprintType, Blueprintable)
class CEFWEBSOCKETSERVER_API UCefWebSocketClientBase : public UObject
{
	GENERATED_BODY()

public:
	/** @brief Constructs the client wrapper. @param InInitializer Unreal object initializer. */
	UCefWebSocketClientBase(const FObjectInitializer& InInitializer);
public:
#pragma region PublicApi
	/** @brief Returns the server-assigned client identifier. */
	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	int64 GetClientId() const { return ClientInfo.ClientId; }

	/** @brief Returns the client's remote network address. */
	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	FString GetRemoteAddress() const { return ClientInfo.RemoteAddress; }

	/** @brief Returns the time at which the client connected. */
	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	FDateTime GetConnectedAt() const { return ClientInfo.ConnectedAt; }

	/** @brief Sends a UTF-8 string to this client. @param InMessage Message text. @return Result of the send operation. */
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult SendString(const FString& InMessage);

	/** @brief Sends binary data to this client. @param InBytes Payload bytes. @return Result of the send operation. */
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult SendBytes(const TArray<uint8>& InBytes);

	/** @brief Disconnects this client. @param InReason Close reason. @return Result of the disconnect operation. */
	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult Disconnect(ECefWebSocketCloseReason InReason = ECefWebSocketCloseReason::Kicked);

	/** @brief Handles binary data received from this client. @param InData Received bytes. */
	virtual void HandleBytesFromClient(const TArray<uint8>& InData);
	/** @brief Handles text data received from this client. @param InMessage Received text. */
	virtual void HandleStringFromClient(const FString& InMessage);
#pragma endregion

private:
#pragma region Internal
	/** Grants the owning server access to internal client state. */
	friend class UCefWebSocketServerBase;
	/** Initializes this wrapper with its owning server and connection information. */
	void InitializeClient(TWeakObjectPtr<UCefWebSocketServerBase> InOwnerServer, const FCefWebSocketClientInfo& InInfo);

	/** Server that owns this client wrapper. */
	TWeakObjectPtr<UCefWebSocketServerBase> OwnerServer;
	/** Connection identity and timing information. */
	FCefWebSocketClientInfo ClientInfo;
#pragma endregion
};

