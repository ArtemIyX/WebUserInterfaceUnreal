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

/** @brief Type declaration. */
class FCefWebSocketServerInstance : public TSharedFromThis<FCefWebSocketServerInstance>
{
public:
#pragma region Lifecycle
	/** @brief FCefWebSocketServerInstance API. */
	FCefWebSocketServerInstance(FName InNameId, int32 InBoundPort, TWeakObjectPtr<UCefWebSocketServerBase> InOwnerServer);
	/** @brief FCefWebSocketServerInstance API. */
	~FCefWebSocketServerInstance();
#pragma endregion

public:
#pragma region PublicApi
	/** @brief Start API. */
	bool Start();
	/** @brief Stop API. */
	void Stop();
	/** @brief IsRunning API. */
	bool IsRunning() const;

	/** @brief SendToClientString API. */
	ECefWebSocketSendResult SendToClientString(int64 InClientId, const FString& InMessage);
	/** @brief SendToClientBytes API. */
	ECefWebSocketSendResult SendToClientBytes(int64 InClientId, const TArray<uint8>& InBytes);
	/** @brief BroadcastString API. */
	ECefWebSocketSendResult BroadcastString(const FString& InMessage);
	/** @brief BroadcastBytes API. */
	ECefWebSocketSendResult BroadcastBytes(const TArray<uint8>& InBytes);
	/** @brief BroadcastStringExcept API. */
	ECefWebSocketSendResult BroadcastStringExcept(int64 InExcludedClientId, const FString& InMessage);
	/** @brief BroadcastBytesExcept API. */
	ECefWebSocketSendResult BroadcastBytesExcept(int64 InExcludedClientId, const TArray<uint8>& InBytes);
	/** @brief DisconnectClient API. */
	ECefWebSocketSendResult DisconnectClient(int64 InClientId, ECefWebSocketCloseReason InReason);

	/** @brief SetPipelineConfig API. */
	void SetPipelineConfig(const FCefWebSocketPipelineConfig& InConfig);
	/** @brief SetPayloadFormat API. */
	void SetPayloadFormat(ECefWebSocketPayloadFormat InPayloadFormat);
	/** @brief SetPacketCodec API. */
	void SetPacketCodec(const TSharedPtr<ICefWebSocketPacketCodec>& InCodec);

	/** @brief GetClients API. */
	TArray<FCefWebSocketClientInfo> GetClients() const;
	/** @brief GetStats API. */
	FCefWebSocketServerStats GetStats() const;
	/** @brief FindClientInfo API. */
	FCefWebSocketClientInfo FindClientInfo(int64 InClientId) const;
	FName GetNameId() const { return NameId; }
	int32 GetBoundPort() const { return BoundPort; }
#pragma endregion

public:
#pragma region ThreadApi
	/** @brief TickBackendOnReadThread API. */
	bool TickBackendOnReadThread();
	/** @brief PumpInboundOnHandleThread API. */
	bool PumpInboundOnHandleThread();
	/** @brief PumpOutgoingOnSendThread API. */
	bool PumpOutgoingOnSendThread();
	/** @brief PumpOutgoingOnWriteThread API. */
	bool PumpOutgoingOnWriteThread();
#pragma endregion

private:
#pragma region InternalTypes
	/** @brief Type declaration. */
	struct FCefOutboundMessage
	{
		/** @brief Payload state. */
		TArray<uint8> Payload;
		/** @brief bBinary state. */
		bool bBinary = true;
	};

	/** @brief Type declaration. */
	struct FCefClientState
	{
		FCefClientState()
			: Outbox(MakeUnique<TQueue<FCefOutboundMessage, EQueueMode::Mpsc>>())
		{
		}

		/** @brief Info state. */
		FCefWebSocketClientInfo Info;
		/** @brief Socket state. */
		ICefNetWebSocket* Socket = nullptr;
		/** @brief Outbox state. */
		TUniquePtr<TQueue<FCefOutboundMessage, EQueueMode::Mpsc>> Outbox;
		/** @brief QueueBytes state. */
		int64 QueueBytes = 0;
		/** @brief QueueMessages state. */
		int32 QueueMessages = 0;
		/** @brief LastActivityTimeSec state. */
		double LastActivityTimeSec = 0.0;
		/** @brief LastHeartbeatSentTimeSec state. */
		double LastHeartbeatSentTimeSec = 0.0;
		/** @brief RxWindowStartSec state. */
		double RxWindowStartSec = 0.0;
		/** @brief TxWindowStartSec state. */
		double TxWindowStartSec = 0.0;
		/** @brief RxBytesInWindow state. */
		int64 RxBytesInWindow = 0;
		/** @brief TxBytesInWindow state. */
		int64 TxBytesInWindow = 0;
	};
#pragma endregion

private:
#pragma region InternalMethods
	/** @brief HandleClientConnected API. */
	void HandleClientConnected(ICefNetWebSocket* InSocket);
	/** @brief HandleClientDisconnected API. */
	void HandleClientDisconnected(ICefNetWebSocket* InSocket);
	/** @brief HandleClientPacket API. */
	void HandleClientPacket(ICefNetWebSocket* InSocket, const uint8* InData, int32 InCount, bool bInBinary);

