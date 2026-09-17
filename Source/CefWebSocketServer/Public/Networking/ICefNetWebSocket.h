/**
 * @file CefWebSocketServer\Public\Networking\ICefNetWebSocket.h
 * @brief Declares the low-level WebSocket connection interface used by the server backend.
 */
#pragma once

#include "CoreMinimal.h"
#include "Networking/CefNetWebSocketDelegates.h"

/** @brief Represents one active network WebSocket connection. */
class CEFWEBSOCKETSERVER_API ICefNetWebSocket
{
public:
	virtual ~ICefNetWebSocket() = default;

	/** @brief Sets the callback invoked after the connection is established. */
	virtual void SetConnectedCallBack(FCefNetWebSocketInfoCallback InCallBack) = 0;
	/** @brief Sets the callback invoked when a connection error occurs. */
	virtual void SetConnectionErrorCallBack(FCefNetWebSocketInfoCallback InCallBack) = 0;
	/** @brief Sets the callback invoked when a packet is received. */
	virtual void SetReceiveCallBack(FCefNetWebSocketPacketReceivedCallback InCallBack) = 0;
	/** @brief Sets the callback invoked after the connection closes. */
	virtual void SetSocketClosedCallBack(FCefNetWebSocketInfoCallback InCallBack) = 0;

	/** @brief Queues data for transmission. @param InData Data buffer. @param InSize Number of bytes in InData. @param bInPrependSize Whether to prepend the payload size. @return true when the data is accepted. */
	virtual bool Send(const uint8* InData, uint32 InSize, bool bInPrependSize = true) = 0;
	/** @brief Advances the connection's network processing. */
	virtual void Tick() = 0;
	/** @brief Flushes pending outgoing data. */
	virtual void Flush() = 0;
	/** @brief Closes the connection. @param InStatusCode WebSocket close status code. @param InReason Human-readable close reason. */
	virtual void Close(uint16 InStatusCode = 1000, const FString& InReason = FString()) = 0;

	/** @brief Returns the remote endpoint address. @param bInAppendPort Whether to append the port. */
	virtual FString RemoteEndPoint(bool bInAppendPort) = 0;
	/** @brief Returns the local endpoint address. @param bInAppendPort Whether to append the port. */
	virtual FString LocalEndPoint(bool bInAppendPort) = 0;
};

