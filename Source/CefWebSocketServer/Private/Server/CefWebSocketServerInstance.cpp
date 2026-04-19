#include "Server/CefWebSocketServerInstance.h"

#include "Config/CefWebSocketCVars.h"
#include "Logging/CefWebSocketLog.h"
#include "Networking/CefWebSocketServerBackend.h"
#include "Networking/ICefNetWebSocket.h"
#include "Pipeline/CefWebSocketDefaultCodecs.h"
#include "Pipeline/ICefWebSocketPacketCodec.h"
#include "Server/CefWebSocketServerBase.h"
#include "Stats/CefWebSocketStats.h"
#include "Threads/CefWebSocketHandleRunnable.h"
#include "Threads/CefWebSocketReadRunnable.h"
#include "Threads/CefWebSocketSendRunnable.h"
#include "Threads/CefWebSocketWriteRunnable.h"

#include "HAL/Event.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "HAL/RunnableThread.h"

FCefWebSocketServerInstance::FCefWebSocketServerInstance(FName InNameId, int32 InBoundPort,
                                                         TWeakObjectPtr<UCefWebSocketServerBase> InOwnerServer)
	: NameId(InNameId)
	, BoundPort(InBoundPort)
	, OwnerServer(InOwnerServer)
{
}

FCefWebSocketServerInstance::~FCefWebSocketServerInstance()
{
	Stop();
}

bool FCefWebSocketServerInstance::Start()
{
	if (bRunning.Load())
	{
		return true;
	}

	Backend = MakeUnique<FCefWebSocketServerBackend>();
	FCefNetWebSocketClientConnectedCallback onConnected;
	onConnected.BindRaw(this, &FCefWebSocketServerInstance::HandleClientConnected);
	FCefNetWebSocketClientDisconnectedCallback onDisconnected;
	onDisconnected.BindRaw(this, &FCefWebSocketServerInstance::HandleClientDisconnected);

	if (!Backend->Init(static_cast<uint32>(BoundPort), onConnected, onDisconnected))
	{
		if (OwnerServer.IsValid())
		{
			OwnerServer->NotifyServerError(
				ECefWebSocketErrorCode::ServerInitFailed,
				FString::Printf(TEXT("Failed to start server %s on port %d"), *NameId.ToString(), BoundPort));
		}
		Backend.Reset();
		return false;
	}

	{
		FScopeLock codecLock(&CodecLock);
		BinaryCodec = MakeShared<FCefWebSocketBinaryPassthroughCodec>();
		Utf8Codec = MakeShared<FCefWebSocketUtf8StringCodec>(ECefWebSocketPayloadFormat::Utf8String);
		JsonCodec = MakeShared<FCefWebSocketUtf8StringCodec>(ECefWebSocketPayloadFormat::JsonString);
		XmlCodec = MakeShared<FCefWebSocketUtf8StringCodec>(ECefWebSocketPayloadFormat::XmlString);
	}

	bRunning.Store(true);
	LastRateSampleTimeSec = FPlatformTime::Seconds();
	LastRateRxBytes = 0;
	LastRateTxBytes = 0;
	QueueDepthAccum = 0;
	QueueDepthSamples = 0;
	InboundQueueDepth.Store(0);
	SendQueueDepth.Store(0);

	HandleWakeEvent = FPlatformProcess::GetSynchEventFromPool(false);
	SendWakeEvent = FPlatformProcess::GetSynchEventFromPool(false);
	WriteWakeEvent = FPlatformProcess::GetSynchEventFromPool(false);

	ReadRunnable = MakeUnique<FCefWebSocketReadRunnable>(this);
	HandleRunnable = MakeUnique<FCefWebSocketHandleRunnable>(this, HandleWakeEvent);
	SendRunnable = MakeUnique<FCefWebSocketSendRunnable>(this, SendWakeEvent);
	WriteRunnable = MakeUnique<FCefWebSocketWriteRunnable>(this, WriteWakeEvent);

	ReadThread = FRunnableThread::Create(ReadRunnable.Get(), *FString::Printf(TEXT("CefWsRead_%s"), *NameId.ToString()));
	HandleThread = FRunnableThread::Create(HandleRunnable.Get(), *FString::Printf(TEXT("CefWsHandle_%s"), *NameId.ToString()));
	SendThread = FRunnableThread::Create(SendRunnable.Get(), *FString::Printf(TEXT("CefWsSend_%s"), *NameId.ToString()));
	WriteThread = FRunnableThread::Create(WriteRunnable.Get(), *FString::Printf(TEXT("CefWsWrite_%s"), *NameId.ToString()));
	if (!ReadThread || !HandleThread || !SendThread || !WriteThread)
	{
		Stop();
		if (OwnerServer.IsValid())
		{
			OwnerServer->NotifyServerError(ECefWebSocketErrorCode::ServerInitFailed,
			                               TEXT("Failed to create websocket server threads"));
		}
		return false;
	}

	BoundPort = static_cast<int32>(Backend->GetServerPort());
	UE_LOG(LogCefWebSocketServer, Log, TEXT("Server '%s' started on port %d"), *NameId.ToString(), BoundPort);
	return true;
}

