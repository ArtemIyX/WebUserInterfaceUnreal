/**
 * @file CefWebSocketServer\Public\Networking\ICefNetWebSocket.h
 * @brief Declares ICefNetWebSocket for module CefWebSocketServer\Public\Networking\ICefNetWebSocket.h.
 * @details Contains interface contracts used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Networking/CefNetWebSocketDelegates.h"

class CEFWEBSOCKETSERVER_API ICefNetWebSocket
{
public:
	virtual ~ICefNetWebSocket() = default;

	virtual void SetConnectedCallBack(FCefNetWebSocketInfoCallback InCallBack) = 0;
	virtual void SetConnectionErrorCallBack(FCefNetWebSocketInfoCallback InCallBack) = 0;
	virtual void SetReceiveCallBack(FCefNetWebSocketPacketReceivedCallback InCallBack) = 0;
	virtual void SetSocketClosedCallBack(FCefNetWebSocketInfoCallback InCallBack) = 0;

	virtual bool Send(const uint8* InData, uint32 InSize, bool bInPrependSize = true) = 0;
	virtual void Tick() = 0;
	virtual void Flush() = 0;
	virtual void Close(uint16 InStatusCode = 1000, const FString& InReason = FString()) = 0;

	virtual FString RemoteEndPoint(bool bInAppendPort) = 0;
	virtual FString LocalEndPoint(bool bInAppendPort) = 0;
};

