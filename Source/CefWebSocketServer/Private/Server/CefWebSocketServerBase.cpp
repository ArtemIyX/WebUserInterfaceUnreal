#include "Server/CefWebSocketServerBase.h"

#include "Config/CefWebSocketCVars.h"
#include "Data/CefWebSocketStructs.h"
#include "Logging/CefWebSocketLog.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Server/CefWebSocketClientBase.h"
#include "Server/CefWebSocketGameThreadEvent.h"
#include "Server/CefWebSocketServerInstance.h"
#include "Stats/CefWebSocketStats.h"

namespace
{
constexpr int32 GameThreadControlEventReserve = 16;
}

UCefWebSocketServerBase::UCefWebSocketServerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, GameThreadEventState(MakeUnique<FCefGameThreadEventState>())
{
}

UCefWebSocketServerBase::~UCefWebSocketServerBase() = default;

void UCefWebSocketServerBase::BeginDestroy()
{
	StopServer();
	Super::BeginDestroy();
}

bool UCefWebSocketServerBase::StartServerInternal(FName InNameId, int32 InBoundPort,
	TSubclassOf<UCefWebSocketClientBase> InClientClass,
	const FCefWebSocketPipelineConfig& InPipelineConfig)
{
	check(IsInGameThread());
	check(GameThreadEventState.IsValid());

	// A new generation makes events from a previous run harmless if they outlive a restart.
	ClearGameThreadEvents();
	++GameThreadEventState->Generation;
	GameThreadEventState->DroppedEvents.Store(0);
	GameThreadEventState->DroppedMessages.Store(0);
	GameThreadEventState->bControlOverflowStopRequested.Store(false);
	GameThreadEventState->bAcceptEvents.Store(true);

	NameId = InNameId;
	BoundPort = InBoundPort;
	ClientClass = InClientClass;
	PipelineConfig = InPipelineConfig;
	PayloadFormat = PipelineConfig.InPayloadFormat;

	if (!Instance.IsValid())
	{
		Instance = MakeShared<FCefWebSocketServerInstance>(NameId, BoundPort, this);
	}
	Instance->SetPipelineConfig(PipelineConfig);
	Instance->SetPayloadFormat(PayloadFormat);
	Instance->SetPacketCodec(PacketCodec);

	const bool bStarted = Instance->Start();
	if (!bStarted)
	{
		GameThreadEventState->bAcceptEvents.Store(false);
		ClearGameThreadEvents();
	}
	return bStarted;
}

void UCefWebSocketServerBase::AttachInstance(const TSharedPtr<FCefWebSocketServerInstance>& InInstance)
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
	check(IsInGameThread());
	check(GameThreadEventState.IsValid());
	GameThreadEventState->bAcceptEvents.Store(false);
	if (Instance.IsValid())
	{
		Instance->Stop();
	}
	ClearGameThreadEvents();
	FScopeLock lock(&ClientObjectsLock);
	ClientObjects.Empty();
	PendingClientMessages.Empty();
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
	check(GameThreadEventState.IsValid());
	FCefWebSocketServerStats stats;
	if (!Instance.IsValid())
	{
		stats = FCefWebSocketServerStats();
	}
	else
	{
		stats = Instance->GetStats();
	}
	stats.GameThreadEventQueueDepth = GameThreadEventState->EventCount.Load();
	stats.DroppedGameThreadEvents = GameThreadEventState->DroppedEvents.Load();
	stats.DroppedMessages += GameThreadEventState->DroppedMessages.Load();
	SET_DWORD_STAT(STAT_CefWs_GameThreadEventQueueDepth,
	               static_cast<uint32>(FMath::Clamp<int64>(stats.GameThreadEventQueueDepth, 0, MAX_uint32)));
	SET_DWORD_STAT(STAT_CefWs_DroppedGameThreadEvents,
	               static_cast<uint32>(FMath::Clamp<int64>(stats.DroppedGameThreadEvents, 0, MAX_uint32)));
	return stats;
}

void UCefWebSocketServerBase::SetPayloadFormat(ECefWebSocketPayloadFormat InPayloadFormat)
{
	PayloadFormat = InPayloadFormat;
	if (Instance.IsValid())
	{
		Instance->SetPayloadFormat(InPayloadFormat);
	}
}

void UCefWebSocketServerBase::SetPipelineConfig(const FCefWebSocketPipelineConfig& InPipelineConfig)
{
	PipelineConfig = InPipelineConfig;
	if (Instance.IsValid())
	{
		Instance->SetPipelineConfig(InPipelineConfig);
	}
}

void UCefWebSocketServerBase::SetPacketCodec(const TSharedPtr<ICefWebSocketPacketCodec>& InCodec)
{
	PacketCodec = InCodec;
	if (Instance.IsValid())
	{
		Instance->SetPacketCodec(InCodec);
	}
}

