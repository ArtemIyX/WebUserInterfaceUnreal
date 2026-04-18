#include "Server/CefWebSocketServerBase.h"

#include "Async/Async.h"
#include "Data/CefWebSocketStructs.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Server/CefWebSocketClientBase.h"
#include "Server/CefWebSocketServerInstance.h"

void UCefWebSocketServerBase::BeginDestroy()
{
	StopServer();
	Super::BeginDestroy();
}

bool UCefWebSocketServerBase::StartServerInternal(FName InNameId, int32 InBoundPort, TSubclassOf<UCefWebSocketClientBase> InClientClass)
{
	NameId = InNameId;
	BoundPort = InBoundPort;
	ClientClass = InClientClass;

	if (!Instance.IsValid())
	{
		Instance = MakeShared<FCefWebSocketServerInstance>(NameId, BoundPort, this);
	}

	return Instance->Start();
}

void UCefWebSocketServerBase::AttachInstance(TSharedPtr<FCefWebSocketServerInstance> InInstance)
{
	Instance = InInstance;
}

bool UCefWebSocketServerBase::IsRunning() const
{
	return Instance.IsValid() && Instance->IsRunning();
}

ECefWebSocketSendResult UCefWebSocketServerBase::SendToClientString(int64 ClientId, const FString& Message)
{
	if (!Instance.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return Instance->SendToClientString(ClientId, Message);
}

ECefWebSocketSendResult UCefWebSocketServerBase::SendToClientBytes(int64 ClientId, const TArray<uint8>& Bytes)
{
	if (!Instance.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return Instance->SendToClientBytes(ClientId, Bytes);
}

ECefWebSocketSendResult UCefWebSocketServerBase::SendToClientJson(int64 ClientId, const TSharedPtr<FJsonObject>& JsonObject, const TSharedPtr<FJsonValue>& JsonValue)
{
	if (!JsonObject.IsValid() && !JsonValue.IsValid())
	{
		return ECefWebSocketSendResult::SerializeFailed;
	}

	FString Serialized;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Serialized);
	if (JsonObject.IsValid())
	{
		if (!FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
		{
			return ECefWebSocketSendResult::SerializeFailed;
		}
	}
	else
	{
		if (!FJsonSerializer::Serialize(JsonValue.ToSharedRef(), TEXT(""), Writer))
		{
			return ECefWebSocketSendResult::SerializeFailed;
		}
	}

	return SendToClientString(ClientId, Serialized);
}

ECefWebSocketSendResult UCefWebSocketServerBase::BroadcastString(const FString& Message)
{
	if (!Instance.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return Instance->BroadcastString(Message);
}

ECefWebSocketSendResult UCefWebSocketServerBase::BroadcastBytes(const TArray<uint8>& Bytes)
{
	if (!Instance.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return Instance->BroadcastBytes(Bytes);
}

ECefWebSocketSendResult UCefWebSocketServerBase::BroadcastJson(const TSharedPtr<FJsonObject>& JsonObject, const TSharedPtr<FJsonValue>& JsonValue)
{
	if (!JsonObject.IsValid() && !JsonValue.IsValid())
	{
		return ECefWebSocketSendResult::SerializeFailed;
	}

	FString Serialized;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Serialized);
	if (JsonObject.IsValid())
	{
		if (!FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
		{
			return ECefWebSocketSendResult::SerializeFailed;
		}
	}
	else
	{
		if (!FJsonSerializer::Serialize(JsonValue.ToSharedRef(), TEXT(""), Writer))
		{
			return ECefWebSocketSendResult::SerializeFailed;
		}
	}

	return BroadcastString(Serialized);
}

ECefWebSocketSendResult UCefWebSocketServerBase::BroadcastStringExcept(int64 ExcludedClientId, const FString& Message)
{
	if (!Instance.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return Instance->BroadcastStringExcept(ExcludedClientId, Message);
}

ECefWebSocketSendResult UCefWebSocketServerBase::BroadcastBytesExcept(int64 ExcludedClientId, const TArray<uint8>& Bytes)
{
	if (!Instance.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return Instance->BroadcastBytesExcept(ExcludedClientId, Bytes);
}

ECefWebSocketSendResult UCefWebSocketServerBase::DisconnectClient(int64 ClientId, ECefWebSocketCloseReason Reason)
{
	if (!Instance.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return Instance->DisconnectClient(ClientId, Reason);
}

void UCefWebSocketServerBase::StopServer()
{
	if (Instance.IsValid())
	{
		Instance->Stop();
	}
	FScopeLock Lock(&ClientObjectsLock);
	ClientObjects.Empty();
}

TArray<UCefWebSocketClientBase*> UCefWebSocketServerBase::GetClients() const
{
	TArray<UCefWebSocketClientBase*> OutClients;
	FScopeLock Lock(&ClientObjectsLock);
	for (const TPair<int64, TObjectPtr<UCefWebSocketClientBase>>& Pair : ClientObjects)
	{
		if (Pair.Value)
		{
			OutClients.Add(Pair.Value);
		}
	}
	return OutClients;
}

UCefWebSocketClientBase* UCefWebSocketServerBase::GetClient(int64 ClientId) const
{
	FScopeLock Lock(&ClientObjectsLock);
	if (const TObjectPtr<UCefWebSocketClientBase>* Found = ClientObjects.Find(ClientId))
	{
		return *Found;
	}
	return nullptr;
}

FCefWebSocketServerStats UCefWebSocketServerBase::GetStats() const
{
	if (!Instance.IsValid())
	{
		return FCefWebSocketServerStats();
	}
	return Instance->GetStats();
}

void UCefWebSocketServerBase::NotifyClientConnected(const FCefWebSocketClientInfo& ClientInfo)
{
	if (!IsInGameThread())
	{
		TWeakObjectPtr<UCefWebSocketServerBase> WeakThis(this);
		AsyncTask(ENamedThreads::GameThread, [WeakThis, ClientInfo]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->NotifyClientConnected(ClientInfo);
			}
		});
		return;
	}

	UClass* ClientUClass = ClientClass ? ClientClass.Get() : UCefWebSocketClientBase::StaticClass();
	UCefWebSocketClientBase* ClientObject = NewObject<UCefWebSocketClientBase>(this, ClientUClass);
	if (!ClientObject)
	{
		NotifyClientError(ClientInfo.ClientId, ECefWebSocketErrorCode::Unknown, TEXT("Failed to create client object"));
		return;
	}

	ClientObject->InitializeClient(this, ClientInfo);
	{
		FScopeLock Lock(&ClientObjectsLock);
		ClientObjects.Add(ClientInfo.ClientId, ClientObject);
	}
	OnClientConnected.Broadcast(ClientInfo);
}

void UCefWebSocketServerBase::NotifyClientDisconnected(int64 ClientId, ECefWebSocketCloseReason Reason)
{
	if (!IsInGameThread())
	{
		TWeakObjectPtr<UCefWebSocketServerBase> WeakThis(this);
		AsyncTask(ENamedThreads::GameThread, [WeakThis, ClientId, Reason]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->NotifyClientDisconnected(ClientId, Reason);
			}
		});
		return;
	}

	{
		FScopeLock Lock(&ClientObjectsLock);
		ClientObjects.Remove(ClientId);
	}
	OnClientDisconnected.Broadcast(ClientId, Reason);
}

void UCefWebSocketServerBase::NotifyServerError(ECefWebSocketErrorCode ErrorCode, const FString& Message)
{
	if (!IsInGameThread())
	{
		TWeakObjectPtr<UCefWebSocketServerBase> WeakThis(this);
		AsyncTask(ENamedThreads::GameThread, [WeakThis, ErrorCode, Message]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->NotifyServerError(ErrorCode, Message);
			}
		});
		return;
	}
	OnServerError.Broadcast(NameId, ErrorCode, Message);
}

void UCefWebSocketServerBase::NotifyClientError(int64 ClientId, ECefWebSocketErrorCode ErrorCode, const FString& Message)
{
	if (!IsInGameThread())
	{
		TWeakObjectPtr<UCefWebSocketServerBase> WeakThis(this);
		AsyncTask(ENamedThreads::GameThread, [WeakThis, ClientId, ErrorCode, Message]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->NotifyClientError(ClientId, ErrorCode, Message);
			}
		});
		return;
	}
	OnClientError.Broadcast(ClientId, ErrorCode, Message);
}

void UCefWebSocketServerBase::NotifyClientMessage(int64 ClientId, const TArray<uint8>& Payload, bool bBinary)
{
	UCefWebSocketClientBase* Client = nullptr;
	{
		FScopeLock Lock(&ClientObjectsLock);
		if (const TObjectPtr<UCefWebSocketClientBase>* Found = ClientObjects.Find(ClientId))
		{
			Client = *Found;
		}
	}
	if (!Client)
	{
		return;
	}

	Client->HandleBytesFromClient(Payload);
	HandleClientBytes(Client, Payload);

	if (!bBinary)
	{
		FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(Payload.GetData()), Payload.Num());
		const FString Text(Converter.Length(), Converter.Get());
		Client->HandleStringFromClient(Text);
		HandleClientString(Client, Text);
	}
}

void UCefWebSocketServerBase::HandleClientBytes(UCefWebSocketClientBase* Client, const TArray<uint8>& Data)
{
}

void UCefWebSocketServerBase::HandleClientString(UCefWebSocketClientBase* Client, const FString& Message)
{
}