void FCefWebSocketServerInstance::Stop()
{
	if (!bRunning.Load() && !Backend.IsValid())
	{
		return;
	}

	const int32 shutdownDrainMs = FMath::Max(0, CefWebSocketCVars::GetShutdownDrainMs());
	if (bRunning.Load() && shutdownDrainMs > 0)
	{
		const double deadlineSec = FPlatformTime::Seconds() + (static_cast<double>(shutdownDrainMs) / 1000.0);
		while (bRunning.Load() && !AreQueuesDrained() && FPlatformTime::Seconds() < deadlineSec)
		{
			WakeHandleThread();
			WakeSendThread();
			WakeWriteThread();
			FPlatformProcess::SleepNoStats(0.001f);
		}
	}

	bRunning.Store(false);

	if (ReadRunnable)
	{
		ReadRunnable->Stop();
	}
	if (HandleRunnable)
	{
		HandleRunnable->Stop();
	}
	if (SendRunnable)
	{
		SendRunnable->Stop();
	}
	if (WriteRunnable)
	{
		WriteRunnable->Stop();
	}
	WakeHandleThread();
	WakeSendThread();
	WakeWriteThread();

	if (ReadThread)
	{
		ReadThread->WaitForCompletion();
		delete ReadThread;
		ReadThread = nullptr;
	}
	if (HandleThread)
	{
		HandleThread->WaitForCompletion();
		delete HandleThread;
		HandleThread = nullptr;
	}
	if (SendThread)
	{
		SendThread->WaitForCompletion();
		delete SendThread;
		SendThread = nullptr;
	}
	if (WriteThread)
	{
		WriteThread->WaitForCompletion();
		delete WriteThread;
		WriteThread = nullptr;
	}

	ReadRunnable.Reset();
	HandleRunnable.Reset();
	SendRunnable.Reset();
	WriteRunnable.Reset();

	FCefWebSocketInboundPacket inboundPacket;
	while (InboundQueue.Dequeue(inboundPacket))
	{
	}
	FCefWebSocketSendRequest sendRequest;
	while (SendQueue.Dequeue(sendRequest))
	{
	}
	InboundQueueDepth.Store(0);
	SendQueueDepth.Store(0);

	TArray<ICefNetWebSocket*> socketsToClose;
	{
		FScopeLock lock(&ClientLock);
		for (TPair<int64, FCefClientState>& pair : Clients)
		{
			if (pair.Value.Socket)
			{
				socketsToClose.Add(pair.Value.Socket);
			}
		}
		Clients.Empty();
		ClientIdsBySocket.Empty();
		Stats = FCefWebSocketServerStats();
		LastRateSampleTimeSec = 0.0;
		LastRateRxBytes = 0;
		LastRateTxBytes = 0;
		QueueDepthAccum = 0;
		QueueDepthSamples = 0;
	}

	for (ICefNetWebSocket* socket : socketsToClose)
	{
		socket->Close();
	}

	Backend.Reset();
	if (HandleWakeEvent)
	{
		FPlatformProcess::ReturnSynchEventToPool(HandleWakeEvent);
		HandleWakeEvent = nullptr;
	}
	if (SendWakeEvent)
	{
		FPlatformProcess::ReturnSynchEventToPool(SendWakeEvent);
		SendWakeEvent = nullptr;
	}
	if (WriteWakeEvent)
	{
		FPlatformProcess::ReturnSynchEventToPool(WriteWakeEvent);
		WriteWakeEvent = nullptr;
	}
}

bool FCefWebSocketServerInstance::IsRunning() const
{
	return bRunning.Load();
}

void FCefWebSocketServerInstance::SetPipelineConfig(const FCefWebSocketPipelineConfig& InConfig)
{
	FScopeLock lock(&CodecLock);
	PipelineConfig = InConfig;
	PayloadFormat = InConfig.InPayloadFormat;
}

