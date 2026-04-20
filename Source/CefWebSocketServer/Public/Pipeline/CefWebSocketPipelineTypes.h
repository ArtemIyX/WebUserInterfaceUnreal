/**
 * @file CefWebSocketServer\Public\Pipeline\CefWebSocketPipelineTypes.h
 * @brief Declares CefWebSocketPipelineTypes for module CefWebSocketServer\Public\Pipeline\CefWebSocketPipelineTypes.h.
 * @details Contains websocket server components used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Data/CefWebSocketEnums.h"

/** @brief Type declaration. */
struct CEFWEBSOCKETSERVER_API FCefWebSocketInboundPacket
{
	/** @brief ClientId state. */
	int64 ClientId = 0;
	/** @brief Payload state. */
	TArray<uint8> Payload;
	/** @brief bBinary state. */
	bool bBinary = true;
	/** @brief FDateTime::UtcNow API. */
	FDateTime ReceivedAtUtc = FDateTime::UtcNow();
};

/** @brief Type declaration. */
struct CEFWEBSOCKETSERVER_API FCefWebSocketSendRequest
{
	/** @brief TargetClientIds state. */
	TArray<int64> TargetClientIds;
	/** @brief bBroadcast state. */
	bool bBroadcast = false;
	/** @brief ExcludedClientId state. */
	int64 ExcludedClientId = 0;
	/** @brief PayloadFormat state. */
	ECefWebSocketPayloadFormat PayloadFormat = ECefWebSocketPayloadFormat::Binary;
	/** @brief BytesPayload state. */
	TArray<uint8> BytesPayload;
	/** @brief TextPayload state. */
	FString TextPayload;
};

/** @brief Type declaration. */
struct CEFWEBSOCKETSERVER_API FCefWebSocketWritePacket
{
	/** @brief ClientId state. */
	int64 ClientId = 0;
	/** @brief Payload state. */
	TArray<uint8> Payload;
	/** @brief bBinary state. */
	bool bBinary = true;
};

