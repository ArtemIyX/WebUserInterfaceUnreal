#include "Server/CefWebSocketServerInstance.h"

#include "Config/CefWebSocketCVars.h"
#include "Logging/CefWebSocketLog.h"
#include "Networking/CefWebSocketServerBackend.h"
#include "Networking/ICefNetWebSocket.h"
#include "Server/CefWebSocketServerBase.h"
#include "Stats/CefWebSocketStats.h"
#include "Threads/CefWebSocketReadRunnable.h"
#include "Threads/CefWebSocketWriteRunnable.h"

#include "HAL/PlatformProcess.h"
#include "HAL/RunnableThread.h"
#include "HAL/PlatformTime.h"

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
			OwnerServer->NotifyServerError(ECefWebSocketErrorCode::ServerInitFailed,
			                               FString::Printf(
				                               TEXT("Failed to start server %s on port %d"), *NameId.ToString(),
				                               BoundPort));
		}
		Backend.Reset();
		return false;
	}

	bRunning.Store(true);
	LastRateSampleTimeSec = FPlatformTime::Seconds();
	LastRateRxBytes = 0;
	LastRateTxBytes = 0;
	QueueDepthAccum = 0;
	QueueDepthSamples = 0;
	WriteWakeEvent = FPlatformProcess::GetSynchEventFromPool(false);
	ReadRunnable = MakeUnique<FCefWebSocketReadRunnable>(this);
	WriteRunnable = MakeUnique<FCefWebSocketWriteRunnable>(this, WriteWakeEvent);

	ReadThread = FRunnableThread::Create(ReadRunnable.Get(),
	                                     *FString::Printf(TEXT("CefWsRead_%s"), *NameId.ToString()));
	WriteThread = FRunnableThread::Create(WriteRunnable.Get(),
	                                      *FString::Printf(TEXT("CefWsWrite_%s"), *NameId.ToString()));
	if (!ReadThread || !WriteThread)
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

	bRunning.Store(false);

	if (ReadRunnable)
	{
		ReadRunnable->Stop();
	}
	if (WriteRunnable)
	{
		WriteRunnable->Stop();
	}
	WakeWriteThread();

	if (ReadThread)
	{
		ReadThread->WaitForCompletion();
		delete ReadThread;
		ReadThread = nullptr;
	}
	if (WriteThread)
	{
		WriteThread->WaitForCompletion();
		delete WriteThread;
		WriteThread = nullptr;
	}
	ReadRunnable.Reset();
	WriteRunnable.Reset();

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
		Stats.ActiveClients = 0;
		Stats.QueueDepth = 0;
		Stats.RxBytesPerSec = 0.0f;
		Stats.TxBytesPerSec = 0.0f;
		Stats.AvgQueueDepth = 0.0f;
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

void FCefWebSocketServerInstance::TickBackendOnReadThread()
{
	if (!bRunning.Load() || !Backend)
	{
		return;
	}
	Backend->Tick();
}