void FCefWebSocketServerInstance::SetPayloadFormat(ECefWebSocketPayloadFormat InPayloadFormat)
{
	FScopeLock lock(&CodecLock);
	PayloadFormat = InPayloadFormat;
}

void FCefWebSocketServerInstance::SetPacketCodec(const TSharedPtr<ICefWebSocketPacketCodec>& InCodec)
{
	FScopeLock lock(&CodecLock);
	CustomCodec = InCodec;
}

bool FCefWebSocketServerInstance::TickBackendOnReadThread()
{
	if (!bRunning.Load() || !Backend)
	{
		return false;
	}
	Backend->Tick();
	SweepConnectionHealthOnReadThread();
	return bReadActivity.Exchange(false);
}

bool FCefWebSocketServerInstance::PumpInboundOnHandleThread()
{
	if (!bRunning.Load())
	{
		return false;
	}

	bool bDidWork = false;
	FCefWebSocketInboundPacket packet;
	while (InboundQueue.Dequeue(packet))
	{
		bDidWork = true;
		InboundQueueDepth.Store(FMath::Max<int64>(0, InboundQueueDepth.Load() - 1));

		FCefWebSocketInboundPacket decodedPacket;
		FString error;
		TSharedPtr<ICefWebSocketPacketCodec> codec = ResolveCodec(PayloadFormat);
		if (!codec.IsValid() || !codec->DecodeInbound(packet, decodedPacket, error))
		{
			if (OwnerServer.IsValid())
			{
				OwnerServer->NotifyClientError(packet.ClientId, ECefWebSocketErrorCode::InvalidPayload,
				                               error.IsEmpty() ? TEXT("Inbound decode failed") : error);
			}
			continue;
		}

		if (OwnerServer.IsValid())
		{
			OwnerServer->NotifyClientMessage(decodedPacket.ClientId, decodedPacket.Payload, decodedPacket.bBinary);
		}
	}

	Stats.InInboundQueueDepth = InboundQueueDepth.Load();
	Stats.InHandleQueueDepth = Stats.InInboundQueueDepth;
	return bDidWork;
}

bool FCefWebSocketServerInstance::PumpOutgoingOnSendThread()
{
	if (!bRunning.Load())
	{
		return false;
	}

	bool bDidWork = false;
	FCefWebSocketSendRequest sendRequest;
	while (SendQueue.Dequeue(sendRequest))
	{
		bDidWork = true;
		SendQueueDepth.Store(FMath::Max<int64>(0, SendQueueDepth.Load() - 1));

		TSharedPtr<ICefWebSocketPacketCodec> codec = ResolveCodec(sendRequest.PayloadFormat);
		if (!codec.IsValid())
		{
			continue;
		}

		TArray<FCefWebSocketWritePacket> writePackets;
		FString error;
		if (!codec->EncodeSendRequest(sendRequest, writePackets, error))
		{
			if (OwnerServer.IsValid())
			{
				OwnerServer->NotifyServerError(ECefWebSocketErrorCode::JsonSerializeFailed,
				                               error.IsEmpty() ? TEXT("Send encode failed") : error);
			}
			continue;
		}

		for (const FCefWebSocketWritePacket& writePacket : writePackets)
		{
			if (!EnqueueWritePacket(writePacket.ClientId, writePacket.Payload.GetData(), writePacket.Payload.Num(),
			                       writePacket.bBinary))
			{
				if (OwnerServer.IsValid())
				{
					OwnerServer->NotifyClientError(writePacket.ClientId, ECefWebSocketErrorCode::QueueOverflow,
					                               TEXT("Write queue overflow"));
				}
			}
		}
	}

	Stats.InSendQueueDepth = SendQueueDepth.Load();
	return bDidWork;
}

