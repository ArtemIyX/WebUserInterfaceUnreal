/**
 * @file CefWebSocketServer\\Private\\Tests\\CefWebSocketGameThreadEventTestTypes.h
 * @brief Declares test-only WebSocket server and client types for event queue automation tests.
 */
#pragma once

#include "CoreMinimal.h"
#include "Server/CefWebSocketClientBase.h"
#include "Server/CefWebSocketServerBase.h"
#include "CefWebSocketGameThreadEventTestTypes.generated.h"

class UCefWebSocketGameThreadEventTestServer;

/** @brief Test client that records downstream byte and text callbacks. */
UCLASS()
class UCefWebSocketGameThreadEventTestClient : public UCefWebSocketClientBase
{
	GENERATED_BODY()

public:
	static UCefWebSocketGameThreadEventTestServer* ActiveServer;

	virtual void HandleBytesFromClient(const TArray<uint8>& InData) override;
	virtual void HandleStringFromClient(const FString& InMessage) override;
};

/** @brief Test server that records event order, payloads, and callback thread affinity. */
UCLASS()
class UCefWebSocketGameThreadEventTestServer : public UCefWebSocketServerBase
{
	GENERATED_BODY()

public:
	TArray<FString> Records;
	TArray<uint32> CallbackThreadIds;
	TArray<int64> MessageIds;
	TArray<TArray<uint8>> MessagePayloads;
	TArray<bool> MessageBinaryFlags;
	TArray<FString> MessageTexts;
	TArray<bool> ConnectedClientVisibleInCallbacks;
	TArray<bool> DisconnectedClientVisibleInCallbacks;
	bool bStopOnConnected = false;

	void ResetRecords();
	void RecordClientBytes();
	void RecordClientString(const FString& InMessage);

	UFUNCTION()
	void RecordConnectedDelegate(const FCefWebSocketClientInfo& InClientInfo);
	UFUNCTION()
	void RecordDisconnectedDelegate(int64 InClientId, ECefWebSocketCloseReason InReason);
	UFUNCTION()
	void RecordServerErrorDelegate(FName InServerName, ECefWebSocketErrorCode InErrorCode, const FString& InMessage);
	UFUNCTION()
	void RecordClientErrorDelegate(int64 InClientId, ECefWebSocketErrorCode InErrorCode, const FString& InMessage);

protected:
	virtual void HandleClientConnected(const FCefWebSocketClientInfo& InClientInfo) override;
	virtual void HandleClientDisconnected(int64 InClientId, ECefWebSocketCloseReason InReason) override;
	virtual void HandleServerError(ECefWebSocketErrorCode InErrorCode, const FString& InMessage) override;
	virtual void HandleClientError(int64 InClientId, ECefWebSocketErrorCode InErrorCode, const FString& InMessage) override;
	virtual void HandleClientMessage(int64 InClientId, const TArray<uint8>& InPayload, bool bInBinary) override;
	virtual void HandleClientBytes(UCefWebSocketClientBase* InClient, const TArray<uint8>& InData) override;
	virtual void HandleClientString(UCefWebSocketClientBase* InClient, const FString& InMessage) override;
	virtual void ServerInitialized(class UCefWebSocketSubsystem* InCefSubsystem) override;
};