bool FCefWebSocketServerInstance::PumpOutgoingOnWriteThread()
{
	if (!bRunning.Load())
	{
		return false;
	}

	struct FSendWork
	{
		int64 clientId = 0;
		ICefNetWebSocket* Socket = nullptr;
		FCefOutboundMessage message;
	};

	TArray<FSendWork> workItems;
	{
		FScopeLock lock(&ClientLock);
		for (TPair<int64, FCefClientState>& pair : Clients)
		{
			FCefOutboundMessage message;
			if (pair.Value.Outbox && pair.Value.Outbox->Dequeue(message))
			{
				pair.Value.QueueMessages = FMath::Max(0, pair.Value.QueueMessages - 1);
				pair.Value.QueueBytes = FMath::Max<int64>(0, pair.Value.QueueBytes - message.Payload.Num());
				Stats.QueueDepth = FMath::Max<int64>(0, Stats.QueueDepth - 1);
				RecordQueueDepthSample_NoLock();
				FSendWork& item = workItems.AddDefaulted_GetRef();
				item.clientId = pair.Key;
				item.Socket = pair.Value.Socket;
				item.message = MoveTemp(message);
			}
		}
	}

	if (workItems.Num() == 0)
	{
		return false;
	}

	int64 sentBytes = 0;
	for (const FSendWork& item : workItems)
	{
		if (!item.Socket || item.message.Payload.Num() == 0)
		{
			continue;
		}
		if (!item.Socket->Send(item.message.Payload.GetData(), static_cast<uint32>(item.message.Payload.Num()), false))
		{
			if (OwnerServer.IsValid())
			{
				OwnerServer->NotifyClientError(item.clientId, ECefWebSocketErrorCode::SocketSendFailed,
				                               TEXT("Socket send failed"));
			}
			continue;
		}
		sentBytes += item.message.Payload.Num();
	}

	{
		FScopeLock lock(&ClientLock);
		Stats.TxBytes += sentBytes;
		UpdateRateStats_NoLock();
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
		ClientIdsBySocket.Add(InSocket, info.ClientId);
		Stats.ActiveClients = Clients.Num();
		RecordQueueDepthSample_NoLock();
	}

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
			ClientIdsBySocket.Remove(InSocket);
			Clients.Remove(clientId);
			Stats.ActiveClients = Clients.Num();
			RecordQueueDepthSample_NoLock();
		}
	}

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

	int64 clientId = 0;
	{
		FScopeLock lock(&ClientLock);
		if (int64* found = ClientIdsBySocket.Find(InSocket))
		{
			clientId = *found;
			Stats.RxBytes += InCount;
			UpdateRateStats_NoLock();
		}
	}

	if (clientId == 0)
	{
		return;
	}

	SET_DWORD_STAT(STAT_CefWs_RxBytes, static_cast<uint32>(FMath::Min<int64>(Stats.RxBytes, MAX_uint32)));

	TArray<uint8> payload;
	payload.Append(InData, InCount);
	if (OwnerServer.IsValid())
	{
		OwnerServer->NotifyClientMessage(clientId, payload, bInBinary);
	}
}

