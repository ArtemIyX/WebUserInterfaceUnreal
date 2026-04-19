#include "Server/CefWebSocketClientBase.h"

#include "Server/CefWebSocketServerBase.h"

UCefWebSocketClientBase::UCefWebSocketClientBase(const FObjectInitializer& InInitializer) : Super(InInitializer)
{
	
}

void UCefWebSocketClientBase::InitializeClient(TWeakObjectPtr<UCefWebSocketServerBase> InOwnerServer, const FCefWebSocketClientInfo& InInfo)
{
	OwnerServer = InOwnerServer;
	ClientInfo = InInfo;
}



ECefWebSocketSendResult UCefWebSocketClientBase::SendString(const FString& InMessage)
{
	if (!OwnerServer.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return OwnerServer->SendToClientString(ClientInfo.ClientId, InMessage);
}

ECefWebSocketSendResult UCefWebSocketClientBase::SendBytes(const TArray<uint8>& InBytes)
{
	if (!OwnerServer.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return OwnerServer->SendToClientBytes(ClientInfo.ClientId, InBytes);
}

ECefWebSocketSendResult UCefWebSocketClientBase::Disconnect(ECefWebSocketCloseReason InReason)
{
	if (!OwnerServer.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return OwnerServer->DisconnectClient(ClientInfo.ClientId, InReason);
}

void UCefWebSocketClientBase::HandleBytesFromClient(const TArray<uint8>& InData)
{
}

void UCefWebSocketClientBase::HandleStringFromClient(const FString& InMessage)
{
}
