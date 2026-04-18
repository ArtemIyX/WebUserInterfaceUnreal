#include "Server/CefWebSocketServerBase.h"

#include "Data/CefWebSocketStructs.h"`r`n#include "Serialization/JsonSerializer.h"`r`n#include "Serialization/JsonWriter.h"
#include "JsonObjectConverter.h"
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
	if (JsonObject.IsValid())
	{
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Serialized);
		if (!FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
		{
			return ECefWebSocketSendResult::SerializeFailed;
		}
	}
	else
	{
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Serialized);
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
	if (JsonObject.IsValid())
	{
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Serialized);
		if (!FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
		{
			return ECefWebSocketSendResult::SerializeFailed;
		}
	}
	else
	{
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Serialized);
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
	ClientObjects.Remove(ClientId);
	return Instance->DisconnectClient(ClientId, Reason);
}

void UCefWebSocketServerBase::StopServer()
{
	if (Instance.IsValid())
	{
		Instance->Stop();
	}
	ClientObjects.Empty();
}

TArray<UCefWebSocketClientBase*> UCefWebSocketServerBase::GetClients() const
{
	TArray<UCefWebSocketClientBase*> OutClients;
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

void UCefWebSocketServerBase::HandleClientBytes(UCefWebSocketClientBase* Client, const TArray<uint8>& Data)
{
}

void UCefWebSocketServerBase::HandleClientString(UCefWebSocketClientBase* Client, const FString& Message)
{
}

