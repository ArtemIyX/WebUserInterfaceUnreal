#pragma once

#include "CoreMinimal.h"
#include "Data/CefWebSocketEnums.h"
#include "Data/CefWebSocketStructs.h"
#include "CefWebSocketDelegates.generated.h"

UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCefWebSocketClientConnectedDynamic, const FCefWebSocketClientInfo&, Client);

UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCefWebSocketClientDisconnectedDynamic, int64, ClientId, ECefWebSocketCloseReason, Reason);

UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCefWebSocketServerErrorDynamic, FName, ServerName, ECefWebSocketErrorCode, ErrorCode, const FString&, Message);

UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCefWebSocketClientErrorDynamic, int64, ClientId, ECefWebSocketErrorCode, ErrorCode, const FString&, Message);
