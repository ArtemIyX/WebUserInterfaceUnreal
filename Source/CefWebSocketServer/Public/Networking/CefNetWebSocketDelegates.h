#pragma once

#include "Delegates/Delegate.h"

DECLARE_DELEGATE(FCefNetWebSocketInfoCallback)
DECLARE_DELEGATE_TwoParams(FCefNetWebSocketPacketReceivedCallback, void*, int32)
DECLARE_DELEGATE_OneParam(FCefNetWebSocketClientConnectedCallback, class ICefNetWebSocket*)
DECLARE_DELEGATE_OneParam(FCefNetWebSocketClientDisconnectedCallback, class ICefNetWebSocket*)
