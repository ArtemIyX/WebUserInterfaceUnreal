#pragma once

#include "CoreMinimal.h"
#include "Data/CefWebSocketEnums.h"
#include "Data/CefWebSocketStructs.h"
#include "CefWebSocketDelegates.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCefWebSocketClientConnectedDynamic, const FCefWebSocketClientInfo&, Client);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCefWebSocketClientDisconnectedDynamic, int64, ClientId, ECefWebSocketCloseReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCefWebSocketServerErrorDynamic, FName, ServerName, ECefWebSocketErrorCode, ErrorCode, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCefWebSocketClientErrorDynamic, int64, ClientId, ECefWebSocketErrorCode, ErrorCode, const FString&, Message);
