#pragma once

#include "Networking/ICefNetWebSocket.h"
#include "Networking/CefWebSocketPrivate.h"

class FCefNetWebSocket : public ICefNetWebSocket
{
public:
	FCefNetWebSocket(CefWebSocketInternalContext* InContext, CefWebSocketInternal* InWsi);
	virtual ~FCefNetWebSocket() override;

	virtual void SetConnectedCallBack(FCefNetWebSocketInfoCallback InCallBack) override;
	virtual void SetConnectionErrorCallBack(FCefNetWebSocketInfoCallback InCallBack) override;
	virtual void SetReceiveCallBack(FCefNetWebSocketPacketReceivedCallback InCallBack) override;
	virtual void SetSocketClosedCallBack(FCefNetWebSocketInfoCallback InCallBack) override;

	virtual bool Send(const uint8* InData, uint32 InSize, bool bInPrependSize = true) override;
	virtual void Tick() override;
	virtual void Flush() override;
	virtual void Close() override;

	virtual FString RemoteEndPoint(bool bInAppendPort) override;
	virtual FString LocalEndPoint(bool bInAppendPort) override;

	void OnReceive(void* InData, uint32 InSize, bool bInIsBinary);
	void OnRawWebSocketWritable(CefWebSocketInternal* InWsi);
	void OnClose();

private:
	void FlushInternal();

public:
	FCefNetWebSocketPacketReceivedCallback ReceivedCallback;
	FCefNetWebSocketInfoCallback ConnectedCallBack;
	FCefNetWebSocketInfoCallback ErrorCallBack;
	FCefNetWebSocketInfoCallback SocketClosedCallback;

	TArray<TArray<uint8>> OutgoingBuffer;
	FCriticalSection OutgoingLock;
	CefWebSocketInternalContext* Context = nullptr;
	CefWebSocketInternal* Wsi = nullptr;
	FString RemoteIp;
	int32 RemotePort = 0;
};