ECefWebSocketSendResult FCefWebSocketServerInstance::EnqueueToClient(int64 InClientId, const uint8* InData,
                                                                     int32 InCount, bool bInBinary)
{
	if (!IsRunning())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	if (!InData || InCount <= 0)
	{
		return ECefWebSocketSendResult::InternalError;
	}
	if (InCount > CefWebSocketCVars::GetMaxMessageBytes())
	{
		return ECefWebSocketSendResult::TooLarge;
	}

	FScopeLock lock(&ClientLock);
	FCefClientState* found = Clients.Find(InClientId);
	if (!found || !found->Socket)
	{
		return ECefWebSocketSendResult::InvalidClient;
	}

	const int32 maxMessages = FMath::Max(1, CefWebSocketCVars::GetMaxQueueMessagesPerClient());
	const int32 maxBytes = FMath::Max(1, CefWebSocketCVars::GetMaxQueueBytesPerClient());

	while ((found->QueueMessages >= maxMessages || found->QueueBytes + InCount > maxBytes) && found->QueueMessages > 0)
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

	if (found->QueueMessages >= maxMessages || found->QueueBytes + InCount > maxBytes)
	{
		Stats.DroppedMessages += 1;
		return ECefWebSocketSendResult::QueueFull;
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
	RecordQueueDepthSample_NoLock();

	WakeWriteThread();
	return ECefWebSocketSendResult::Ok;
}

ECefWebSocketSendResult FCefWebSocketServerInstance::EnqueueToClients(const TArray<int64>& InClientIds,
                                                                      const uint8* InData, int32 InCount,
                                                                      bool bInBinary)
{
	ECefWebSocketSendResult finalResult = ECefWebSocketSendResult::Ok;
	for (const int64 clientId : InClientIds)
	{
		const ECefWebSocketSendResult result = EnqueueToClient(clientId, InData, InCount, bInBinary);
		if (result != ECefWebSocketSendResult::Ok)
		{
			finalResult = result;
		}
	}
	return finalResult;
}

void FCefWebSocketServerInstance::WakeWriteThread()
{
	if (WriteWakeEvent)
	{
		WriteWakeEvent->Trigger();
	}
}

ECefWebSocketSendResult FCefWebSocketServerInstance::SendToClientString(int64 InClientId, const FString& InMessage)
{
	if (!IsRunning())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}

	FTCHARToUTF8 utf8(*InMessage);
	if (utf8.Length() <= 0)
	{
		return ECefWebSocketSendResult::InvalidUtf8;
	}
	return EnqueueToClient(InClientId, reinterpret_cast<const uint8*>(utf8.Get()), utf8.Length(), false);
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
	return EnqueueToClient(InClientId, InBytes.GetData(), InBytes.Num(), true);
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastString(const FString& InMessage)
{
	FTCHARToUTF8 utf8(*InMessage);
	if (utf8.Length() <= 0)
	{
		return ECefWebSocketSendResult::InvalidUtf8;
	}

	TArray<int64> targetClients;
	{
		FScopeLock lock(&ClientLock);
		Clients.GetKeys(targetClients);
	}
	return EnqueueToClients(targetClients, reinterpret_cast<const uint8*>(utf8.Get()), utf8.Length(), false);
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastBytes(const TArray<uint8>& InBytes)
{
	if (InBytes.Num() == 0)
	{
		return ECefWebSocketSendResult::InternalError;
	}
	TArray<int64> targetClients;
	{
		FScopeLock lock(&ClientLock);
		Clients.GetKeys(targetClients);
	}
	return EnqueueToClients(targetClients, InBytes.GetData(), InBytes.Num(), true);
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastStringExcept(
	int64 InExcludedClientId, const FString& InMessage)
{
	FTCHARToUTF8 utf8(*InMessage);
	if (utf8.Length() <= 0)
	{
		return ECefWebSocketSendResult::InvalidUtf8;
	}
	TArray<int64> targetClients;
	{
		FScopeLock lock(&ClientLock);
		for (const TPair<int64, FCefClientState>& pair : Clients)
		{
			if (pair.Key != InExcludedClientId)
			{
				targetClients.Add(pair.Key);
			}
		}
	}
	return EnqueueToClients(targetClients, reinterpret_cast<const uint8*>(utf8.Get()), utf8.Length(), false);
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastBytesExcept(
	int64 InExcludedClientId, const TArray<uint8>& InBytes)
{
	if (InBytes.Num() == 0)
	{
		return ECefWebSocketSendResult::InternalError;
	}
	TArray<int64> targetClients;
	{
		FScopeLock lock(&ClientLock);
		for (const TPair<int64, FCefClientState>& pair : Clients)
		{
			if (pair.Key != InExcludedClientId)
			{
				targetClients.Add(pair.Key);
			}
		}
	}
	return EnqueueToClients(targetClients, InBytes.GetData(), InBytes.Num(), true);
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
		const_cast<FCefWebSocketServerInstance*>(this)->Stats.AvgQueueDepth = static_cast<float>(static_cast<double>(
			QueueDepthAccum) / static_cast<double>(QueueDepthSamples));
	}
	else
	{
		const_cast<FCefWebSocketServerInstance*>(this)->Stats.AvgQueueDepth = static_cast<float>(Stats.QueueDepth);
	}
	return Stats;
}

FCefWebSocketClientInfo FCefWebSocketServerInstance::FindClientInfo(int64 ClientId) const
{
	FScopeLock lock(&ClientLock);
	if (const FCefClientState* found = Clients.Find(ClientId))
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
		Stats.AvgQueueDepth = static_cast<float>(static_cast<double>(QueueDepthAccum) / static_cast<double>(
			QueueDepthSamples));
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