bool FCefWebSocketServerInstance::PumpOutgoingOnWriteThread()
{
	if (!bRunning.Load())
	{
		return false;
	}

	struct FSendWork
	{
		int64 ClientId = 0;
		ICefNetWebSocket* Socket = nullptr;
		FCefOutboundMessage Message;
	};

	TArray<FSendWork> workItems;
	const int32 maxBatchMessagesPerClient = FMath::Max(1, CefWebSocketCVars::GetWriteBatchMaxMessages());
	const int32 maxBatchBytesPerClient = FMath::Max(1, CefWebSocketCVars::GetWriteBatchMaxBytes());
	{
		FScopeLock lock(&ClientLock);
		for (TPair<int64, FCefClientState>& pair : Clients)
		{
			if (!pair.Value.Outbox)
			{
				continue;
			}

			int32 drainedMessages = 0;
			int32 drainedBytes = 0;
			FCefOutboundMessage message;
			while (drainedMessages < maxBatchMessagesPerClient &&
				drainedBytes < maxBatchBytesPerClient &&
				pair.Value.Outbox->Dequeue(message))
			{
				pair.Value.QueueMessages = FMath::Max(0, pair.Value.QueueMessages - 1);
				pair.Value.QueueBytes = FMath::Max<int64>(0, pair.Value.QueueBytes - message.Payload.Num());
				Stats.QueueDepth = FMath::Max<int64>(0, Stats.QueueDepth - 1);
				RecordQueueDepthSample_NoLock();
				FSendWork& item = workItems.AddDefaulted_GetRef();
				item.ClientId = pair.Key;
				item.Socket = pair.Value.Socket;
				item.Message = MoveTemp(message);
				drainedMessages += 1;
				drainedBytes += item.Message.Payload.Num();
			}
		}
	}

	if (workItems.Num() == 0)
	{
		Stats.InWriteQueueDepth = Stats.QueueDepth;
		return false;
	}

	int64 sentBytes = 0;
	for (const FSendWork& item : workItems)
	{
		if (!item.Socket || item.Message.Payload.Num() == 0)
		{
			continue;
		}
		if (!item.Socket->Send(item.Message.Payload.GetData(), static_cast<uint32>(item.Message.Payload.Num()), false))
		{
			if (OwnerServer.IsValid())
			{
				OwnerServer->NotifyClientError(item.ClientId, ECefWebSocketErrorCode::SocketSendFailed,
				                               TEXT("Socket send failed"));
			}
			continue;
		}
		sentBytes += item.Message.Payload.Num();
	}

	{
		FScopeLock lock(&ClientLock);
		Stats.TxBytes += sentBytes;
		UpdateRateStats_NoLock();
		Stats.InWriteQueueDepth = Stats.QueueDepth;
	}

	SET_DWORD_STAT(STAT_CefWs_ActiveClients, Stats.ActiveClients);
	SET_DWORD_STAT(STAT_CefWs_QueueDepth, static_cast<uint32>(FMath::Min<int64>(Stats.QueueDepth, MAX_uint32)));
	SET_DWORD_STAT(STAT_CefWs_TxBytes, static_cast<uint32>(FMath::Min<int64>(Stats.TxBytes, MAX_uint32)));
	SET_DWORD_STAT(STAT_CefWs_DroppedMessages,
	               static_cast<uint32>(FMath::Min<int64>(Stats.DroppedMessages, MAX_uint32)));
	return true;
}

void FCefWebSocketServerInstance::HandleClientConnected(ICefNetWebSocket* InSocket)
{
	if (!InSocket)
	{
		return;
	}

	FCefWebSocketClientInfo info;
	info.ClientId = NextClientId++;
	info.RemoteAddress = InSocket->RemoteEndPoint(true);
	info.ConnectedAt = FDateTime::UtcNow();
	const double nowSec = FPlatformTime::Seconds();

	FCefNetWebSocketPacketReceivedCallback onReceive;
	onReceive.BindLambda([this, InSocket](void* data, int32 count, bool bBinary)
	{
		HandleClientPacket(InSocket, static_cast<const uint8*>(data), count, bBinary);
	});
	InSocket->SetReceiveCallBack(onReceive);

	{
		FScopeLock lock(&ClientLock);
		FCefClientState& state = Clients.FindOrAdd(info.ClientId);
		state.Info = info;
		state.Socket = InSocket;
		state.LastActivityTimeSec = nowSec;
		state.LastHeartbeatSentTimeSec = 0.0;
		ClientIdsBySocket.Add(InSocket, info.ClientId);
		Stats.ActiveClients = Clients.Num();
		RecordQueueDepthSample_NoLock();
	}
	bReadActivity.Store(true);

	if (OwnerServer.IsValid())
	{
		OwnerServer->NotifyClientConnected(info);
	}
}

void FCefWebSocketServerInstance::HandleClientDisconnected(ICefNetWebSocket* InSocket)
{
	if (!InSocket)
	{
		return;
	}

	int64 clientId = 0;
	{
		FScopeLock lock(&ClientLock);
		if (int64* found = ClientIdsBySocket.Find(InSocket))
		{
			clientId = *found;
			if (const FCefClientState* state = Clients.Find(clientId))
			{
				Stats.QueueDepth = FMath::Max<int64>(0, Stats.QueueDepth - state->QueueMessages);
			}
			ClientIdsBySocket.Remove(InSocket);
			Clients.Remove(clientId);
			Stats.ActiveClients = Clients.Num();
			RecordQueueDepthSample_NoLock();
		}
	}
	bReadActivity.Store(true);

	if (clientId != 0 && OwnerServer.IsValid())
	{
		OwnerServer->NotifyClientDisconnected(clientId, ECefWebSocketCloseReason::ClientClosed);
	}
}

