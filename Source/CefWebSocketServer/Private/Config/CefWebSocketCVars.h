/**
 * @file CefWebSocketServer\Private\Config\CefWebSocketCVars.h
 * @brief Declares CefWebSocketCVars for module CefWebSocketServer\Private\Config\CefWebSocketCVars.h.
 * @details Contains websocket server components used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"

namespace CefWebSocketCVars
{
	/** @brief GetMaxMessageBytes API. */
	int32 GetMaxMessageBytes();
	/** @brief GetMaxTextMessageBytes API. */
	int32 GetMaxTextMessageBytes();
	/** @brief GetMaxOutboundMessageBytes API. */
	int32 GetMaxOutboundMessageBytes();
	/** @brief GetMaxRxBytesPerSecPerClient API. */
	int32 GetMaxRxBytesPerSecPerClient();
	/** @brief GetMaxTxBytesPerSecPerClient API. */
	int32 GetMaxTxBytesPerSecPerClient();
	/** @brief GetMaxQueueMessagesPerClient API. */
	int32 GetMaxQueueMessagesPerClient();
	/** @brief GetMaxQueueBytesPerClient API. */
	int32 GetMaxQueueBytesPerClient();
	/** @brief GetQueueDropPolicy API. */
	int32 GetQueueDropPolicy();
	/** @brief GetWriteBatchMaxMessages API. */
	int32 GetWriteBatchMaxMessages();
	/** @brief GetWriteBatchMaxBytes API. */
	int32 GetWriteBatchMaxBytes();
	/** @brief GetReadBusySleepMs API. */
	int32 GetReadBusySleepMs();
	/** @brief GetReadIdleMaxSleepMs API. */
	int32 GetReadIdleMaxSleepMs();
	/** @brief GetShutdownDrainMs API. */
	int32 GetShutdownDrainMs();
	/** @brief GetWriteIdleSleepMs API. */
	int32 GetWriteIdleSleepMs();
	/** @brief GetShutdownTimeoutMs API. */
	int32 GetShutdownTimeoutMs();
	/** @brief GetMaxPortScan API. */
	int32 GetMaxPortScan();
	/** @brief GetHeartbeatIntervalSec API. */
	float GetHeartbeatIntervalSec();
	/** @brief GetIdleTimeoutSec API. */
	float GetIdleTimeoutSec();
	/** @brief GetValidateUtf8 API. */
	bool GetValidateUtf8();
	/** @brief GetLogTraffic API. */
	bool GetLogTraffic();
}

