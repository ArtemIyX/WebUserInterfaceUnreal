#pragma once

#include "CoreMinimal.h"

DECLARE_STATS_GROUP(TEXT("CefWebSocket"), STATGROUP_CefWebSocket, STATCAT_Advanced);
DECLARE_DWORD_COUNTER_STAT(TEXT("Active Clients"), STAT_CefWs_ActiveClients, STATGROUP_CefWebSocket);
DECLARE_DWORD_COUNTER_STAT(TEXT("RX Bytes Per Tick"), STAT_CefWs_RxBytes, STATGROUP_CefWebSocket);
DECLARE_DWORD_COUNTER_STAT(TEXT("TX Bytes Per Tick"), STAT_CefWs_TxBytes, STATGROUP_CefWebSocket);
DECLARE_DWORD_COUNTER_STAT(TEXT("Dropped Messages"), STAT_CefWs_DroppedMessages, STATGROUP_CefWebSocket);
DECLARE_DWORD_COUNTER_STAT(TEXT("Queue Depth"), STAT_CefWs_QueueDepth, STATGROUP_CefWebSocket);