void FCefWebSocketServerInstance::HandleClientPacket(ICefNetWebSocket* InSocket, const uint8* InData, int32 InCount,
                                                     bool bInBinary)
{
	if (!InSocket || !InData || InCount <= 0)
	{
		return;
	}
	if (InCount > CefWebSocketCVars::GetMaxMessageBytes())
	{
		int64 clientId = 0;
		{
			FScopeLock lock(&ClientLock);
			if (int64* found = ClientIdsBySocket.Find(InSocket))
			{
				clientId = *found;
			}
		}
		if (clientId != 0 && OwnerServer.IsValid())
		{
			OwnerServer->NotifyClientError(clientId, ECefWebSocketErrorCode::InvalidPayload,
			                               TEXT("Incoming message exceeds cefws.max_message_bytes"));
		}
		return;
	}
	if (!bInBinary && InCount > CefWebSocketCVars::GetMaxTextMessageBytes())
	{
		int64 clientId = 0;
		{
			FScopeLock lock(&ClientLock);
			if (int64* found = ClientIdsBySocket.Find(InSocket))
			{
				clientId = *found;
			}
		}
		if (clientId != 0 && OwnerServer.IsValid())
		{
			OwnerServer->NotifyClientError(clientId, ECefWebSocketErrorCode::InvalidPayload,
			                               TEXT("Incoming text message exceeds cefws.max_text_message_bytes"));
		}
		return;
	}

	int64 clientId = 0;
	{
		FScopeLock lock(&ClientLock);
		if (int64* found = ClientIdsBySocket.Find(InSocket))
		{
			clientId = *found;
			Stats.RxBytes += InCount;
			if (FCefClientState* state = Clients.Find(clientId))
			{
				state->LastActivityTimeSec = FPlatformTime::Seconds();
			}
			UpdateRateStats_NoLock();
		}
	}
	bReadActivity.Store(true);

	if (clientId == 0)
	{
		return;
	}

	if (InboundQueueDepth.Load() >= FMath::Max(1, PipelineConfig.InInboundQueueMax))
	{
		Stats.DroppedMessages += 1;
		if (OwnerServer.IsValid())
		{
			OwnerServer->NotifyClientError(clientId, ECefWebSocketErrorCode::QueueOverflow,
			                               TEXT("Inbound queue overflow"));
		}
		return;
	}

	FCefWebSocketInboundPacket inboundPacket;
	inboundPacket.ClientId = clientId;
	inboundPacket.Payload.Append(InData, InCount);
	inboundPacket.bBinary = bInBinary;
	InboundQueue.Enqueue(MoveTemp(inboundPacket));
	InboundQueueDepth.Store(InboundQueueDepth.Load() + 1);
	Stats.InInboundQueueDepth = InboundQueueDepth.Load();
	WakeHandleThread();

	SET_DWORD_STAT(STAT_CefWs_RxBytes, static_cast<uint32>(FMath::Min<int64>(Stats.RxBytes, MAX_uint32)));
}

ECefWebSocketSendResult FCefWebSocketServerInstance::QueueSendRequest(FCefWebSocketSendRequest&& InRequest)
{
	if (!IsRunning())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}

	int32 payloadSizeBytes = InRequest.BytesPayload.Num();
	if (payloadSizeBytes <= 0 && !InRequest.TextPayload.IsEmpty())
	{
		FTCHARToUTF8 utf8Payload(*InRequest.TextPayload);
		payloadSizeBytes = utf8Payload.Length();
	}
	if (payloadSizeBytes > CefWebSocketCVars::GetMaxOutboundMessageBytes())
	{
		return ECefWebSocketSendResult::TooLarge;
	}

	if (SendQueueDepth.Load() >= FMath::Max(1, PipelineConfig.InSendQueueMax))
	{
		Stats.DroppedMessages += 1;
		return ECefWebSocketSendResult::QueueFull;
	}

	SendQueue.Enqueue(MoveTemp(InRequest));
	SendQueueDepth.Store(SendQueueDepth.Load() + 1);
	Stats.InSendQueueDepth = SendQueueDepth.Load();
	WakeSendThread();
	return ECefWebSocketSendResult::Ok;
}

