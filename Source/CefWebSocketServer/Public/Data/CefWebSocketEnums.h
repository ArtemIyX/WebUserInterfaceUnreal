/**
 * @file CefWebSocketServer\Public\Data\CefWebSocketEnums.h
 * @brief Defines enumerations used by the Cef WebSocket server.
 */
#pragma once

#include "CoreMinimal.h"
#include "CefWebSocketEnums.generated.h"

/** @brief Result of creating or reusing a Cef WebSocket server. */
UENUM(BlueprintType)
enum class ECefWebSocketCreateResult : uint8
{
	/** A new server was created. */
	Created = 0,
	/** An existing server instance was returned. */
	ReturnedExisting = 1,
	/** The requested port was unavailable and another port was selected. */
	PortAutoAdjusted = 2,
	/** Server creation failed. */
	Failed = 3
};

/** @brief Result of sending data through a Cef WebSocket server. */
UENUM(BlueprintType)
enum class ECefWebSocketSendResult : uint8
{
	/** Data was accepted for sending. */
	Ok = 0,
	/** The target server is invalid. */
	InvalidServer = 1,
	/** The target client is invalid. */
	InvalidClient = 2,
	/** The target client is disconnected. */
	Disconnected = 3,
	/** The send queue has no capacity. */
	QueueFull = 4,
	/** The payload exceeds the supported size limit. */
	TooLarge = 5,
	/** Payload serialization failed. */
	SerializeFailed = 6,
	/** The payload contains invalid UTF-8 data. */
	InvalidUtf8 = 7,
	/** An unspecified internal error occurred. */
	InternalError = 8
};

/** @brief Error categories reported by the Cef WebSocket server. */
UENUM(BlueprintType)
enum class ECefWebSocketErrorCode : uint8
{
	/** No error occurred. */
	None = 0,
	/** Server initialization failed. */
	ServerInitFailed = 1,
	/** No usable port was available. */
	PortUnavailable = 2,
	/** The server is not running. */
	ServerNotRunning = 3,
	/** The client disconnected before the operation completed. */
	ClientDisconnected = 4,
	/** The socket rejected a send operation. */
	SocketSendFailed = 5,
	/** A queue exceeded its configured capacity. */
	QueueOverflow = 6,
	/** The received or transmitted payload is invalid. */
	InvalidPayload = 7,
	/** JSON serialization failed. */
	JsonSerializeFailed = 8,
	/** The error does not match a known category. */
	Unknown = 255
};

/** @brief Reason a Cef WebSocket connection was closed. */
UENUM(BlueprintType)
enum class ECefWebSocketCloseReason : uint8
{
	/** The client closed the connection. */
	ClientClosed = 0,
	/** The server shut down. */
	ServerShutdown = 1,
	/** The server forcibly closed the connection. */
	Kicked = 2,
	/** The connection timed out. */
	Timeout = 3,
	/** The connection closed because of an error. */
	Error = 4
};

/** @brief Payload encoding used by the Cef WebSocket pipeline. */
UENUM(BlueprintType)
enum class ECefWebSocketPayloadFormat : uint8
{
	/** Arbitrary binary data. */
	Binary = 0,
	/** A UTF-8 encoded string. */
	Utf8String = 1,
	/** A UTF-8 encoded JSON document. */
	JsonString = 2,
	/** A UTF-8 encoded XML document. */
	XmlString = 3,
	/** A format defined by the consuming system. */
	Custom = 4
};

