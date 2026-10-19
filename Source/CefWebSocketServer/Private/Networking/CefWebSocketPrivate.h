/**
 * @file CefWebSocketServer\Private\Networking\CefWebSocketPrivate.h
 * @brief Declares CefWebSocketPrivate for module CefWebSocketServer\Private\Networking\CefWebSocketPrivate.h.
 * @details Contains websocket server components used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

class FCefNetWebSocket;
class FCefWebSocketServerBackend;

typedef struct lws_context CefWebSocketInternalContext;
typedef struct lws CefWebSocketInternal;
typedef struct lws_protocols CefWebSocketInternalProtocol;