bool FCefWebSocketServerInstance::EnqueueWritePacket(int64 InClientId, const uint8* InData, int32 InCount, bool bInBinary)
{
	if (!IsRunning() || !InData || InCount <= 0)
	{
		return false;
	}

	FScopeLock lock(&ClientLock);
	FCefClientState* found = Clients.Find(InClientId);
	if (!found || !found->Socket)
	{
		return false;
	}

	const int32 maxMessages = FMath::Max(1, CefWebSocketCVars::GetMaxQueueMessagesPerClient());
	const int32 maxBytes = FMath::Max(1, CefWebSocketCVars::GetMaxQueueBytesPerClient());
	const bool bRejectNewOnOverflow = CefWebSocketCVars::GetQueueDropPolicy() == 1;
	const int32 configuredMaxMessages = FMath::Max(1, PipelineConfig.InWriteQueueMaxPerClient);
	const int32 finalMaxMessages = FMath::Min(maxMessages, configuredMaxMessages);

	while (!bRejectNewOnOverflow &&
		(found->QueueMessages >= finalMaxMessages || found->QueueBytes + InCount > maxBytes) &&
		found->QueueMessages > 0)
	{
		FCefOutboundMessage dropped;
		if (!found->Outbox || !found->Outbox->Dequeue(dropped))
		{
			break;
		}
		found->QueueMessages = FMath::Max(0, found->QueueMessages - 1);
		found->QueueBytes = FMath::Max<int64>(0, found->QueueBytes - dropped.Payload.Num());
		Stats.DroppedMessages += 1;
		Stats.QueueDepth = FMath::Max<int64>(0, Stats.QueueDepth - 1);
		RecordQueueDepthSample_NoLock();
	}

	if (found->QueueMessages >= finalMaxMessages || found->QueueBytes + InCount > maxBytes)
	{
		Stats.DroppedMessages += 1;
		return false;
	}

	FCefOutboundMessage message;
	message.Payload.Append(InData, InCount);
	message.bBinary = bInBinary;
	if (!found->Outbox)
	{
		found->Outbox = MakeUnique<TQueue<FCefOutboundMessage, EQueueMode::Mpsc>>();
	}
	found->Outbox->Enqueue(MoveTemp(message));
	found->QueueMessages += 1;
	found->QueueBytes += InCount;
	Stats.QueueDepth += 1;
	Stats.InWriteQueueDepth = Stats.QueueDepth;
	RecordQueueDepthSample_NoLock();
	WakeWriteThread();
	return true;
}

void FCefWebSocketServerInstance::WakeHandleThread()
{
	if (HandleWakeEvent)
	{
		HandleWakeEvent->Trigger();
	}
}

void FCefWebSocketServerInstance::WakeSendThread()
{
	if (SendWakeEvent)
	{
		SendWakeEvent->Trigger();
	}
}

void FCefWebSocketServerInstance::WakeWriteThread()
{
	if (WriteWakeEvent)
	{
		WriteWakeEvent->Trigger();
	}
}

TSharedPtr<ICefWebSocketPacketCodec> FCefWebSocketServerInstance::ResolveCodec(ECefWebSocketPayloadFormat InPayloadFormat) const
{
	FScopeLock lock(&CodecLock);
	if (CustomCodec.IsValid())
	{
		return CustomCodec;
	}

	switch (InPayloadFormat)
	{
	case ECefWebSocketPayloadFormat::Binary:
		return BinaryCodec;
	case ECefWebSocketPayloadFormat::Utf8String:
		return Utf8Codec;
	case ECefWebSocketPayloadFormat::JsonString:
		return JsonCodec;
	case ECefWebSocketPayloadFormat::XmlString:
		return XmlCodec;
	case ECefWebSocketPayloadFormat::Custom:
	default:
		return BinaryCodec;
	}
}

ECefWebSocketSendResult FCefWebSocketServerInstance::SendToClientString(int64 InClientId, const FString& InMessage)
{
	if (!IsRunning())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}

	FCefWebSocketSendRequest request;
	request.TargetClientIds.Add(InClientId);
	request.PayloadFormat = PayloadFormat;
	request.TextPayload = InMessage;
	return QueueSendRequest(MoveTemp(request));
}

