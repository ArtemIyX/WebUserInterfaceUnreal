#pragma once

#include "CoreMinimal.h"

namespace CefWebSocketCVars
{
	int32 GetMaxMessageBytes();
	int32 GetMaxQueueMessagesPerClient();
	int32 GetMaxQueueBytesPerClient();
	int32 GetWriteIdleSleepMs();
	int32 GetShutdownTimeoutMs();
	int32 GetMaxPortScan();
	bool GetValidateUtf8();
	bool GetLogTraffic();
}
