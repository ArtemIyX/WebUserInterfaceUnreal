#pragma once

#include "CoreMinimal.h"
#include "Networking/CefNetWebSocketDelegates.h"

class CEFWEBSOCKETSERVER_API ICefNetWebSocket
{
public:
	virtual ~ICefNetWebSocket() = default;

	virtual void SetConnectedCallBack(FCefNetWebSocketInfoCallback CallBack) = 0;
	virtual void SetConnectionErrorCallBack(FCefNetWebSocketInfoCallback CallBack) = 0;
	virtual void SetReceiveCallBack(FCefNetWebSocketPacketReceivedCallback CallBack) = 0;
	virtual void SetSocketClosedCallBack(FCefNetWebSocketInfoCallback CallBack) = 0;

	virtual bool Send(const uint8* Data, uint32 Size, bool bPrependSize = true) = 0;
	virtual void Tick() = 0;
	virtual void Flush() = 0;
	virtual void Close() = 0;

	virtual FString RemoteEndPoint(bool bAppendPort) = 0;
	virtual FString LocalEndPoint(bool bAppendPort) = 0;
};
