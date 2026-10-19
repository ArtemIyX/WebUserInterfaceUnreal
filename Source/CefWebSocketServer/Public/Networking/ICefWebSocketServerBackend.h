/**
 * @file CefWebSocketServer\Public\Networking\ICefWebSocketServerBackend.h
 * @brief Declares the backend interface used to host WebSocket connections.
 */
#pragma once

#include "CoreMinimal.h"
#include "Networking/CefNetWebSocketDelegates.h"

/** @brief Platform-specific WebSocket server backend. */
class CEFWEBSOCKETSERVER_API ICefWebSocketServerBackend
{
public:
	virtual ~ICefWebSocketServerBackend() = default;

	/** @brief Initializes the backend and starts listening on a port. @param InPort Requested listening port. @param InOnConnected Callback for new clients. @param InOnDisconnected Callback for disconnected clients. @return true when initialization succeeds. */
	virtual bool Init(uint32 InPort, FCefNetWebSocketClientConnectedCallback InOnConnected, FCefNetWebSocketClientDisconnectedCallback InOnDisconnected) = 0;
	/** @brief Advances backend network processing. */
	virtual void Tick() = 0;
	/** @brief Returns backend diagnostic information. */
	virtual FString Info() const = 0;
	/** @brief Returns the actual listening port. */
	virtual uint32 GetServerPort() const = 0;
};

