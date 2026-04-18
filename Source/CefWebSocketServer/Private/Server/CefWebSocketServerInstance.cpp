#include "Server/CefWebSocketServerInstance.h"

#include "Config/CefWebSocketCVars.h"
#include "Logging/CefWebSocketLog.h"
#include "Networking/CefWebSocketServerBackend.h"
#include "Networking/ICefNetWebSocket.h"
#include "Server/CefWebSocketServerBase.h"
#include "Stats/CefWebSocketStats.h"

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

	BoundPort = static_cast<int32>(Backend->GetServerPort());
	bRunning.Store(true);
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FCefWebSocketServerInstance::TickServer));
	return true;
}

void FCefWebSocketServerInstance::Stop()
{
	if (!bRunning.Load())
	{
		return;
	}

	bRunning.Store(false);
	if (TickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		TickHandle.Reset();
	}

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
	}

	for (ICefNetWebSocket* Socket : ToClose)
	{
		Socket->Close();
	}

	Backend.Reset();
}

bool FCefWebSocketServerInstance::IsRunning() const
{
	return bRunning.Load();
}

bool FCefWebSocketServerInstance::TickServer(float DeltaTime)
{
	if (!bRunning.Load() || !Backend)
	{
		return false;
	}

	Backend->Tick();
	SET_DWORD_STAT(STAT_CefWs_ActiveClients, Stats.ActiveClients);
	SET_DWORD_STAT(STAT_CefWs_QueueDepth, static_cast<uint32>(Stats.QueueDepth));
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
		Clients.Add(Info.ClientId, State);
		ClientIdsBySocket.Add(Socket, Info.ClientId);
		Stats.ActiveClients = Clients.Num();
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
			SET_DWORD_STAT(STAT_CefWs_RxBytes, static_cast<uint32>(FMath::Min<int64>(Stats.RxBytes, MAX_uint32)));
		}
	}

	if (ClientId == 0)
	{
		return;
	}

	TArray<uint8> Payload;
	Payload.Append(Data, Count);
	if (OwnerServer.IsValid())
	{
		OwnerServer->NotifyClientMessage(ClientId, Payload, bBinary);
	}
}

ECefWebSocketSendResult FCefWebSocketServerInstance::SendToSocket(ICefNetWebSocket* Socket, const uint8* Data, int32 Count)
{
	if (!Socket)
	{
		return ECefWebSocketSendResult::InvalidClient;
	}
	if (!Data || Count <= 0)
	{
		return ECefWebSocketSendResult::InternalError;
	}
	if (Count > CefWebSocketCVars::GetMaxMessageBytes())
	{
		return ECefWebSocketSendResult::TooLarge;
	}

	if (!Socket->Send(Data, static_cast<uint32>(Count), false))
	{
		return ECefWebSocketSendResult::InternalError;
	}

	Stats.TxBytes += Count;
	SET_DWORD_STAT(STAT_CefWs_TxBytes, static_cast<uint32>(FMath::Min<int64>(Stats.TxBytes, MAX_uint32)));
	return ECefWebSocketSendResult::Ok;
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

	ICefNetWebSocket* Socket = nullptr;
	{
		FScopeLock Lock(&ClientLock);
		if (FCefClientState* Found = Clients.Find(ClientId))
		{
			Socket = Found->Socket;
		}
	}
	return SendToSocket(Socket, reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
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

	ICefNetWebSocket* Socket = nullptr;
	{
		FScopeLock Lock(&ClientLock);
		if (FCefClientState* Found = Clients.Find(ClientId))
		{
			Socket = Found->Socket;
		}
	}
	return SendToSocket(Socket, Bytes.GetData(), Bytes.Num());
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastString(const FString& Message)
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

	TArray<ICefNetWebSocket*> Sockets;
	{
		FScopeLock Lock(&ClientLock);
		for (const TPair<int64, FCefClientState>& Pair : Clients)
		{
			if (Pair.Value.Socket)
			{
				Sockets.Add(Pair.Value.Socket);
			}
		}
	}

	ECefWebSocketSendResult Result = ECefWebSocketSendResult::Ok;
	for (ICefNetWebSocket* Socket : Sockets)
	{
		const ECefWebSocketSendResult SendResult = SendToSocket(Socket, reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
		if (SendResult != ECefWebSocketSendResult::Ok)
		{
			Result = SendResult;
		}
	}
	return Result;
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastBytes(const TArray<uint8>& Bytes)
{
	if (!IsRunning())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	if (Bytes.Num() == 0)
	{
		return ECefWebSocketSendResult::InternalError;
	}

	TArray<ICefNetWebSocket*> Sockets;
	{
		FScopeLock Lock(&ClientLock);
		for (const TPair<int64, FCefClientState>& Pair : Clients)
		{
			if (Pair.Value.Socket)
			{
				Sockets.Add(Pair.Value.Socket);
			}
		}
	}

	ECefWebSocketSendResult Result = ECefWebSocketSendResult::Ok;
	for (ICefNetWebSocket* Socket : Sockets)
	{
		const ECefWebSocketSendResult SendResult = SendToSocket(Socket, Bytes.GetData(), Bytes.Num());
		if (SendResult != ECefWebSocketSendResult::Ok)
		{
			Result = SendResult;
		}
	}
	return Result;
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastStringExcept(int64 ExcludedClientId, const FString& Message)
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

	TArray<ICefNetWebSocket*> Sockets;
	{
		FScopeLock Lock(&ClientLock);
		for (const TPair<int64, FCefClientState>& Pair : Clients)
		{
			if (Pair.Key != ExcludedClientId && Pair.Value.Socket)
			{
				Sockets.Add(Pair.Value.Socket);
			}
		}
	}

	ECefWebSocketSendResult Result = ECefWebSocketSendResult::Ok;
	for (ICefNetWebSocket* Socket : Sockets)
	{
		const ECefWebSocketSendResult SendResult = SendToSocket(Socket, reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
		if (SendResult != ECefWebSocketSendResult::Ok)
		{
			Result = SendResult;
		}
	}
	return Result;
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastBytesExcept(int64 ExcludedClientId, const TArray<uint8>& Bytes)
{
	if (!IsRunning())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	if (Bytes.Num() == 0)
	{
		return ECefWebSocketSendResult::InternalError;
	}

	TArray<ICefNetWebSocket*> Sockets;
	{
		FScopeLock Lock(&ClientLock);
		for (const TPair<int64, FCefClientState>& Pair : Clients)
		{
			if (Pair.Key != ExcludedClientId && Pair.Value.Socket)
			{
				Sockets.Add(Pair.Value.Socket);
			}
		}
	}

	ECefWebSocketSendResult Result = ECefWebSocketSendResult::Ok;
	for (ICefNetWebSocket* Socket : Sockets)
	{
		const ECefWebSocketSendResult SendResult = SendToSocket(Socket, Bytes.GetData(), Bytes.Num());
		if (SendResult != ECefWebSocketSendResult::Ok)
		{
			Result = SendResult;
		}
	}
	return Result;
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
