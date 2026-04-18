#pragma once

#include "Delegates/Delegate.h"

DECLARE_DELEGATE(FCefNetWebSocketInfoCallback)
DECLARE_DELEGATE_ThreeParams(FCefNetWebSocketPacketReceivedCallback, void*, int32, bool)
DECLARE_DELEGATE_OneParam(FCefNetWebSocketClientConnectedCallback, class ICefNetWebSocket*)
DECLARE_DELEGATE_OneParam(FCefNetWebSocketClientDisconnectedCallback, class ICefNetWebSocket*)
