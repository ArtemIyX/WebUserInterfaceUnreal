#include "Server/CefWebSocketServerBase.h"

#include "Async/Async.h"
#include "Data/CefWebSocketStructs.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Server/CefWebSocketClientBase.h"
#include "Server/CefWebSocketServerInstance.h"

UCefWebSocketServerBase::UCefWebSocketServerBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UCefWebSocketServerBase::BeginDestroy()
{
	StopServer();
	Super::BeginDestroy();
}

bool UCefWebSocketServerBase::StartServerInternal(FName InNameId, int32 InBoundPort,
                                                  TSubclassOf<UCefWebSocketClientBase> InClientClass)
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

ECefWebSocketSendResult UCefWebSocketServerBase::SendToClientString(int64 InClientId, const FString& InMessage)
{
	if (!Instance.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return Instance->SendToClientString(InClientId, InMessage);
}

ECefWebSocketSendResult UCefWebSocketServerBase::SendToClientBytes(int64 InClientId, const TArray<uint8>& InBytes)
{
	if (!Instance.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return Instance->SendToClientBytes(InClientId, InBytes);
}

ECefWebSocketSendResult UCefWebSocketServerBase::SendToClientJson(int64 InClientId,
                                                                  const TSharedPtr<FJsonObject>& InJsonObject,
                                                                  const TSharedPtr<FJsonValue>& InJsonValue)
{
	if (!InJsonObject.IsValid() && !InJsonValue.IsValid())
	{
		return ECefWebSocketSendResult::SerializeFailed;
	}

	FString serialized;
	TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&serialized);
	if (InJsonObject.IsValid())
	{
		if (!FJsonSerializer::Serialize(InJsonObject.ToSharedRef(), writer))
		{
			return ECefWebSocketSendResult::SerializeFailed;
		}
	}
	else
	{
		if (!FJsonSerializer::Serialize(InJsonValue.ToSharedRef(), TEXT(""), writer))
		{
			return ECefWebSocketSendResult::SerializeFailed;
		}
	}

	return SendToClientString(InClientId, serialized);
}

ECefWebSocketSendResult UCefWebSocketServerBase::BroadcastString(const FString& InMessage)
{
	if (!Instance.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return Instance->BroadcastString(InMessage);
}

ECefWebSocketSendResult UCefWebSocketServerBase::BroadcastBytes(const TArray<uint8>& InBytes)
{
	if (!Instance.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return Instance->BroadcastBytes(InBytes);
}

ECefWebSocketSendResult UCefWebSocketServerBase::BroadcastJson(const TSharedPtr<FJsonObject>& InJsonObject,
                                                               const TSharedPtr<FJsonValue>& InJsonValue)
{
	if (!InJsonObject.IsValid() && !InJsonValue.IsValid())
	{
		return ECefWebSocketSendResult::SerializeFailed;
	}

	FString serialized;
	TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&serialized);
	if (InJsonObject.IsValid())
	{
		if (!FJsonSerializer::Serialize(InJsonObject.ToSharedRef(), writer))
		{
			return ECefWebSocketSendResult::SerializeFailed;
		}
	}
	else
	{
		if (!FJsonSerializer::Serialize(InJsonValue.ToSharedRef(), TEXT(""), writer))
		{
			return ECefWebSocketSendResult::SerializeFailed;
		}
	}

	return BroadcastString(serialized);
}

ECefWebSocketSendResult UCefWebSocketServerBase::BroadcastStringExcept(int64 InExcludedClientId,
                                                                       const FString& InMessage)
{
	if (!Instance.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return Instance->BroadcastStringExcept(InExcludedClientId, InMessage);
}

ECefWebSocketSendResult UCefWebSocketServerBase::BroadcastBytesExcept(int64 InExcludedClientId,
                                                                      const TArray<uint8>& InBytes)
{
	if (!Instance.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return Instance->BroadcastBytesExcept(InExcludedClientId, InBytes);
}

ECefWebSocketSendResult UCefWebSocketServerBase::DisconnectClient(int64 InClientId, ECefWebSocketCloseReason InReason)
{
	if (!Instance.IsValid())
	{
		return ECefWebSocketSendResult::InvalidServer;
	}
	return Instance->DisconnectClient(InClientId, InReason);
}

void UCefWebSocketServerBase::StopServer()
{
	if (Instance.IsValid())
	{
		Instance->Stop();
	}
	FScopeLock lock(&ClientObjectsLock);
	ClientObjects.Empty();
}

TArray<UCefWebSocketClientBase*> UCefWebSocketServerBase::GetClients() const
{
	TArray<UCefWebSocketClientBase*> outClients;
	FScopeLock lock(&ClientObjectsLock);
	for (const TPair<int64, TObjectPtr<UCefWebSocketClientBase>>& Pair : ClientObjects)
	{
		if (Pair.Value)
		{
			outClients.Add(Pair.Value);
		}
	}
	return outClients;
}

UCefWebSocketClientBase* UCefWebSocketServerBase::GetClient(int64 InClientId) const
{
	FScopeLock lock(&ClientObjectsLock);
	if (const TObjectPtr<UCefWebSocketClientBase>* Found = ClientObjects.Find(InClientId))
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

void UCefWebSocketServerBase::NotifyClientConnected(const FCefWebSocketClientInfo& InClientInfo)
{
	if (!IsInGameThread())
	{
		TWeakObjectPtr<UCefWebSocketServerBase> weakThis(this);
		AsyncTask(ENamedThreads::GameThread, [weakThis, InClientInfo]()
		{
			if (weakThis.IsValid())
			{
				weakThis->NotifyClientConnected(InClientInfo);
			}
		});
		return;
	}

	UClass* ClientUClass = ClientClass ? ClientClass.Get() : UCefWebSocketClientBase::StaticClass();
	UCefWebSocketClientBase* ClientObject = NewObject<UCefWebSocketClientBase>(this, ClientUClass);
	if (!ClientObject)
	{
		NotifyClientError(InClientInfo.ClientId, ECefWebSocketErrorCode::Unknown,
		                  TEXT("Failed to create client object"));
		return;
	}

	ClientObject->InitializeClient(this, InClientInfo);
	{
		FScopeLock lock(&ClientObjectsLock);
		ClientObjects.Add(InClientInfo.ClientId, ClientObject);
	}
	OnClientConnected.Broadcast(InClientInfo);
}

void UCefWebSocketServerBase::NotifyClientDisconnected(int64 InClientId, ECefWebSocketCloseReason InReason)
{
	if (!IsInGameThread())
	{
		TWeakObjectPtr<UCefWebSocketServerBase> weakThis(this);
		AsyncTask(ENamedThreads::GameThread, [weakThis, InClientId, InReason]()
		{
			if (weakThis.IsValid())
			{
				weakThis->NotifyClientDisconnected(InClientId, InReason);
			}
		});
		return;
	}

	{
		FScopeLock lock(&ClientObjectsLock);
		ClientObjects.Remove(InClientId);
	}
	OnClientDisconnected.Broadcast(InClientId, InReason);
}

void UCefWebSocketServerBase::NotifyServerError(ECefWebSocketErrorCode InErrorCode, const FString& InMessage)
{
	if (!IsInGameThread())
	{
		TWeakObjectPtr<UCefWebSocketServerBase> weakThis(this);
		AsyncTask(ENamedThreads::GameThread, [weakThis, InErrorCode, InMessage]()
		{
			if (weakThis.IsValid())
			{
				weakThis->NotifyServerError(InErrorCode, InMessage);
			}
		});
		return;
	}
	OnServerError.Broadcast(NameId, InErrorCode, InMessage);
}

void UCefWebSocketServerBase::NotifyClientError(int64 InClientId, ECefWebSocketErrorCode InErrorCode,
                                                const FString& InMessage)
{
	if (!IsInGameThread())
	{
		TWeakObjectPtr<UCefWebSocketServerBase> weakThis(this);
		AsyncTask(ENamedThreads::GameThread, [weakThis, InClientId, InErrorCode, InMessage]()
		{
			if (weakThis.IsValid())
			{
				weakThis->NotifyClientError(InClientId, InErrorCode, InMessage);
			}
		});
		return;
	}
	OnClientError.Broadcast(InClientId, InErrorCode, InMessage);
}

void UCefWebSocketServerBase::NotifyClientMessage(int64 InClientId, const TArray<uint8>& InPayload, bool bInBinary)
{
	UCefWebSocketClientBase* Client = nullptr;
	{
		FScopeLock lock(&ClientObjectsLock);
		if (const TObjectPtr<UCefWebSocketClientBase>* Found = ClientObjects.Find(InClientId))
		{
			Client = *Found;
		}
	}
	if (!Client)
	{
		return;
	}

	Client->HandleBytesFromClient(InPayload);
	HandleClientBytes(Client, InPayload);

	if (!bInBinary)
	{
		FUTF8ToTCHAR converter(reinterpret_cast<const ANSICHAR*>(InPayload.GetData()), InPayload.Num());
		const FString text(converter.Length(), converter.Get());
		Client->HandleStringFromClient(text);
		HandleClientString(Client, text);
	}
}

void UCefWebSocketServerBase::HandleClientBytes(UCefWebSocketClientBase* InClient, const TArray<uint8>& InData)
{
}

void UCefWebSocketServerBase::HandleClientString(UCefWebSocketClientBase* InClient, const FString& InMessage)
{
}
