/**
 * @file CefWebSocketServer\Public\Data\CefWebSocketEnums.h
 * @brief Declares CefWebSocketEnums for module CefWebSocketServer\Public\Data\CefWebSocketEnums.h.
 * @details Contains websocket server components used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "CefWebSocketEnums.generated.h"

UENUM(BlueprintType)
/** @brief Type declaration. */
enum class ECefWebSocketCreateResult : uint8
{
	Created = 0,
	ReturnedExisting = 1,
	PortAutoAdjusted = 2,
	Failed = 3
};

UENUM(BlueprintType)
/** @brief Type declaration. */
enum class ECefWebSocketSendResult : uint8
{
	Ok = 0,
	InvalidServer = 1,
	InvalidClient = 2,
	Disconnected = 3,
	QueueFull = 4,
	TooLarge = 5,
	SerializeFailed = 6,
	InvalidUtf8 = 7,
	InternalError = 8
};

UENUM(BlueprintType)
/** @brief Type declaration. */
enum class ECefWebSocketErrorCode : uint8
{
	None = 0,
	ServerInitFailed = 1,
	PortUnavailable = 2,
	ServerNotRunning = 3,
	ClientDisconnected = 4,
	SocketSendFailed = 5,
	QueueOverflow = 6,
	InvalidPayload = 7,
	JsonSerializeFailed = 8,
	Unknown = 255
};

UENUM(BlueprintType)
/** @brief Type declaration. */
enum class ECefWebSocketCloseReason : uint8
{
	ClientClosed = 0,
	ServerShutdown = 1,
	Kicked = 2,
	Timeout = 3,
	Error = 4
};

UENUM(BlueprintType)
/** @brief Type declaration. */
enum class ECefWebSocketPayloadFormat : uint8
{
	Binary = 0,
	Utf8String = 1,
	JsonString = 2,
	XmlString = 3,
	Custom = 4
};

