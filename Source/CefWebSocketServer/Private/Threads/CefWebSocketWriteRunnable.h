/**
 * @file CefWebSocketServer\Private\Threads\CefWebSocketWriteRunnable.h
 * @brief Declares CefWebSocketWriteRunnable for module CefWebSocketServer\Private\Threads\CefWebSocketWriteRunnable.h.
 * @details Contains IPC reader/writer primitives used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "HAL/Runnable.h"

class FCefWebSocketServerInstance;
class FEvent;

class FCefWebSocketWriteRunnable : public FRunnable
{
public:
#pragma region Lifecycle
	FCefWebSocketWriteRunnable(FCefWebSocketServerInstance* InOwner, FEvent* InWakeEvent);
	virtual ~FCefWebSocketWriteRunnable() override = default;
#pragma endregion

#pragma region FRunnable
	virtual uint32 Run() override;
	virtual void Stop() override;
#pragma endregion

private:
#pragma region InternalState
	FCefWebSocketServerInstance* Owner = nullptr;
	FEvent* WakeEvent = nullptr;
	TAtomic<bool> bStopRequested = false;
#pragma endregion
};

