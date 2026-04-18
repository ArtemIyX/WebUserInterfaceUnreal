#include "Config/CefWebSocketCVars.h"

#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarCefWsMaxMessageBytes(TEXT("cefws.max_message_bytes"), 1024 * 1024, TEXT("Maximum accepted websocket message size in bytes."));
	TAutoConsoleVariable<int32> CVarCefWsMaxQueueMessagesPerClient(TEXT("cefws.max_queue_messages_per_client"), 1024, TEXT("Maximum queued outbound messages per client."));
	TAutoConsoleVariable<int32> CVarCefWsMaxQueueBytesPerClient(TEXT("cefws.max_queue_bytes_per_client"), 8 * 1024 * 1024, TEXT("Maximum queued outbound bytes per client."));
	TAutoConsoleVariable<int32> CVarCefWsWriteIdleSleepMs(TEXT("cefws.write_idle_sleep_ms"), 2, TEXT("Write thread idle wait in milliseconds."));
	TAutoConsoleVariable<int32> CVarCefWsShutdownTimeoutMs(TEXT("cefws.shutdown_timeout_ms"), 1000, TEXT("Server shutdown grace timeout in milliseconds."));
	TAutoConsoleVariable<int32> CVarCefWsMaxPortScan(TEXT("cefws.max_port_scan"), 32, TEXT("Maximum ports to probe for auto-next-port behavior."));
	TAutoConsoleVariable<int32> CVarCefWsValidateUtf8(TEXT("cefws.validate_utf8"), 1, TEXT("Validate UTF-8 payloads when processing text frames."));
	TAutoConsoleVariable<int32> CVarCefWsLogTraffic(TEXT("cefws.log_traffic"), 0, TEXT("Enable traffic logging for websocket payload flow."));
}

namespace CefWebSocketCVars
{
	int32 GetMaxMessageBytes() { return CVarCefWsMaxMessageBytes.GetValueOnAnyThread(); }
	int32 GetMaxQueueMessagesPerClient() { return CVarCefWsMaxQueueMessagesPerClient.GetValueOnAnyThread(); }
	int32 GetMaxQueueBytesPerClient() { return CVarCefWsMaxQueueBytesPerClient.GetValueOnAnyThread(); }
	int32 GetWriteIdleSleepMs() { return CVarCefWsWriteIdleSleepMs.GetValueOnAnyThread(); }
	int32 GetShutdownTimeoutMs() { return CVarCefWsShutdownTimeoutMs.GetValueOnAnyThread(); }
	int32 GetMaxPortScan() { return CVarCefWsMaxPortScan.GetValueOnAnyThread(); }
	bool GetValidateUtf8() { return CVarCefWsValidateUtf8.GetValueOnAnyThread() != 0; }
	bool GetLogTraffic() { return CVarCefWsLogTraffic.GetValueOnAnyThread() != 0; }
}
