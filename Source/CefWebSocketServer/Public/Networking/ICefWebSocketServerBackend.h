#pragma once

#include "CoreMinimal.h"
#include "Networking/CefNetWebSocketDelegates.h"

class CEFWEBSOCKETSERVER_API ICefWebSocketServerBackend
{
public:
	virtual ~ICefWebSocketServerBackend() = default;

	virtual bool Init(uint32 InPort, FCefNetWebSocketClientConnectedCallback InOnConnected, FCefNetWebSocketClientDisconnectedCallback InOnDisconnected) = 0;
	virtual void Tick() = 0;
	virtual FString Info() const = 0;
	virtual uint32 GetServerPort() const = 0;
};
