#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Data/CefWebSocketStructs.h"
#include "Data/CefWebSocketEnums.h"
#include "CefWebSocketClientBase.generated.h"

class UCefWebSocketServerBase;

UCLASS(BlueprintType, Blueprintable)
class CEFWEBSOCKETSERVER_API UCefWebSocketClientBase : public UObject
{
	GENERATED_BODY()

public:
#pragma region PublicApi
	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	int64 GetClientId() const { return ClientInfo.ClientId; }

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	FString GetRemoteAddress() const { return ClientInfo.RemoteAddress; }

	UFUNCTION(BlueprintPure, Category = "CefWebSocket")
	FDateTime GetConnectedAt() const { return ClientInfo.ConnectedAt; }

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult SendString(const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult SendBytes(const TArray<uint8>& Bytes);

	UFUNCTION(BlueprintCallable, Category = "CefWebSocket")
	ECefWebSocketSendResult Disconnect(ECefWebSocketCloseReason Reason = ECefWebSocketCloseReason::Kicked);

	virtual void HandleBytesFromClient(const TArray<uint8>& Data);
	virtual void HandleStringFromClient(const FString& Message);
#pragma endregion

private:
#pragma region Internal
	friend class UCefWebSocketServerBase;
	void InitializeClient(TWeakObjectPtr<UCefWebSocketServerBase> InOwnerServer, const FCefWebSocketClientInfo& InInfo);

	TWeakObjectPtr<UCefWebSocketServerBase> OwnerServer;
	FCefWebSocketClientInfo ClientInfo;
#pragma endregion
};
