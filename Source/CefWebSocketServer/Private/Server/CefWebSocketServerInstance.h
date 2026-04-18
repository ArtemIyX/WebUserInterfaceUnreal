#pragma once

#include "CoreMinimal.h"
#include "Data/CefWebSocketStructs.h"
#include "Data/CefWebSocketEnums.h"
#include "Containers/Queue.h"

class ICefNetWebSocket;
class ICefWebSocketServerBackend;
class FCefWebSocketReadRunnable;
class FCefWebSocketWriteRunnable;
class FRunnableThread;
class UCefWebSocketServerBase;

class FCefWebSocketServerInstance : public TSharedFromThis<FCefWebSocketServerInstance>
{
public:
#pragma region Lifecycle
	FCefWebSocketServerInstance(FName InNameId, int32 InBoundPort, TWeakObjectPtr<UCefWebSocketServerBase> InOwnerServer);
	~FCefWebSocketServerInstance();
#pragma endregion

public:
#pragma region PublicApi
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
	FName GetNameId() const { return NameId; }
	int32 GetBoundPort() const { return BoundPort; }
#pragma endregion

public:
#pragma region ThreadApi
	void TickBackendOnReadThread();
	bool PumpOutgoingOnWriteThread();
#pragma endregion

private:
#pragma region InternalTypes
	struct FCefOutboundMessage
	{
		TArray<uint8> Payload;
		bool bBinary = true;
	};

	struct FCefClientState
	{
		FCefWebSocketClientInfo Info;
		ICefNetWebSocket* Socket = nullptr;
		TQueue<FCefOutboundMessage, EQueueMode::Mpsc> Outbox;
		int64 QueueBytes = 0;
		int32 QueueMessages = 0;
	};
#pragma endregion

private:
#pragma region InternalMethods
	void HandleClientConnected(ICefNetWebSocket* Socket);
	void HandleClientDisconnected(ICefNetWebSocket* Socket);
	void HandleClientPacket(ICefNetWebSocket* Socket, const uint8* Data, int32 Count, bool bBinary);
	ECefWebSocketSendResult EnqueueToClient(int64 ClientId, const uint8* Data, int32 Count, bool bBinary);
	ECefWebSocketSendResult EnqueueToClients(const TArray<int64>& ClientIds, const uint8* Data, int32 Count, bool bBinary);
	void RecordQueueDepthSample_NoLock();
	void UpdateRateStats_NoLock();
	void WakeWriteThread();
#pragma endregion

private:
#pragma region InternalState
	FName NameId = NAME_None;
	int32 BoundPort = 0;
	TWeakObjectPtr<UCefWebSocketServerBase> OwnerServer;
	TAtomic<bool> bRunning = false;
	mutable FCriticalSection ClientLock;
	TMap<int64, FCefClientState> Clients;
	TMap<ICefNetWebSocket*, int64> ClientIdsBySocket;
	TUniquePtr<ICefWebSocketServerBackend> Backend;
	int64 NextClientId = 1;
	FCefWebSocketServerStats Stats;
	double LastRateSampleTimeSec = 0.0;
	int64 LastRateRxBytes = 0;
	int64 LastRateTxBytes = 0;
	int64 QueueDepthAccum = 0;
	int64 QueueDepthSamples = 0;

	TUniquePtr<FCefWebSocketReadRunnable> ReadRunnable;
	TUniquePtr<FCefWebSocketWriteRunnable> WriteRunnable;
	FRunnableThread* ReadThread = nullptr;
	FRunnableThread* WriteThread = nullptr;
	FEvent* WriteWakeEvent = nullptr;
#pragma endregion
};
