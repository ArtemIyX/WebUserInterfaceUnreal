/**
 * @file CefWebSocketServer\Private\Networking\CefNetWebSocket.h
 * @brief Declares CefNetWebSocket for module CefWebSocketServer\Private\Networking\CefNetWebSocket.h.
 * @details Contains websocket server components used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "Networking/ICefNetWebSocket.h"
#include "Networking/CefWebSocketPrivate.h"

/** @brief Type declaration. */
class FCefNetWebSocket : public ICefNetWebSocket
{
public:
	/** @brief FCefNetWebSocket API. */
	FCefNetWebSocket(CefWebSocketInternalContext* InContext, CefWebSocketInternal* InWsi);
	virtual ~FCefNetWebSocket() override;

	virtual void SetConnectedCallBack(FCefNetWebSocketInfoCallback InCallBack) override;
	virtual void SetConnectionErrorCallBack(FCefNetWebSocketInfoCallback InCallBack) override;
	virtual void SetReceiveCallBack(FCefNetWebSocketPacketReceivedCallback InCallBack) override;
	virtual void SetSocketClosedCallBack(FCefNetWebSocketInfoCallback InCallBack) override;

	virtual bool Send(const uint8* InData, uint32 InSize, bool bInPrependSize = true) override;
	virtual void Tick() override;
	virtual void Flush() override;
	virtual void Close(uint16 InStatusCode = 1000, const FString& InReason = FString()) override;

	virtual FString RemoteEndPoint(bool bInAppendPort) override;
	virtual FString LocalEndPoint(bool bInAppendPort) override;

	/** @brief OnReceive API. */
	void OnReceive(void* InData, uint32 InSize, bool bInIsBinary);
	/** @brief OnRawWebSocketWritable API. */
	void OnRawWebSocketWritable(CefWebSocketInternal* InWsi);
	/** @brief OnClose API. */
	void OnClose();

private:
	/** @brief FlushInternal API. */
	void FlushInternal();

public:
	/** @brief ReceivedCallback state. */
	FCefNetWebSocketPacketReceivedCallback ReceivedCallback;
	/** @brief ConnectedCallBack state. */
	FCefNetWebSocketInfoCallback ConnectedCallBack;
	/** @brief ErrorCallBack state. */
	FCefNetWebSocketInfoCallback ErrorCallBack;
	/** @brief SocketClosedCallback state. */
	FCefNetWebSocketInfoCallback SocketClosedCallback;

	/** @brief OutgoingBuffer state. */
	TArray<TArray<uint8>> OutgoingBuffer;
	/** @brief OutgoingLock state. */
	FCriticalSection OutgoingLock;
	/** @brief Context state. */
	CefWebSocketInternalContext* Context = nullptr;
	/** @brief Wsi state. */
	CefWebSocketInternal* Wsi = nullptr;
	/** @brief RemoteIp state. */
	FString RemoteIp;
	/** @brief RemotePort state. */
	int32 RemotePort = 0;
};

