/**
 * @file CefWebSocketServer\\Private\\Server\\CefWebSocketGameThreadEvent.h
 * @brief Defines owned event payloads transferred to the WebSocket game-thread dispatcher.
 */
#pragma once

#include "CoreMinimal.h"
#include "Data/CefWebSocketEnums.h"
#include "Data/CefWebSocketStructs.h"
#include "Misc/TVariant.h"

/** @brief Payload for a newly connected client. */
struct FCefClientConnectedEvent
{
	FCefWebSocketClientInfo ClientInfo;
};

/** @brief Payload for a disconnected client. */
struct FCefClientDisconnectedEvent
{
	int64 ClientId = 0;
	ECefWebSocketCloseReason Reason = ECefWebSocketCloseReason::ClientClosed;
};

/** @brief Payload for a server-level error. */
struct FCefServerErrorEvent
{
	ECefWebSocketErrorCode ErrorCode = ECefWebSocketErrorCode::None;
	FString Message;
};

/** @brief Payload for a client-level error. */
struct FCefClientErrorEvent
{
	int64 ClientId = 0;
	ECefWebSocketErrorCode ErrorCode = ECefWebSocketErrorCode::None;
	FString Message;
};

/** @brief Payload for bytes received from a client. */
struct FCefClientMessageEvent
{
	int64 ClientId = 0;
	TArray<uint8> Payload;
	bool bBinary = false;
};

/** @brief Variant containing one owned WebSocket game-thread event payload. */
using FCefGameThreadEventPayload = TVariant<FCefClientConnectedEvent, FCefClientDisconnectedEvent,
	FCefServerErrorEvent, FCefClientErrorEvent, FCefClientMessageEvent>;

/** @brief A server-generation-stamped event queued for game-thread dispatch. */
struct FCefGameThreadEvent
{
	uint64 Generation = 0;
	FCefGameThreadEventPayload Payload;
	FCefGameThreadEvent() = default;

	FCefGameThreadEvent(uint64 InGeneration, FCefClientConnectedEvent&& InPayload)
		: Generation(InGeneration)
	{
		Payload.Set<FCefClientConnectedEvent>(MoveTemp(InPayload));
	}

	FCefGameThreadEvent(uint64 InGeneration, FCefClientDisconnectedEvent&& InPayload)
		: Generation(InGeneration)
	{
		Payload.Set<FCefClientDisconnectedEvent>(MoveTemp(InPayload));
	}

	FCefGameThreadEvent(uint64 InGeneration, FCefServerErrorEvent&& InPayload)
		: Generation(InGeneration)
	{
		Payload.Set<FCefServerErrorEvent>(MoveTemp(InPayload));
	}

	FCefGameThreadEvent(uint64 InGeneration, FCefClientErrorEvent&& InPayload)
		: Generation(InGeneration)
	{
		Payload.Set<FCefClientErrorEvent>(MoveTemp(InPayload));
	}

	FCefGameThreadEvent(uint64 InGeneration, FCefClientMessageEvent&& InPayload)
		: Generation(InGeneration)
	{
		Payload.Set<FCefClientMessageEvent>(MoveTemp(InPayload));
	}
};

/** @brief Owns the cross-thread event queue and its atomic lifecycle counters. */
class FCefGameThreadEventState
{
public:
	/** @brief MPSC queue written by runtime threads and read by the game thread. */
	TQueue<FCefGameThreadEvent, EQueueMode::Mpsc> Events;
	/** @brief Queue depth including an in-flight producer reservation. */
	TAtomic<int32> EventCount{0};
	/** @brief Producer gate closed during stop and failed startup. */
	TAtomic<bool> bAcceptEvents{false};
	/** @brief Monotonic server-run identifier. */
	TAtomic<uint64> Generation{0};
	/** @brief Total rejected game-thread events. */
	TAtomic<int64> DroppedEvents{0};
	/** @brief Rejected message events included in DroppedMessages. */
	TAtomic<int64> DroppedMessages{0};
	/** @brief Requests game-thread shutdown after a control event overflow. */
	TAtomic<bool> bControlOverflowStopRequested{false};
};
