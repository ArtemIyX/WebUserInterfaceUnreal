#pragma once

#include "CoreMinimal.h"

namespace CefWebSocketCVars
{
	int32 GetMaxMessageBytes();
	int32 GetMaxTextMessageBytes();
	int32 GetMaxOutboundMessageBytes();
	int32 GetMaxRxBytesPerSecPerClient();
	int32 GetMaxTxBytesPerSecPerClient();
	int32 GetMaxQueueMessagesPerClient();
	int32 GetMaxQueueBytesPerClient();
	int32 GetQueueDropPolicy();
	int32 GetWriteBatchMaxMessages();
	int32 GetWriteBatchMaxBytes();
	int32 GetReadBusySleepMs();
	int32 GetReadIdleMaxSleepMs();
	int32 GetShutdownDrainMs();
	int32 GetWriteIdleSleepMs();
	int32 GetShutdownTimeoutMs();
	int32 GetMaxPortScan();
	float GetHeartbeatIntervalSec();
	float GetIdleTimeoutSec();
	bool GetValidateUtf8();
	bool GetLogTraffic();
}
