#include "Server/CefWebSocketServerInstance.h"

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
	bRunning.Store(true);
	return true;
}

void FCefWebSocketServerInstance::Stop()
{
	bRunning.Store(false);
	FScopeLock Lock(&ClientLock);
	Clients.Empty();
	Stats.ActiveClients = 0;
}

bool FCefWebSocketServerInstance::IsRunning() const
{
	return bRunning.Load();
}

ECefWebSocketSendResult FCefWebSocketServerInstance::SendToClientString(int64 ClientId, const FString& Message)
{
	if (!IsRunning())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	if (Message.IsEmpty())
	{
		return ECefWebSocketSendResult::InvalidUtf8;
	}
	FScopeLock Lock(&ClientLock);
	if (!Clients.Contains(ClientId))
	{
		return ECefWebSocketSendResult::InvalidClient;
	}
	return ECefWebSocketSendResult::Ok;
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
	FScopeLock Lock(&ClientLock);
	if (!Clients.Contains(ClientId))
	{
		return ECefWebSocketSendResult::InvalidClient;
	}
	return ECefWebSocketSendResult::Ok;
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastString(const FString& Message)
{
	if (!IsRunning())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	if (Message.IsEmpty())
	{
		return ECefWebSocketSendResult::InvalidUtf8;
	}
	return ECefWebSocketSendResult::Ok;
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
	return ECefWebSocketSendResult::Ok;
}

ECefWebSocketSendResult FCefWebSocketServerInstance::BroadcastStringExcept(int64 ExcludedClientId, const FString& Message)
{
	if (!IsRunning())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	if (Message.IsEmpty())
	{
		return ECefWebSocketSendResult::InvalidUtf8;
	}
	return ECefWebSocketSendResult::Ok;
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
	return ECefWebSocketSendResult::Ok;
}

ECefWebSocketSendResult FCefWebSocketServerInstance::DisconnectClient(int64 ClientId, ECefWebSocketCloseReason Reason)
{
	FScopeLock Lock(&ClientLock);
	if (!Clients.Contains(ClientId))
	{
		return ECefWebSocketSendResult::InvalidClient;
	}
	Clients.Remove(ClientId);
	Stats.ActiveClients = Clients.Num();
	return ECefWebSocketSendResult::Ok;
}

TArray<FCefWebSocketClientInfo> FCefWebSocketServerInstance::GetClients() const
{
	FScopeLock Lock(&ClientLock);
	TArray<FCefWebSocketClientInfo> OutClients;
	Clients.GenerateValueArray(OutClients);
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
	if (const FCefWebSocketClientInfo* Found = Clients.Find(ClientId))
	{
		return *Found;
	}
	return FCefWebSocketClientInfo();
}
