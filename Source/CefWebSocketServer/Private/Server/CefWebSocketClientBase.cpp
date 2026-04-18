#include "Server/CefWebSocketClientBase.h"

#include "Server/CefWebSocketServerBase.h"

void UCefWebSocketClientBase::InitializeClient(TWeakObjectPtr<UCefWebSocketServerBase> InOwnerServer, const FCefWebSocketClientInfo& InInfo)
{
	OwnerServer = InOwnerServer;
	ClientInfo = InInfo;
}

ECefWebSocketSendResult UCefWebSocketClientBase::SendString(const FString& Message)
{
	if (!OwnerServer.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return OwnerServer->SendToClientString(ClientInfo.ClientId, Message);
}

ECefWebSocketSendResult UCefWebSocketClientBase::SendBytes(const TArray<uint8>& Bytes)
{
	if (!OwnerServer.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return OwnerServer->SendToClientBytes(ClientInfo.ClientId, Bytes);
}

ECefWebSocketSendResult UCefWebSocketClientBase::Disconnect(ECefWebSocketCloseReason Reason)
{
	if (!OwnerServer.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return OwnerServer->DisconnectClient(ClientInfo.ClientId, Reason);
}

void UCefWebSocketClientBase::HandleBytesFromClient(const TArray<uint8>& Data)
{
}

void UCefWebSocketClientBase::HandleStringFromClient(const FString& Message)
{
}
