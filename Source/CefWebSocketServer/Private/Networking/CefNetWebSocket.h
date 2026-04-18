#pragma once

#include "Networking/ICefNetWebSocket.h"
#include "Networking/CefWebSocketPrivate.h"

class FCefNetWebSocket : public ICefNetWebSocket
{
public:
	FCefNetWebSocket(CefWebSocketInternalContext* InContext, CefWebSocketInternal* InWsi);
	virtual ~FCefNetWebSocket() override;

	virtual void SetConnectedCallBack(FCefNetWebSocketInfoCallback CallBack) override;
	virtual void SetConnectionErrorCallBack(FCefNetWebSocketInfoCallback CallBack) override;
	virtual void SetReceiveCallBack(FCefNetWebSocketPacketReceivedCallback CallBack) override;
	virtual void SetSocketClosedCallBack(FCefNetWebSocketInfoCallback CallBack) override;

	virtual bool Send(const uint8* Data, uint32 Size, bool bPrependSize = true) override;
	virtual void Tick() override;
	virtual void Flush() override;
	virtual void Close() override;

	virtual FString RemoteEndPoint(bool bAppendPort) override;
	virtual FString LocalEndPoint(bool bAppendPort) override;

	void OnReceive(void* Data, uint32 Size, bool bIsBinary);
	void OnRawWebSocketWritable(CefWebSocketInternal* Wsi);
	void OnClose();

	FCefNetWebSocketPacketReceivedCallback ReceivedCallback;
	FCefNetWebSocketInfoCallback ConnectedCallBack;
	FCefNetWebSocketInfoCallback ErrorCallBack;
	FCefNetWebSocketInfoCallback SocketClosedCallback;

	TArray<TArray<uint8>> OutgoingBuffer;
	FCriticalSection OutgoingLock;
	CefWebSocketInternalContext* Context = nullptr;
	CefWebSocketInternal* Wsi = nullptr;
	struct sockaddr_in RemoteAddr;
};
