/**
 * @file CefWebSocketServer\Private\Server\CefWebSocketServerInstance.h
 * @brief Declares CefWebSocketServerInstance for module CefWebSocketServer\Private\Server\CefWebSocketServerInstance.h.
 * @details Contains websocket server components used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Data/CefWebSocketStructs.h"
#include "Data/CefWebSocketEnums.h"
#include "Containers/Queue.h"
#include "Pipeline/CefWebSocketPipelineTypes.h"

class ICefNetWebSocket;
class ICefWebSocketServerBackend;
class ICefWebSocketPacketCodec;
class FCefWebSocketReadRunnable;
class FCefWebSocketHandleRunnable;
class FCefWebSocketSendRunnable;
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

	void SetPipelineConfig(const FCefWebSocketPipelineConfig& InConfig);
	void SetPayloadFormat(ECefWebSocketPayloadFormat InPayloadFormat);
	void SetPacketCodec(const TSharedPtr<ICefWebSocketPacketCodec>& InCodec);

	TArray<FCefWebSocketClientInfo> GetClients() const;
	FCefWebSocketServerStats GetStats() const;
	FCefWebSocketClientInfo FindClientInfo(int64 InClientId) const;
	FName GetNameId() const { return NameId; }
	int32 GetBoundPort() const { return BoundPort; }
#pragma endregion

public:
#pragma region ThreadApi
	bool TickBackendOnReadThread();
	bool PumpInboundOnHandleThread();
	bool PumpOutgoingOnSendThread();
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
		double LastActivityTimeSec = 0.0;
		double LastHeartbeatSentTimeSec = 0.0;
		double RxWindowStartSec = 0.0;
		double TxWindowStartSec = 0.0;
		int64 RxBytesInWindow = 0;
		int64 TxBytesInWindow = 0;
	};
#pragma endregion

private:
#pragma region InternalMethods
	void HandleClientConnected(ICefNetWebSocket* InSocket);
	void HandleClientDisconnected(ICefNetWebSocket* InSocket);
	void HandleClientPacket(ICefNetWebSocket* InSocket, const uint8* InData, int32 InCount, bool bInBinary);

	ECefWebSocketSendResult QueueSendRequest(FCefWebSocketSendRequest&& InRequest);
	void WakeHandleThread();
	void WakeSendThread();
	void WakeWriteThread();

	TSharedPtr<ICefWebSocketPacketCodec> ResolveCodec(ECefWebSocketPayloadFormat InPayloadFormat) const;
	bool EnqueueWritePacket(int64 InClientId, const uint8* InData, int32 InCount, bool bInBinary);
	void RecordQueueDepthSample_NoLock();
	void UpdateRateStats_NoLock();
	void SweepConnectionHealthOnReadThread();
	bool AreQueuesDrained() const;
#pragma endregion

private:
#pragma region InternalState
	FName NameId = NAME_None;
	int32 BoundPort = 0;
	TWeakObjectPtr<UCefWebSocketServerBase> OwnerServer;
	TAtomic<bool> bRunning = false;

	mutable FRWLock ClientLock;
	TMap<int64, FCefClientState> Clients;
	TMap<ICefNetWebSocket*, int64> ClientIdsBySocket;

	mutable FCriticalSection CodecLock;
	ECefWebSocketPayloadFormat PayloadFormat = ECefWebSocketPayloadFormat::Binary;
	TSharedPtr<ICefWebSocketPacketCodec> CustomCodec;
	TSharedPtr<ICefWebSocketPacketCodec> BinaryCodec;
	TSharedPtr<ICefWebSocketPacketCodec> Utf8Codec;
	TSharedPtr<ICefWebSocketPacketCodec> JsonCodec;
	TSharedPtr<ICefWebSocketPacketCodec> XmlCodec;

	TQueue<FCefWebSocketInboundPacket, EQueueMode::Mpsc> InboundQueue;
	TQueue<FCefWebSocketSendRequest, EQueueMode::Mpsc> SendQueue;
	TAtomic<int64> InboundQueueDepth = 0;
	TAtomic<int64> SendQueueDepth = 0;
	FCefWebSocketPipelineConfig PipelineConfig;

	TUniquePtr<ICefWebSocketServerBackend> Backend;
	int64 NextClientId = 1;
	FCefWebSocketServerStats Stats;
	double LastRateSampleTimeSec = 0.0;
	int64 LastRateRxBytes = 0;
	int64 LastRateTxBytes = 0;
	int64 QueueDepthAccum = 0;
	int64 QueueDepthSamples = 0;

	TUniquePtr<FCefWebSocketReadRunnable> ReadRunnable;
	TUniquePtr<FCefWebSocketHandleRunnable> HandleRunnable;
	TUniquePtr<FCefWebSocketSendRunnable> SendRunnable;
	TUniquePtr<FCefWebSocketWriteRunnable> WriteRunnable;
	FRunnableThread* ReadThread = nullptr;
	FRunnableThread* HandleThread = nullptr;
	FRunnableThread* SendThread = nullptr;
	FRunnableThread* WriteThread = nullptr;
	FEvent* HandleWakeEvent = nullptr;
	FEvent* SendWakeEvent = nullptr;
	FEvent* WriteWakeEvent = nullptr;
	TAtomic<bool> bReadActivity = false;
#pragma endregion
};