ECefWebSocketSendResult FCefWebSocketServerInstance::SendToClientBytes(int64 InClientId, const TArray<uint8>& InBytes)
{
	if (!IsRunning())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	if (InBytes.Num() == 0)
	{
		return ECefWebSocketSendResult::InternalError;
	}

	FCefWebSocketSendRequest request;
	request.TargetClientIds.Add(InClientId);
	request.PayloadFormat = ECefWebSocketPayloadFormat::Binary;
	request.BytesPayload = InBytes;
	return QueueSendRequest(MoveTemp(request));
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastString(const FString& InMessage)
{
	FCefWebSocketSendRequest request;
	request.bBroadcast = true;
	request.PayloadFormat = PayloadFormat;
	request.TextPayload = InMessage;
	{
		FScopeLock lock(&ClientLock);
		Clients.GetKeys(request.TargetClientIds);
	}
	return QueueSendRequest(MoveTemp(request));
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastBytes(const TArray<uint8>& InBytes)
{
	if (InBytes.Num() == 0)
	{
		return ECefWebSocketSendResult::InternalError;
	}

	FCefWebSocketSendRequest request;
	request.bBroadcast = true;
	request.PayloadFormat = ECefWebSocketPayloadFormat::Binary;
	request.BytesPayload = InBytes;
	{
		FScopeLock lock(&ClientLock);
		Clients.GetKeys(request.TargetClientIds);
	}
	return QueueSendRequest(MoveTemp(request));
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastStringExcept(int64 InExcludedClientId, const FString& InMessage)
{
	FCefWebSocketSendRequest request;
	request.bBroadcast = true;
	request.ExcludedClientId = InExcludedClientId;
	request.PayloadFormat = PayloadFormat;
	request.TextPayload = InMessage;
	{
		FScopeLock lock(&ClientLock);
		Clients.GetKeys(request.TargetClientIds);
	}
	return QueueSendRequest(MoveTemp(request));
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastBytesExcept(int64 InExcludedClientId, const TArray<uint8>& InBytes)
{
	if (InBytes.Num() == 0)
	{
		return ECefWebSocketSendResult::InternalError;
	}

	FCefWebSocketSendRequest request;
	request.bBroadcast = true;
	request.ExcludedClientId = InExcludedClientId;
	request.PayloadFormat = ECefWebSocketPayloadFormat::Binary;
	request.BytesPayload = InBytes;
	{
		FScopeLock lock(&ClientLock);
		Clients.GetKeys(request.TargetClientIds);
	}
	return QueueSendRequest(MoveTemp(request));
}

ECefWebSocketSendResult FCefWebSocketServerInstance::DisconnectClient(int64 InClientId,
                                                                      ECefWebSocketCloseReason InReason)
{
	(void)InReason;
	ICefNetWebSocket* socket = nullptr;
	{
		FScopeLock lock(&ClientLock);
		if (FCefClientState* found = Clients.Find(InClientId))
		{
			socket = found->Socket;
		}
	}
	if (!socket)
	{
		return ECefWebSocketSendResult::InvalidClient;
	}

	socket->Close();
	return ECefWebSocketSendResult::Ok;
}

TArray<FCefWebSocketClientInfo> FCefWebSocketServerInstance::GetClients() const
{
	FScopeLock lock(&ClientLock);
	TArray<FCefWebSocketClientInfo> outClients;
	outClients.Reserve(Clients.Num());
	for (const TPair<int64, FCefClientState>& pair : Clients)
	{
		outClients.Add(pair.Value.Info);
	}
	return outClients;
}

FCefWebSocketServerStats FCefWebSocketServerInstance::GetStats() const
{
	FScopeLock lock(&ClientLock);
	const_cast<FCefWebSocketServerInstance*>(this)->UpdateRateStats_NoLock();
	if (QueueDepthSamples > 0)
	{
		const_cast<FCefWebSocketServerInstance*>(this)->Stats.AvgQueueDepth =
			static_cast<float>(static_cast<double>(QueueDepthAccum) / static_cast<double>(QueueDepthSamples));
	}
	else
	{
		const_cast<FCefWebSocketServerInstance*>(this)->Stats.AvgQueueDepth = static_cast<float>(Stats.QueueDepth);
	}
	const_cast<FCefWebSocketServerInstance*>(this)->Stats.InInboundQueueDepth = InboundQueueDepth.Load();
	const_cast<FCefWebSocketServerInstance*>(this)->Stats.InHandleQueueDepth = InboundQueueDepth.Load();
	const_cast<FCefWebSocketServerInstance*>(this)->Stats.InSendQueueDepth = SendQueueDepth.Load();
	const_cast<FCefWebSocketServerInstance*>(this)->Stats.InWriteQueueDepth = Stats.QueueDepth;
	return Stats;
}

FCefWebSocketClientInfo FCefWebSocketServerInstance::FindClientInfo(int64 InClientId) const
{
	FScopeLock lock(&ClientLock);
	if (const FCefClientState* found = Clients.Find(InClientId))
	{
		return found->Info;
	}
	return FCefWebSocketClientInfo();
}

void FCefWebSocketServerInstance::RecordQueueDepthSample_NoLock()
{
	QueueDepthAccum += Stats.QueueDepth;
	QueueDepthSamples += 1;
	if (QueueDepthSamples > 0)
	{
		Stats.AvgQueueDepth = static_cast<float>(static_cast<double>(QueueDepthAccum) / static_cast<double>(QueueDepthSamples));
	}
}

void FCefWebSocketServerInstance::UpdateRateStats_NoLock()
{
	const double nowSec = FPlatformTime::Seconds();
	if (LastRateSampleTimeSec <= 0.0)
	{
		LastRateSampleTimeSec = nowSec;
		LastRateRxBytes = Stats.RxBytes;
		LastRateTxBytes = Stats.TxBytes;
		Stats.RxBytesPerSec = 0.0f;
		Stats.TxBytesPerSec = 0.0f;
		return;
	}

	const double deltaSec = nowSec - LastRateSampleTimeSec;
	if (deltaSec < 0.05)
	{
		return;
	}

	const int64 deltaRx = Stats.RxBytes - LastRateRxBytes;
	const int64 deltaTx = Stats.TxBytes - LastRateTxBytes;
	Stats.RxBytesPerSec = static_cast<float>(static_cast<double>(deltaRx) / deltaSec);
	Stats.TxBytesPerSec = static_cast<float>(static_cast<double>(deltaTx) / deltaSec);
	LastRateSampleTimeSec = nowSec;
	LastRateRxBytes = Stats.RxBytes;
	LastRateTxBytes = Stats.TxBytes;
}

void FCefWebSocketServerInstance::SweepConnectionHealthOnReadThread()
{
	const float heartbeatIntervalSec = CefWebSocketCVars::GetHeartbeatIntervalSec();
	const float idleTimeoutSec = CefWebSocketCVars::GetIdleTimeoutSec();
	if (heartbeatIntervalSec <= 0.0f && idleTimeoutSec <= 0.0f)
	{
		return;
	}

	const double nowSec = FPlatformTime::Seconds();
	TArray<ICefNetWebSocket*> socketsToHeartbeat;
	TArray<ICefNetWebSocket*> socketsToClose;

	{
		FScopeLock lock(&ClientLock);
		for (TPair<int64, FCefClientState>& pair : Clients)
		{
			FCefClientState& state = pair.Value;
			if (!state.Socket)
			{
				continue;
			}

			if (idleTimeoutSec > 0.0f && state.LastActivityTimeSec > 0.0 &&
				nowSec - state.LastActivityTimeSec >= static_cast<double>(idleTimeoutSec))
			{
				socketsToClose.Add(state.Socket);
				continue;
			}

			if (heartbeatIntervalSec > 0.0f &&
				(nowSec - state.LastHeartbeatSentTimeSec) >= static_cast<double>(heartbeatIntervalSec))
			{
				socketsToHeartbeat.Add(state.Socket);
				state.LastHeartbeatSentTimeSec = nowSec;
			}
		}
	}

	static const uint8 heartbeatPayload[] = {'_', '_', 'c', 'e', 'f', 'w', 's', '_', 'h', 'b', '_', '_'};
	for (ICefNetWebSocket* socket : socketsToHeartbeat)
	{
		if (!socket)
		{
			continue;
		}
		socket->Send(heartbeatPayload, static_cast<uint32>(UE_ARRAY_COUNT(heartbeatPayload)), false);
	}

	for (ICefNetWebSocket* socket : socketsToClose)
	{
		if (!socket)
		{
			continue;
		}
		socket->Close();
	}
}

bool FCefWebSocketServerInstance::AreQueuesDrained() const
{
	if (InboundQueueDepth.Load() > 0 || SendQueueDepth.Load() > 0)
	{
		return false;
	}

	FScopeLock lock(&ClientLock);
	if (Stats.QueueDepth > 0)
	{
		return false;
	}
	for (const TPair<int64, FCefClientState>& pair : Clients)
	{
		if (pair.Value.QueueMessages > 0)
		{
			return false;
		}
	}
	return true;
}


