#include "Config/CefWebSocketCVars.h"

#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarCefWsMaxMessageBytes(TEXT("cefws.max_message_bytes"), 1024 * 1024, TEXT("Maximum accepted websocket message size in bytes."));
	TAutoConsoleVariable<int32> CVarCefWsMaxTextMessageBytes(TEXT("cefws.max_text_message_bytes"), 512 * 1024, TEXT("Maximum accepted text websocket message size in bytes."));
	TAutoConsoleVariable<int32> CVarCefWsMaxOutboundMessageBytes(TEXT("cefws.max_outbound_message_bytes"), 1024 * 1024, TEXT("Maximum outbound websocket payload size in bytes."));
	TAutoConsoleVariable<int32> CVarCefWsMaxQueueMessagesPerClient(TEXT("cefws.max_queue_messages_per_client"), 1024, TEXT("Maximum queued outbound messages per client."));
	TAutoConsoleVariable<int32> CVarCefWsMaxQueueBytesPerClient(TEXT("cefws.max_queue_bytes_per_client"), 8 * 1024 * 1024, TEXT("Maximum queued outbound bytes per client."));
	TAutoConsoleVariable<int32> CVarCefWsQueueDropPolicy(TEXT("cefws.queue_drop_policy"), 0, TEXT("Queue overflow policy: 0=drop_oldest, 1=reject_new."));
	TAutoConsoleVariable<int32> CVarCefWsWriteBatchMaxMessages(TEXT("cefws.write_batch_max_messages"), 16, TEXT("Maximum messages drained per client on one write pass."));
	TAutoConsoleVariable<int32> CVarCefWsWriteBatchMaxBytes(TEXT("cefws.write_batch_max_bytes"), 128 * 1024, TEXT("Maximum bytes drained per client on one write pass."));
	TAutoConsoleVariable<int32> CVarCefWsReadBusySleepMs(TEXT("cefws.read_busy_sleep_ms"), 1, TEXT("Read thread sleep in milliseconds when activity is detected."));
	TAutoConsoleVariable<int32> CVarCefWsReadIdleMaxSleepMs(TEXT("cefws.read_idle_max_sleep_ms"), 16, TEXT("Read thread max backoff sleep in milliseconds during idle periods."));
	TAutoConsoleVariable<int32> CVarCefWsWriteIdleSleepMs(TEXT("cefws.write_idle_sleep_ms"), 2, TEXT("Write thread idle wait in milliseconds."));
	TAutoConsoleVariable<int32> CVarCefWsShutdownTimeoutMs(TEXT("cefws.shutdown_timeout_ms"), 1000, TEXT("Server shutdown grace timeout in milliseconds."));
	TAutoConsoleVariable<int32> CVarCefWsMaxPortScan(TEXT("cefws.max_port_scan"), 32, TEXT("Maximum ports to probe for auto-next-port behavior."));
	TAutoConsoleVariable<float> CVarCefWsHeartbeatIntervalSec(TEXT("cefws.heartbeat_interval_sec"), 15.0f, TEXT("Heartbeat send interval in seconds. <=0 disables heartbeat."));
	TAutoConsoleVariable<float> CVarCefWsIdleTimeoutSec(TEXT("cefws.idle_timeout_sec"), 60.0f, TEXT("Client idle timeout in seconds. <=0 disables timeout."));
	TAutoConsoleVariable<int32> CVarCefWsValidateUtf8(TEXT("cefws.validate_utf8"), 1, TEXT("Validate UTF-8 payloads when processing text frames."));
	TAutoConsoleVariable<int32> CVarCefWsLogTraffic(TEXT("cefws.log_traffic"), 0, TEXT("Enable traffic logging for websocket payload flow."));
}

namespace CefWebSocketCVars
{
	int32 GetMaxMessageBytes() { return CVarCefWsMaxMessageBytes.GetValueOnAnyThread(); }
	int32 GetMaxTextMessageBytes() { return CVarCefWsMaxTextMessageBytes.GetValueOnAnyThread(); }
	int32 GetMaxOutboundMessageBytes() { return CVarCefWsMaxOutboundMessageBytes.GetValueOnAnyThread(); }
	int32 GetMaxQueueMessagesPerClient() { return CVarCefWsMaxQueueMessagesPerClient.GetValueOnAnyThread(); }
	int32 GetMaxQueueBytesPerClient() { return CVarCefWsMaxQueueBytesPerClient.GetValueOnAnyThread(); }
	int32 GetQueueDropPolicy() { return CVarCefWsQueueDropPolicy.GetValueOnAnyThread(); }
	int32 GetWriteBatchMaxMessages() { return CVarCefWsWriteBatchMaxMessages.GetValueOnAnyThread(); }
	int32 GetWriteBatchMaxBytes() { return CVarCefWsWriteBatchMaxBytes.GetValueOnAnyThread(); }
	int32 GetReadBusySleepMs() { return CVarCefWsReadBusySleepMs.GetValueOnAnyThread(); }
	int32 GetReadIdleMaxSleepMs() { return CVarCefWsReadIdleMaxSleepMs.GetValueOnAnyThread(); }
	int32 GetWriteIdleSleepMs() { return CVarCefWsWriteIdleSleepMs.GetValueOnAnyThread(); }
	int32 GetShutdownTimeoutMs() { return CVarCefWsShutdownTimeoutMs.GetValueOnAnyThread(); }
	int32 GetMaxPortScan() { return CVarCefWsMaxPortScan.GetValueOnAnyThread(); }
	float GetHeartbeatIntervalSec() { return CVarCefWsHeartbeatIntervalSec.GetValueOnAnyThread(); }
	float GetIdleTimeoutSec() { return CVarCefWsIdleTimeoutSec.GetValueOnAnyThread(); }
	bool GetValidateUtf8() { return CVarCefWsValidateUtf8.GetValueOnAnyThread() != 0; }
	bool GetLogTraffic() { return CVarCefWsLogTraffic.GetValueOnAnyThread() != 0; }
}
