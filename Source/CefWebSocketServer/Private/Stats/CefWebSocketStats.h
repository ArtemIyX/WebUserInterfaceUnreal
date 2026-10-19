/**
 * @file CefWebSocketServer\Private\Stats\CefWebSocketStats.h
 * @brief Declares CefWebSocketStats for module CefWebSocketServer\Private\Stats\CefWebSocketStats.h.
 * @details Contains websocket server components used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"

DECLARE_STATS_GROUP(TEXT("CefWebSocket"), STATGROUP_CefWebSocket, STATCAT_Advanced);
DECLARE_DWORD_COUNTER_STAT(TEXT("Active Clients"), STAT_CefWs_ActiveClients, STATGROUP_CefWebSocket);
DECLARE_DWORD_COUNTER_STAT(TEXT("RX Bytes Per Tick"), STAT_CefWs_RxBytes, STATGROUP_CefWebSocket);
DECLARE_DWORD_COUNTER_STAT(TEXT("TX Bytes Per Tick"), STAT_CefWs_TxBytes, STATGROUP_CefWebSocket);
DECLARE_DWORD_COUNTER_STAT(TEXT("Dropped Messages"), STAT_CefWs_DroppedMessages, STATGROUP_CefWebSocket);
DECLARE_DWORD_COUNTER_STAT(TEXT("Queue Depth"), STAT_CefWs_QueueDepth, STATGROUP_CefWebSocket);
DECLARE_DWORD_COUNTER_STAT(TEXT("Inbound Queue Depth"), STAT_CefWs_InboundQueueDepth, STATGROUP_CefWebSocket);
DECLARE_DWORD_COUNTER_STAT(TEXT("Send Queue Depth"), STAT_CefWs_SendQueueDepth, STATGROUP_CefWebSocket);
DECLARE_DWORD_COUNTER_STAT(TEXT("Write Queue Depth"), STAT_CefWs_WriteQueueDepth, STATGROUP_CefWebSocket);
DECLARE_CYCLE_STAT(TEXT("Read Pump"), STAT_CefWs_ReadPump, STATGROUP_CefWebSocket);
DECLARE_CYCLE_STAT(TEXT("Handle Pump"), STAT_CefWs_HandlePump, STATGROUP_CefWebSocket);
DECLARE_CYCLE_STAT(TEXT("Send Pump"), STAT_CefWs_SendPump, STATGROUP_CefWebSocket);
DECLARE_CYCLE_STAT(TEXT("Write Pump"), STAT_CefWs_WritePump, STATGROUP_CefWebSocket);

