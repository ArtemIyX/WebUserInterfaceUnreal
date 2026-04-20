/**
 * @file CefWebSocketServer\Public\Server\CefWebSocketClientBase.h
 * @brief Declares CefWebSocketClientBase for module CefWebSocketServer\Public\Server\CefWebSocketClientBase.h.
 * @details Contains websocket server components used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Data/CefWebSocketStructs.h"
#include "Data/CefWebSocketEnums.h"
#include "CefWebSocketClientBase.generated.h"

class UCefWebSocketServerBase;

UCLASS(BlueprintType, Blueprintable)
/** @brief Type declaration. */
class CEFWEBSOCKETSERVER_API UCefWebSocketClientBase : public UObject
{
	GENERATED_BODY()

public:
	/** @brief UCefWebSocketClientBase API. */
	UCefWebSocketClientBase(const FObjectInitializer& InInitializer);
public:
#pragma region PublicApi
	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	int64 GetClientId() const { return ClientInfo.ClientId; }

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	FString GetRemoteAddress() const { return ClientInfo.RemoteAddress; }

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	FDateTime GetConnectedAt() const { return ClientInfo.ConnectedAt; }

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	/** @brief SendString API. */
	ECefWebSocketSendResult SendString(const FString& InMessage);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	/** @brief SendBytes API. */
	ECefWebSocketSendResult SendBytes(const TArray<uint8>& InBytes);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	/** @brief Disconnect API. */
	ECefWebSocketSendResult Disconnect(ECefWebSocketCloseReason InReason = ECefWebSocketCloseReason::Kicked);

	/** @brief HandleBytesFromClient API. */
	virtual void HandleBytesFromClient(const TArray<uint8>& InData);
	/** @brief HandleStringFromClient API. */
	virtual void HandleStringFromClient(const FString& InMessage);
#pragma endregion

private:
#pragma region Internal
	/** @brief UCefWebSocketServerBase state. */
	friend class UCefWebSocketServerBase;
	/** @brief InitializeClient API. */
	void InitializeClient(TWeakObjectPtr<UCefWebSocketServerBase> InOwnerServer, const FCefWebSocketClientInfo& InInfo);

	/** @brief OwnerServer state. */
	TWeakObjectPtr<UCefWebSocketServerBase> OwnerServer;
	/** @brief ClientInfo state. */
	FCefWebSocketClientInfo ClientInfo;
#pragma endregion
};