	/** @brief QueueSendRequest API. */
	ECefWebSocketSendResult QueueSendRequest(FCefWebSocketSendRequest&& InRequest);
	/** @brief WakeHandleThread API. */
	void WakeHandleThread();
	/** @brief WakeSendThread API. */
	void WakeSendThread();
	/** @brief WakeWriteThread API. */
	void WakeWriteThread();

	/** @brief ResolveCodec API. */
	TSharedPtr<ICefWebSocketPacketCodec> ResolveCodec(ECefWebSocketPayloadFormat InPayloadFormat) const;
	/** @brief EnqueueWritePacket API. */
	bool EnqueueWritePacket(int64 InClientId, const uint8* InData, int32 InCount, bool bInBinary);
	/** @brief RecordQueueDepthSample_NoLock API. */
	void RecordQueueDepthSample_NoLock();
	/** @brief UpdateRateStats_NoLock API. */
	void UpdateRateStats_NoLock();
	/** @brief SweepConnectionHealthOnReadThread API. */
	void SweepConnectionHealthOnReadThread();
	/** @brief AreQueuesDrained API. */
	bool AreQueuesDrained() const;
#pragma endregion

private:
#pragma region InternalState
	/** @brief NameId state. */
	FName NameId = NAME_None;
	/** @brief BoundPort state. */
	int32 BoundPort = 0;
	/** @brief OwnerServer state. */
	TWeakObjectPtr<UCefWebSocketServerBase> OwnerServer;
	/** @brief bRunning state. */
	TAtomic<bool> bRunning = false;

	/** @brief ClientLock state. */
	mutable FRWLock ClientLock;
	/** @brief Clients state. */
	TMap<int64, FCefClientState> Clients;
	/** @brief ClientIdsBySocket state. */
	TMap<ICefNetWebSocket*, int64> ClientIdsBySocket;

	/** @brief CodecLock state. */
	mutable FCriticalSection CodecLock;
	/** @brief PayloadFormat state. */
	ECefWebSocketPayloadFormat PayloadFormat = ECefWebSocketPayloadFormat::Binary;
	/** @brief CustomCodec state. */
	TSharedPtr<ICefWebSocketPacketCodec> CustomCodec;
	/** @brief BinaryCodec state. */
	TSharedPtr<ICefWebSocketPacketCodec> BinaryCodec;
	/** @brief Utf8Codec state. */
	TSharedPtr<ICefWebSocketPacketCodec> Utf8Codec;
	/** @brief JsonCodec state. */
	TSharedPtr<ICefWebSocketPacketCodec> JsonCodec;
	/** @brief XmlCodec state. */
	TSharedPtr<ICefWebSocketPacketCodec> XmlCodec;

	/** @brief InboundQueue state. */
	TQueue<FCefWebSocketInboundPacket, EQueueMode::Mpsc> InboundQueue;
	/** @brief SendQueue state. */
	TQueue<FCefWebSocketSendRequest, EQueueMode::Mpsc> SendQueue;
	/** @brief InboundQueueDepth state. */
	TAtomic<int64> InboundQueueDepth = 0;
	/** @brief SendQueueDepth state. */
	TAtomic<int64> SendQueueDepth = 0;
	/** @brief PipelineConfig state. */
	FCefWebSocketPipelineConfig PipelineConfig;

	/** @brief Backend state. */
	TUniquePtr<ICefWebSocketServerBackend> Backend;
	/** @brief NextClientId state. */
	int64 NextClientId = 1;
	/** @brief Stats state. */
	FCefWebSocketServerStats Stats;
	/** @brief LastRateSampleTimeSec state. */
	double LastRateSampleTimeSec = 0.0;
	/** @brief LastRateRxBytes state. */
	int64 LastRateRxBytes = 0;
	/** @brief LastRateTxBytes state. */
	int64 LastRateTxBytes = 0;
	/** @brief QueueDepthAccum state. */
	int64 QueueDepthAccum = 0;
	/** @brief QueueDepthSamples state. */
	int64 QueueDepthSamples = 0;

	/** @brief ReadRunnable state. */
	TUniquePtr<FCefWebSocketReadRunnable> ReadRunnable;
	/** @brief HandleRunnable state. */
	TUniquePtr<FCefWebSocketHandleRunnable> HandleRunnable;
	/** @brief SendRunnable state. */
	TUniquePtr<FCefWebSocketSendRunnable> SendRunnable;
	/** @brief WriteRunnable state. */
	TUniquePtr<FCefWebSocketWriteRunnable> WriteRunnable;
	/** @brief ReadThread state. */
	FRunnableThread* ReadThread = nullptr;
	/** @brief HandleThread state. */
	FRunnableThread* HandleThread = nullptr;
	/** @brief SendThread state. */
	FRunnableThread* SendThread = nullptr;
	/** @brief WriteThread state. */
	FRunnableThread* WriteThread = nullptr;
	/** @brief HandleWakeEvent state. */
	FEvent* HandleWakeEvent = nullptr;
	/** @brief SendWakeEvent state. */
	FEvent* SendWakeEvent = nullptr;
	/** @brief WriteWakeEvent state. */
	FEvent* WriteWakeEvent = nullptr;
	/** @brief bReadActivity state. */
	TAtomic<bool> bReadActivity = false;
#pragma endregion
};