void UCefWebSocketServerBase::NotifyClientConnected(const FCefWebSocketClientInfo& InClientInfo)
{
	FCefClientConnectedEvent payload;
	payload.ClientInfo = InClientInfo;
	if (IsInGameThread())
	{
		DispatchGameThreadEvent(FCefGameThreadEvent(GameThreadEventState->Generation.Load(), MoveTemp(payload)));
		return;
	}
	EnqueueGameThreadEvent(FCefGameThreadEvent(GameThreadEventState->Generation.Load(), MoveTemp(payload)));
}

void UCefWebSocketServerBase::NotifyClientDisconnected(int64 InClientId, ECefWebSocketCloseReason InReason)
{
	FCefClientDisconnectedEvent payload;
	payload.ClientId = InClientId;
	payload.Reason = InReason;
	if (IsInGameThread())
	{
		DispatchGameThreadEvent(FCefGameThreadEvent(GameThreadEventState->Generation.Load(), MoveTemp(payload)));
		return;
	}
	EnqueueGameThreadEvent(FCefGameThreadEvent(GameThreadEventState->Generation.Load(), MoveTemp(payload)));
}

void UCefWebSocketServerBase::NotifyServerError(ECefWebSocketErrorCode InErrorCode, const FString& InMessage)
{
	FCefServerErrorEvent payload;
	payload.ErrorCode = InErrorCode;
	payload.Message = InMessage;
	if (IsInGameThread())
	{
		DispatchGameThreadEvent(FCefGameThreadEvent(GameThreadEventState->Generation.Load(), MoveTemp(payload)));
		return;
	}
	EnqueueGameThreadEvent(FCefGameThreadEvent(GameThreadEventState->Generation.Load(), MoveTemp(payload)));
}

void UCefWebSocketServerBase::NotifyClientError(int64 InClientId, ECefWebSocketErrorCode InErrorCode,
	const FString& InMessage)
{
	FCefClientErrorEvent payload;
	payload.ClientId = InClientId;
	payload.ErrorCode = InErrorCode;
	payload.Message = InMessage;
	if (IsInGameThread())
	{
		DispatchGameThreadEvent(FCefGameThreadEvent(GameThreadEventState->Generation.Load(), MoveTemp(payload)));
		return;
	}
	EnqueueGameThreadEvent(FCefGameThreadEvent(GameThreadEventState->Generation.Load(), MoveTemp(payload)));
}

void UCefWebSocketServerBase::NotifyClientMessage(int64 InClientId, const TArray<uint8>& InPayload, bool bInBinary)
{
	FCefClientMessageEvent payload;
	payload.ClientId = InClientId;
	payload.Payload = InPayload;
	payload.bBinary = bInBinary;
	if (IsInGameThread())
	{
		DispatchGameThreadEvent(FCefGameThreadEvent(GameThreadEventState->Generation.Load(), MoveTemp(payload)));
		return;
	}
	EnqueueGameThreadEvent(FCefGameThreadEvent(GameThreadEventState->Generation.Load(), MoveTemp(payload)));
}

bool UCefWebSocketServerBase::EnqueueGameThreadEvent(FCefGameThreadEvent&& InEvent)
{
	check(!IsInGameThread());
	if (!GameThreadEventState.IsValid() || !GameThreadEventState->bAcceptEvents.Load())
	{
		return false;
	}

	const bool bMessageEvent = InEvent.Payload.IsType<FCefClientMessageEvent>();
	const int32 maxEvents = FMath::Max(1, CefWebSocketCVars::GetGameThreadEventQueueMax());
	// Keep control events in reserved capacity so lifecycle state wins over message throughput.
	const int32 controlReserve = FMath::Min(GameThreadControlEventReserve, maxEvents / 4);
	const int32 reservationLimit = bMessageEvent ? maxEvents - controlReserve : maxEvents;
	const auto dropEvent = [this, bMessageEvent]() {
		++GameThreadEventState->DroppedEvents;
		if (bMessageEvent)
		{
			++GameThreadEventState->DroppedMessages;
			return;
		}

		bool bExpectedStopRequest = false;
		if (GameThreadEventState->bControlOverflowStopRequested.CompareExchange(bExpectedStopRequest, true))
		{
			GameThreadEventState->bAcceptEvents.Store(false);
			UE_LOG(LogCefWebSocketServer, Error,
			       TEXT("Server '%s' dropped a control event because the game-thread queue is full"),
			       *NameId.ToString());
		}
	};
	int32 observedCount = GameThreadEventState->EventCount.Load();
	while (observedCount < reservationLimit)
	{
		if (!GameThreadEventState->bAcceptEvents.Load())
		{
			return false;
		}

		int32 expectedCount = observedCount;
		if (GameThreadEventState->EventCount.CompareExchange(expectedCount, observedCount + 1))
		{
			if (!GameThreadEventState->bAcceptEvents.Load())
			{
				--GameThreadEventState->EventCount;
				return false;
			}
			// Reserve before enqueue so concurrent producers cannot exceed capacity.
			if (GameThreadEventState->Events.Enqueue(MoveTemp(InEvent)))
			{
				return true;
			}
			--GameThreadEventState->EventCount;
			dropEvent();
			return false;
		}
		observedCount = expectedCount;
	}

	dropEvent();
	return false;
}

