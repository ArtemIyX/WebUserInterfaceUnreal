#pragma once

#include "CoreMinimal.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

class FCefNetWebSocket;
class FCefWebSocketServerBackend;

typedef struct lws_context CefWebSocketInternalContext;
typedef struct lws CefWebSocketInternal;
typedef struct lws_protocols CefWebSocketInternalProtocol;
