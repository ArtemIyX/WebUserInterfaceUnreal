/**
 * @file CefWebSocketServer\Public\Networking\ICefNetWebSocket.h
 * @brief Declares ICefNetWebSocket for module CefWebSocketServer\Public\Networking\ICefNetWebSocket.h.
 * @details Contains interface contracts used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Networking/CefNetWebSocketDelegates.h"

/** @brief Type declaration. */
class CEFWEBSOCKETSERVER_API ICefNetWebSocket
{
public:
	virtual ~ICefNetWebSocket() = default;

	/** @brief SetConnectedCallBack API. */
	virtual void SetConnectedCallBack(FCefNetWebSocketInfoCallback InCallBack) = 0;
	/** @brief SetConnectionErrorCallBack API. */
	virtual void SetConnectionErrorCallBack(FCefNetWebSocketInfoCallback InCallBack) = 0;
	/** @brief SetReceiveCallBack API. */
	virtual void SetReceiveCallBack(FCefNetWebSocketPacketReceivedCallback InCallBack) = 0;
	/** @brief SetSocketClosedCallBack API. */
	virtual void SetSocketClosedCallBack(FCefNetWebSocketInfoCallback InCallBack) = 0;

	/** @brief Send API. */
	virtual bool Send(const uint8* InData, uint32 InSize, bool bInPrependSize = true) = 0;
	/** @brief Tick API. */
	virtual void Tick() = 0;
	/** @brief Flush API. */
	virtual void Flush() = 0;
	/** @brief Close API. */
	virtual void Close(uint16 InStatusCode = 1000, const FString& InReason = FString()) = 0;

	/** @brief RemoteEndPoint API. */
	virtual FString RemoteEndPoint(bool bInAppendPort) = 0;
	/** @brief LocalEndPoint API. */
	virtual FString LocalEndPoint(bool bInAppendPort) = 0;
};