int32 UCefWebSocketServerBase::DrainGameThreadEvents(int32 InMaxEvents)
{
	check(IsInGameThread());
	if (!GameThreadEventState.IsValid() || InMaxEvents <= 0)
	{
		return 0;
	}

	int32 drainedCount = 0;
	FCefGameThreadEvent event;
	while (drainedCount < InMaxEvents && GameThreadEventState->Events.Dequeue(event))
	{
		--GameThreadEventState->EventCount;
		DispatchGameThreadEvent(MoveTemp(event));
		++drainedCount;
	}
	return drainedCount;
}

void UCefWebSocketServerBase::DispatchGameThreadEvent(FCefGameThreadEvent&& InEvent)
{
	check(IsInGameThread());
	check(GameThreadEventState.IsValid());
	if (InEvent.Generation != GameThreadEventState->Generation.Load())
	{
		return;
	}

	// Keep variant routing small; each handler owns one event's state transition and callbacks.
	if (InEvent.Payload.IsType<FCefClientConnectedEvent>())
	{
		DispatchClientConnectedEvent(InEvent.Payload.Get<FCefClientConnectedEvent>());
		return;
	}

	if (InEvent.Payload.IsType<FCefClientDisconnectedEvent>())
	{
		DispatchClientDisconnectedEvent(InEvent.Payload.Get<FCefClientDisconnectedEvent>());
		return;
	}

	if (InEvent.Payload.IsType<FCefServerErrorEvent>())
	{
		DispatchServerErrorEvent(InEvent.Payload.Get<FCefServerErrorEvent>());
		return;
	}

	if (InEvent.Payload.IsType<FCefClientErrorEvent>())
	{
		DispatchClientErrorEvent(InEvent.Payload.Get<FCefClientErrorEvent>());
		return;
	}

	DispatchClientMessageEvent(InEvent.Payload.Get<FCefClientMessageEvent>());
}

void UCefWebSocketServerBase::DispatchClientConnectedEvent(const FCefClientConnectedEvent& InEvent)
{
	check(IsInGameThread());
	const FCefWebSocketClientInfo& clientInfo = InEvent.ClientInfo;
	{
		FScopeLock lock(&ClientObjectsLock);
		PendingClientMessages.FindOrAdd(clientInfo.ClientId);
	}

	UClass* clientUClass = ClientClass ? ClientClass.Get() : UCefWebSocketClientBase::StaticClass();
	UCefWebSocketClientBase* clientObject = NewObject<UCefWebSocketClientBase>(this, clientUClass);
	if (!IsValid(clientObject))
	{
		{
			FScopeLock lock(&ClientObjectsLock);
			PendingClientMessages.Remove(clientInfo.ClientId);
		}
		NotifyClientError(clientInfo.ClientId, ECefWebSocketErrorCode::Unknown,
			TEXT("Failed to create client object"));
		return;
	}

	TArray<FCefWebSocketInboundPacket> pendingMessages;
	clientObject->InitializeClient(this, clientInfo);
	{
		FScopeLock lock(&ClientObjectsLock);
		ClientObjects.Add(clientInfo.ClientId, clientObject);
		if (TArray<FCefWebSocketInboundPacket>* foundPendingMessages = PendingClientMessages.Find(clientInfo.ClientId))
		{
			pendingMessages = MoveTemp(*foundPendingMessages);
			PendingClientMessages.Remove(clientInfo.ClientId);
		}
	}

	check(IsInGameThread());
	OnClientConnected.Broadcast(clientInfo);
	check(IsInGameThread());
	HandleClientConnected(clientInfo);

	for (const FCefWebSocketInboundPacket& pendingMessage : pendingMessages)
	{
		NotifyClientMessage(pendingMessage.ClientId, pendingMessage.Payload, pendingMessage.bBinary);
	}
}

void UCefWebSocketServerBase::DispatchClientDisconnectedEvent(const FCefClientDisconnectedEvent& InEvent)
{
	check(IsInGameThread());
	{
		FScopeLock lock(&ClientObjectsLock);
		PendingClientMessages.Remove(InEvent.ClientId);
		ClientObjects.Remove(InEvent.ClientId);
	}
	check(IsInGameThread());
	OnClientDisconnected.Broadcast(InEvent.ClientId, InEvent.Reason);
	check(IsInGameThread());
	HandleClientDisconnected(InEvent.ClientId, InEvent.Reason);
}

