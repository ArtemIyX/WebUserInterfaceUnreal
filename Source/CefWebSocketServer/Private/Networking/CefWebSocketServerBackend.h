/**
 * @file CefWebSocketServer\Private\Networking\CefWebSocketServerBackend.h
 * @brief Declares CefWebSocketServerBackend for module CefWebSocketServer\Private\Networking\CefWebSocketServerBackend.h.
 * @details Contains websocket server components used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Networking/ICefWebSocketServerBackend.h"
#include "Networking/CefWebSocketPrivate.h"

/** @brief Type declaration. */
class FCefWebSocketServerBackend : public ICefWebSocketServerBackend
{
public:
	FCefWebSocketServerBackend() = default;
	virtual ~FCefWebSocketServerBackend() override;

	virtual bool Init(uint32 InPort, FCefNetWebSocketClientConnectedCallback InOnConnected, FCefNetWebSocketClientDisconnectedCallback InOnDisconnected) override;
	virtual void Tick() override;
	virtual FString Info() const override;
	virtual uint32 GetServerPort() const override { return ServerPort; }

	/** @brief ConnectedCallback state. */
	FCefNetWebSocketClientConnectedCallback ConnectedCallback;
	/** @brief DisconnectedCallback state. */
	FCefNetWebSocketClientDisconnectedCallback DisconnectedCallback;
	/** @brief Context state. */
	CefWebSocketInternalContext* Context = nullptr;
	/** @brief Protocols state. */
	CefWebSocketInternalProtocol* Protocols = nullptr;

private:
	/** @brief ServerPort state. */
	uint32 ServerPort = 0;
};

