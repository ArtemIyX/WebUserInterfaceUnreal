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

FCefWebSocketServerInstance::FCefWebSocketServerInstance(FName InNameId, int32 InBoundPort, TWeakObjectPtr<UCefWebSocketServerBase> InOwnerServer)
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
	FCefNetWebSocketClientConnectedCallback OnConnected;
	OnConnected.BindRaw(this, &FCefWebSocketServerInstance::HandleClientConnected);
	FCefNetWebSocketClientDisconnectedCallback OnDisconnected;
	OnDisconnected.BindRaw(this, &FCefWebSocketServerInstance::HandleClientDisconnected);

	if (!Backend->Init(static_cast<uint32>(BoundPort), OnConnected, OnDisconnected))
	{
		if (OwnerServer.IsValid())
		{
			OwnerServer->NotifyServerError(ECefWebSocketErrorCode::ServerInitFailed, FString::Printf(TEXT("Failed to start server %s on port %d"), *NameId.ToString(), BoundPort));
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

	ReadThread = FRunnableThread::Create(ReadRunnable.Get(), *FString::Printf(TEXT("CefWsRead_%s"), *NameId.ToString()));
	WriteThread = FRunnableThread::Create(WriteRunnable.Get(), *FString::Printf(TEXT("CefWsWrite_%s"), *NameId.ToString()));
	if (!ReadThread || !WriteThread)
	{
		Stop();
		if (OwnerServer.IsValid())
		{
			OwnerServer->NotifyServerError(ECefWebSocketErrorCode::ServerInitFailed, TEXT("Failed to create websocket server threads"));
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

	TArray<ICefNetWebSocket*> ToClose;
	{
		FScopeLock Lock(&ClientLock);
		for (TPair<int64, FCefClientState>& Pair : Clients)
		{
			if (Pair.Value.Socket)
			{
				ToClose.Add(Pair.Value.Socket);
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

	for (ICefNetWebSocket* Socket : ToClose)
	{
		Socket->Close();
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
		int64 ClientId = 0;
		ICefNetWebSocket* Socket = nullptr;
		FCefOutboundMessage Message;
	};

	TArray<FSendWork> Work;
	{
		FScopeLock Lock(&ClientLock);
		for (TPair<int64, FCefClientState>& Pair : Clients)
		{
			FCefOutboundMessage Msg;
			if (Pair.Value.Outbox.Dequeue(Msg))
			{
				Pair.Value.QueueMessages = FMath::Max(0, Pair.Value.QueueMessages - 1);
				Pair.Value.QueueBytes = FMath::Max<int64>(0, Pair.Value.QueueBytes - Msg.Payload.Num());
				Stats.QueueDepth = FMath::Max<int64>(0, Stats.QueueDepth - 1);
				RecordQueueDepthSample_NoLock();
				FSendWork& Item = Work.AddDefaulted_GetRef();
				Item.ClientId = Pair.Key;
				Item.Socket = Pair.Value.Socket;
				Item.Message = MoveTemp(Msg);
			}
		}
	}

	if (Work.Num() == 0)
	{
		return false;
	}

	int64 SentBytes = 0;
	for (const FSendWork& Item : Work)
	{
		if (!Item.Socket || Item.Message.Payload.Num() == 0)
		{
			continue;
		}
		if (!Item.Socket->Send(Item.Message.Payload.GetData(), static_cast<uint32>(Item.Message.Payload.Num()), false))
		{
			if (OwnerServer.IsValid())
			{
				OwnerServer->NotifyClientError(Item.ClientId, ECefWebSocketErrorCode::SocketSendFailed, TEXT("Socket send failed"));
			}
			continue;
		}
		SentBytes += Item.Message.Payload.Num();
	}

	{
		FScopeLock Lock(&ClientLock);
		Stats.TxBytes += SentBytes;
		UpdateRateStats_NoLock();
	}

	SET_DWORD_STAT(STAT_CefWs_ActiveClients, Stats.ActiveClients);
	SET_DWORD_STAT(STAT_CefWs_QueueDepth, static_cast<uint32>(FMath::Min<int64>(Stats.QueueDepth, MAX_uint32)));
	SET_DWORD_STAT(STAT_CefWs_TxBytes, static_cast<uint32>(FMath::Min<int64>(Stats.TxBytes, MAX_uint32)));
	SET_DWORD_STAT(STAT_CefWs_DroppedMessages, static_cast<uint32>(FMath::Min<int64>(Stats.DroppedMessages, MAX_uint32)));
	return true;
}

void FCefWebSocketServerInstance::HandleClientConnected(ICefNetWebSocket* Socket)
{
	if (!Socket)
	{
		return;
	}

	FCefWebSocketClientInfo Info;
	Info.ClientId = NextClientId++;
	Info.RemoteAddress = Socket->RemoteEndPoint(true);
	Info.ConnectedAt = FDateTime::UtcNow();

	FCefNetWebSocketPacketReceivedCallback OnReceive;
	OnReceive.BindLambda([this, Socket](void* Data, int32 Count, bool bBinary)
	{
		HandleClientPacket(Socket, static_cast<const uint8*>(Data), Count, bBinary);
	});
	Socket->SetReceiveCallBack(OnReceive);

	{
		FScopeLock Lock(&ClientLock);
		FCefClientState State;
		State.Info = Info;
		State.Socket = Socket;
		Clients.Add(Info.ClientId, MoveTemp(State));
		ClientIdsBySocket.Add(Socket, Info.ClientId);
		Stats.ActiveClients = Clients.Num();
		RecordQueueDepthSample_NoLock();
	}

	if (OwnerServer.IsValid())
	{
		OwnerServer->NotifyClientConnected(Info);
	}
}

void FCefWebSocketServerInstance::HandleClientDisconnected(ICefNetWebSocket* Socket)
{
	if (!Socket)
	{
		return;
	}

	int64 ClientId = 0;
	{
		FScopeLock Lock(&ClientLock);
		if (int64* Found = ClientIdsBySocket.Find(Socket))
		{
			ClientId = *Found;
			ClientIdsBySocket.Remove(Socket);
			Clients.Remove(ClientId);
			Stats.ActiveClients = Clients.Num();
			RecordQueueDepthSample_NoLock();
		}
	}

	if (ClientId != 0 && OwnerServer.IsValid())
	{
		OwnerServer->NotifyClientDisconnected(ClientId, ECefWebSocketCloseReason::ClientClosed);
	}
}

void FCefWebSocketServerInstance::HandleClientPacket(ICefNetWebSocket* Socket, const uint8* Data, int32 Count, bool bBinary)
{
	if (!Socket || !Data || Count <= 0)
	{
		return;
	}
	if (Count > CefWebSocketCVars::GetMaxMessageBytes())
	{
		int64 ClientId = 0;
		{
			FScopeLock Lock(&ClientLock);
			if (int64* Found = ClientIdsBySocket.Find(Socket))
			{
				ClientId = *Found;
			}
		}
		if (ClientId != 0 && OwnerServer.IsValid())
		{
			OwnerServer->NotifyClientError(ClientId, ECefWebSocketErrorCode::InvalidPayload, TEXT("Incoming message exceeds cefws.max_message_bytes"));
		}
		return;
	}

	int64 ClientId = 0;
	{
		FScopeLock Lock(&ClientLock);
		if (int64* Found = ClientIdsBySocket.Find(Socket))
		{
			ClientId = *Found;
			Stats.RxBytes += Count;
			UpdateRateStats_NoLock();
		}
	}

	if (ClientId == 0)
	{
		return;
	}

	SET_DWORD_STAT(STAT_CefWs_RxBytes, static_cast<uint32>(FMath::Min<int64>(Stats.RxBytes, MAX_uint32)));

	TArray<uint8> Payload;
	Payload.Append(Data, Count);
	if (OwnerServer.IsValid())
	{
		OwnerServer->NotifyClientMessage(ClientId, Payload, bBinary);
	}
}

ECefWebSocketSendResult FCefWebSocketServerInstance::EnqueueToClient(int64 ClientId, const uint8* Data, int32 Count, bool bBinary)
{
	if (!IsRunning())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	if (!Data || Count <= 0)
	{
		return ECefWebSocketSendResult::InternalError;
	}
	if (Count > CefWebSocketCVars::GetMaxMessageBytes())
	{
		return ECefWebSocketSendResult::TooLarge;
	}

	FScopeLock Lock(&ClientLock);
	FCefClientState* Found = Clients.Find(ClientId);
	if (!Found || !Found->Socket)
	{
		return ECefWebSocketSendResult::InvalidClient;
	}

	const int32 MaxMessages = FMath::Max(1, CefWebSocketCVars::GetMaxQueueMessagesPerClient());
	const int32 MaxBytes = FMath::Max(1, CefWebSocketCVars::GetMaxQueueBytesPerClient());

	while ((Found->QueueMessages >= MaxMessages || Found->QueueBytes + Count > MaxBytes) && Found->QueueMessages > 0)
	{
		FCefOutboundMessage Dropped;
		if (!Found->Outbox.Dequeue(Dropped))
		{
			break;
		}
		Found->QueueMessages = FMath::Max(0, Found->QueueMessages - 1);
		Found->QueueBytes = FMath::Max<int64>(0, Found->QueueBytes - Dropped.Payload.Num());
		Stats.DroppedMessages += 1;
		Stats.QueueDepth = FMath::Max<int64>(0, Stats.QueueDepth - 1);
		RecordQueueDepthSample_NoLock();
	}

	if (Found->QueueMessages >= MaxMessages || Found->QueueBytes + Count > MaxBytes)
	{
		Stats.DroppedMessages += 1;
		return ECefWebSocketSendResult::QueueFull;
	}

	FCefOutboundMessage Message;
	Message.Payload.Append(Data, Count);
	Message.bBinary = bBinary;
	Found->Outbox.Enqueue(MoveTemp(Message));
	Found->QueueMessages += 1;
	Found->QueueBytes += Count;
	Stats.QueueDepth += 1;
	RecordQueueDepthSample_NoLock();

	WakeWriteThread();
	return ECefWebSocketSendResult::Ok;
}

ECefWebSocketSendResult FCefWebSocketServerInstance::EnqueueToClients(const TArray<int64>& ClientIds, const uint8* Data, int32 Count, bool bBinary)
{
	ECefWebSocketSendResult FinalResult = ECefWebSocketSendResult::Ok;
	for (const int64 ClientId : ClientIds)
	{
		const ECefWebSocketSendResult Result = EnqueueToClient(ClientId, Data, Count, bBinary);
		if (Result != ECefWebSocketSendResult::Ok)
		{
			FinalResult = Result;
		}
	}
	return FinalResult;
}

void FCefWebSocketServerInstance::WakeWriteThread()
{
	if (WriteWakeEvent)
	{
		WriteWakeEvent->Trigger();
	}
}

ECefWebSocketSendResult FCefWebSocketServerInstance::SendToClientString(int64 ClientId, const FString& Message)
{
	if (!IsRunning())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}

	FTCHARToUTF8 Utf8(*Message);
	if (Utf8.Length() <= 0)
	{
		return ECefWebSocketSendResult::InvalidUtf8;
	}
	return EnqueueToClient(ClientId, reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length(), false);
}

ECefWebSocketSendResult FCefWebSocketServerInstance::SendToClientBytes(int64 ClientId, const TArray<uint8>& Bytes)
{
	if (!IsRunning())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	if (Bytes.Num() == 0)
	{
		return ECefWebSocketSendResult::InternalError;
	}
	return EnqueueToClient(ClientId, Bytes.GetData(), Bytes.Num(), true);
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastString(const FString& Message)
{
	FTCHARToUTF8 Utf8(*Message);
	if (Utf8.Length() <= 0)
	{
		return ECefWebSocketSendResult::InvalidUtf8;
	}

	TArray<int64> TargetClients;
	{
		FScopeLock Lock(&ClientLock);
		Clients.GetKeys(TargetClients);
	}
	return EnqueueToClients(TargetClients, reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length(), false);
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastBytes(const TArray<uint8>& Bytes)
{
	if (Bytes.Num() == 0)
	{
		return ECefWebSocketSendResult::InternalError;
	}
	TArray<int64> TargetClients;
	{
		FScopeLock Lock(&ClientLock);
		Clients.GetKeys(TargetClients);
	}
	return EnqueueToClients(TargetClients, Bytes.GetData(), Bytes.Num(), true);
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastStringExcept(int64 ExcludedClientId, const FString& Message)
{
	FTCHARToUTF8 Utf8(*Message);
	if (Utf8.Length() <= 0)
	{
		return ECefWebSocketSendResult::InvalidUtf8;
	}
	TArray<int64> TargetClients;
	{
		FScopeLock Lock(&ClientLock);
		for (const TPair<int64, FCefClientState>& Pair : Clients)
		{
			if (Pair.Key != ExcludedClientId)
			{
				TargetClients.Add(Pair.Key);
			}
		}
	}
	return EnqueueToClients(TargetClients, reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length(), false);
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastBytesExcept(int64 ExcludedClientId, const TArray<uint8>& Bytes)
{
	if (Bytes.Num() == 0)
	{
		return ECefWebSocketSendResult::InternalError;
	}
	TArray<int64> TargetClients;
	{
		FScopeLock Lock(&ClientLock);
		for (const TPair<int64, FCefClientState>& Pair : Clients)
		{
			if (Pair.Key != ExcludedClientId)
			{
				TargetClients.Add(Pair.Key);
			}
		}
	}
	return EnqueueToClients(TargetClients, Bytes.GetData(), Bytes.Num(), true);
}

ECefWebSocketSendResult FCefWebSocketServerInstance::DisconnectClient(int64 ClientId, ECefWebSocketCloseReason Reason)
{
	(void)Reason;
	ICefNetWebSocket* Socket = nullptr;
	{
		FScopeLock Lock(&ClientLock);
		if (FCefClientState* Found = Clients.Find(ClientId))
		{
			Socket = Found->Socket;
		}
	}
	if (!Socket)
	{
		return ECefWebSocketSendResult::InvalidClient;
	}

	Socket->Close();
	return ECefWebSocketSendResult::Ok;
}

TArray<FCefWebSocketClientInfo> FCefWebSocketServerInstance::GetClients() const
{
	FScopeLock Lock(&ClientLock);
	TArray<FCefWebSocketClientInfo> OutClients;
	OutClients.Reserve(Clients.Num());
	for (const TPair<int64, FCefClientState>& Pair : Clients)
	{
		OutClients.Add(Pair.Value.Info);
	}
	return OutClients;
}

FCefWebSocketServerStats FCefWebSocketServerInstance::GetStats() const
{
	FScopeLock Lock(&ClientLock);
	const_cast<FCefWebSocketServerInstance*>(this)->UpdateRateStats_NoLock();
	if (QueueDepthSamples > 0)
	{
		const_cast<FCefWebSocketServerInstance*>(this)->Stats.AvgQueueDepth = static_cast<float>(static_cast<double>(QueueDepthAccum) / static_cast<double>(QueueDepthSamples));
	}
	else
	{
		const_cast<FCefWebSocketServerInstance*>(this)->Stats.AvgQueueDepth = static_cast<float>(Stats.QueueDepth);
	}
	return Stats;
}

FCefWebSocketClientInfo FCefWebSocketServerInstance::FindClientInfo(int64 ClientId) const
{
	FScopeLock Lock(&ClientLock);
	if (const FCefClientState* Found = Clients.Find(ClientId))
	{
		return Found->Info;
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
	const double NowSec = FPlatformTime::Seconds();
	if (LastRateSampleTimeSec <= 0.0)
	{
		LastRateSampleTimeSec = NowSec;
		LastRateRxBytes = Stats.RxBytes;
		LastRateTxBytes = Stats.TxBytes;
		Stats.RxBytesPerSec = 0.0f;
		Stats.TxBytesPerSec = 0.0f;
		return;
	}

	const double DeltaSec = NowSec - LastRateSampleTimeSec;
	if (DeltaSec < 0.05)
	{
		return;
	}

	const int64 DeltaRx = Stats.RxBytes - LastRateRxBytes;
	const int64 DeltaTx = Stats.TxBytes - LastRateTxBytes;
	Stats.RxBytesPerSec = static_cast<float>(static_cast<double>(DeltaRx) / DeltaSec);
	Stats.TxBytesPerSec = static_cast<float>(static_cast<double>(DeltaTx) / DeltaSec);
	LastRateSampleTimeSec = NowSec;
	LastRateRxBytes = Stats.RxBytes;
	LastRateTxBytes = Stats.TxBytes;
}