void UCefWebSocketServerBase::DispatchServerErrorEvent(const FCefServerErrorEvent& InEvent)
{
	check(IsInGameThread());
	OnServerError.Broadcast(NameId, InEvent.ErrorCode, InEvent.Message);
	check(IsInGameThread());
	HandleServerError(InEvent.ErrorCode, InEvent.Message);
}

void UCefWebSocketServerBase::DispatchClientErrorEvent(const FCefClientErrorEvent& InEvent)
{
	check(IsInGameThread());
	OnClientError.Broadcast(InEvent.ClientId, InEvent.ErrorCode, InEvent.Message);
	check(IsInGameThread());
	HandleClientError(InEvent.ClientId, InEvent.ErrorCode, InEvent.Message);
}

void UCefWebSocketServerBase::DispatchClientMessageEvent(const FCefClientMessageEvent& InEvent)
{
	check(IsInGameThread());
	UCefWebSocketClientBase* client = nullptr;
	{
		FScopeLock lock(&ClientObjectsLock);
		if (const TObjectPtr<UCefWebSocketClientBase>* found = ClientObjects.Find(InEvent.ClientId))
		{
			client = *found;
		}
		else if (PendingClientMessages.Contains(InEvent.ClientId))
		{
			FCefWebSocketInboundPacket& pendingMessage = PendingClientMessages.FindOrAdd(InEvent.ClientId).AddDefaulted_GetRef();
			pendingMessage.ClientId = InEvent.ClientId;
			pendingMessage.Payload = InEvent.Payload;
			pendingMessage.bBinary = InEvent.bBinary;
			return;
		}
	}
	if (!IsValid(client))
	{
		UE_LOG(LogCefWebSocketServer, Verbose,
			TEXT("Discarded message for unknown client %lld"), InEvent.ClientId);
		return;
	}

	// The generic hook intentionally precedes the legacy byte and text hooks.
	check(IsInGameThread());
	HandleClientMessage(InEvent.ClientId, InEvent.Payload, InEvent.bBinary);
	check(IsInGameThread());
	client->HandleBytesFromClient(InEvent.Payload);
	check(IsInGameThread());
	HandleClientBytes(client, InEvent.Payload);

	if (!InEvent.bBinary)
	{
		FUTF8ToTCHAR converter(reinterpret_cast<const ANSICHAR*>(InEvent.Payload.GetData()), InEvent.Payload.Num());
		const FString text(converter.Length(), converter.Get());
		check(IsInGameThread());
		client->HandleStringFromClient(text);
		check(IsInGameThread());
		HandleClientString(client, text);
	}
}

void UCefWebSocketServerBase::ClearGameThreadEvents()
{
	check(IsInGameThread());
	check(GameThreadEventState.IsValid());
	FCefGameThreadEvent event;
	while (GameThreadEventState->Events.Dequeue(event))
	{
	}
	GameThreadEventState->EventCount.Store(0);
}

int32 UCefWebSocketServerBase::GetGameThreadEventQueueDepth() const
{
	check(GameThreadEventState.IsValid());
	return FMath::Max(0, GameThreadEventState->EventCount.Load());
}

bool UCefWebSocketServerBase::ConsumeControlEventOverflowStopRequest()
{
	check(IsInGameThread());
	check(GameThreadEventState.IsValid());
	return GameThreadEventState->bControlOverflowStopRequested.Exchange(false);
}

void UCefWebSocketServerBase::HandleClientConnected(const FCefWebSocketClientInfo& InClientInfo) {}

void UCefWebSocketServerBase::HandleClientDisconnected(int64 InClientId, ECefWebSocketCloseReason InReason) {}

void UCefWebSocketServerBase::HandleServerError(ECefWebSocketErrorCode InErrorCode, const FString& InMessage) {}

void UCefWebSocketServerBase::HandleClientError(int64 InClientId, ECefWebSocketErrorCode InErrorCode, const FString& InMessage) {}

void UCefWebSocketServerBase::HandleClientMessage(int64 InClientId, const TArray<uint8>& InPayload, bool bInBinary) {}

void UCefWebSocketServerBase::HandleClientBytes(UCefWebSocketClientBase* InClient, const TArray<uint8>& InData) {}

void UCefWebSocketServerBase::HandleClientString(UCefWebSocketClientBase* InClient, const FString& InMessage) {}

void UCefWebSocketServerBase::ServerInitialized(UCefWebSocketSubsystem* InCefSubsystem)
{
	check(IsInGameThread());
	(void)InCefSubsystem;
}
