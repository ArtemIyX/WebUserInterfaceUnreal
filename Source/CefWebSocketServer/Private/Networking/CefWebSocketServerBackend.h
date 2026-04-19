#pragma once

#include "CoreMinimal.h"
#include "Networking/ICefWebSocketServerBackend.h"
#include "Networking/CefWebSocketPrivate.h"

class FCefWebSocketServerBackend : public ICefWebSocketServerBackend
{
public:
	FCefWebSocketServerBackend() = default;
	virtual ~FCefWebSocketServerBackend() override;

	virtual bool Init(uint32 InPort, FCefNetWebSocketClientConnectedCallback InOnConnected, FCefNetWebSocketClientDisconnectedCallback InOnDisconnected) override;
	virtual void Tick() override;
	virtual FString Info() const override;
	virtual uint32 GetServerPort() const override { return ServerPort; }

	FCefNetWebSocketClientConnectedCallback ConnectedCallback;
	FCefNetWebSocketClientDisconnectedCallback DisconnectedCallback;
	CefWebSocketInternalContext* Context = nullptr;
	CefWebSocketInternalProtocol* Protocols = nullptr;

private:
	uint32 ServerPort = 0;
};
