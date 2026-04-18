#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Data/CefWebSocketStructs.h"
#include "Data/CefWebSocketEnums.h"

class ICefNetWebSocket;
class ICefWebSocketServerBackend;
class UCefWebSocketServerBase;

class FCefWebSocketServerInstance : public TSharedFromThis<FCefWebSocketServerInstance>
{
public:
	FCefWebSocketServerInstance(FName InNameId, int32 InBoundPort, TWeakObjectPtr<UCefWebSocketServerBase> InOwnerServer);
	~FCefWebSocketServerInstance();

	bool Start();
	void Stop();
	bool IsRunning() const;

	ECefWebSocketSendResult SendToClientString(int64 ClientId, const FString& Message);
	ECefWebSocketSendResult SendToClientBytes(int64 ClientId, const TArray<uint8>& Bytes);
	ECefWebSocketSendResult BroadcastString(const FString& Message);
	ECefWebSocketSendResult BroadcastBytes(const TArray<uint8>& Bytes);
	ECefWebSocketSendResult BroadcastStringExcept(int64 ExcludedClientId, const FString& Message);
	ECefWebSocketSendResult BroadcastBytesExcept(int64 ExcludedClientId, const TArray<uint8>& Bytes);
	ECefWebSocketSendResult DisconnectClient(int64 ClientId, ECefWebSocketCloseReason Reason);

	TArray<FCefWebSocketClientInfo> GetClients() const;
	FCefWebSocketServerStats GetStats() const;
	FCefWebSocketClientInfo FindClientInfo(int64 ClientId) const;

private:
	struct FCefClientState
	{
		FCefWebSocketClientInfo Info;
		ICefNetWebSocket* Socket = nullptr;
	};

	bool TickServer(float DeltaTime);
	void HandleClientConnected(ICefNetWebSocket* Socket);
	void HandleClientDisconnected(ICefNetWebSocket* Socket);
	void HandleClientPacket(ICefNetWebSocket* Socket, const uint8* Data, int32 Count, bool bBinary);
	ECefWebSocketSendResult SendToSocket(ICefNetWebSocket* Socket, const uint8* Data, int32 Count);

	FName NameId = NAME_None;
	int32 BoundPort = 0;
	TWeakObjectPtr<UCefWebSocketServerBase> OwnerServer;
	TAtomic<bool> bRunning = false;
	mutable FCriticalSection ClientLock;
	TMap<int64, FCefClientState> Clients;
	TMap<ICefNetWebSocket*, int64> ClientIdsBySocket;
	TUniquePtr<ICefWebSocketServerBackend> Backend;
	FTSTicker::FDelegateHandle TickHandle;
	int64 NextClientId = 1;
	FCefWebSocketServerStats Stats;
};
