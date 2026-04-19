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

	ECefWebSocketSendResult SendToClientString(int64 InClientId, const FString& InMessage);
	ECefWebSocketSendResult SendToClientBytes(int64 InClientId, const TArray<uint8>& InBytes);
	ECefWebSocketSendResult BroadcastString(const FString& InMessage);
	ECefWebSocketSendResult BroadcastBytes(const TArray<uint8>& InBytes);
	ECefWebSocketSendResult BroadcastStringExcept(int64 InExcludedClientId, const FString& InMessage);
	ECefWebSocketSendResult BroadcastBytesExcept(int64 InExcludedClientId, const TArray<uint8>& InBytes);
	ECefWebSocketSendResult DisconnectClient(int64 InClientId, ECefWebSocketCloseReason InReason);

	TArray<FCefWebSocketClientInfo> GetClients() const;
	FCefWebSocketServerStats GetStats() const;
	FCefWebSocketClientInfo FindClientInfo(int64 InClientId) const;
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
		FCefClientState()
			: Outbox(MakeUnique<TQueue<FCefOutboundMessage, EQueueMode::Mpsc>>())
		{
		}

		FCefWebSocketClientInfo Info;
		ICefNetWebSocket* Socket = nullptr;
		TUniquePtr<TQueue<FCefOutboundMessage, EQueueMode::Mpsc>> Outbox;
		int64 QueueBytes = 0;
		int32 QueueMessages = 0;
	};
#pragma endregion

private:
#pragma region InternalMethods
	void HandleClientConnected(ICefNetWebSocket* InSocket);
	void HandleClientDisconnected(ICefNetWebSocket* InSocket);
	void HandleClientPacket(ICefNetWebSocket* InSocket, const uint8* InData, int32 InCount, bool bInBinary);
	ECefWebSocketSendResult EnqueueToClient(int64 InClientId, const uint8* InData, int32 InCount, bool bInBinary);
	ECefWebSocketSendResult EnqueueToClients(const TArray<int64>& InClientIds, const uint8* InData, int32 InCount, bool bInBinary);
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
