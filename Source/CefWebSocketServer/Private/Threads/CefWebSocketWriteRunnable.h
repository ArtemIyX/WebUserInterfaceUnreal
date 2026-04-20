/**
 * @file CefWebSocketServer\Private\Threads\CefWebSocketWriteRunnable.h
 * @brief Declares CefWebSocketWriteRunnable for module CefWebSocketServer\Private\Threads\CefWebSocketWriteRunnable.h.
 * @details Contains IPC reader/writer primitives used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "HAL/Runnable.h"

class FCefWebSocketServerInstance;
class FEvent;

/** @brief Type declaration. */
class FCefWebSocketWriteRunnable : public FRunnable
{
public:
#pragma region Lifecycle
	/** @brief FCefWebSocketWriteRunnable API. */
	FCefWebSocketWriteRunnable(FCefWebSocketServerInstance* InOwner, FEvent* InWakeEvent);
	virtual ~FCefWebSocketWriteRunnable() override = default;
#pragma endregion

#pragma region FRunnable
	virtual uint32 Run() override;
	virtual void Stop() override;
#pragma endregion

private:
#pragma region InternalState
	/** @brief Owner state. */
	FCefWebSocketServerInstance* Owner = nullptr;
	/** @brief WakeEvent state. */
	FEvent* WakeEvent = nullptr;
	/** @brief bStopRequested state. */
	TAtomic<bool> bStopRequested = false;
#pragma endregion
};

