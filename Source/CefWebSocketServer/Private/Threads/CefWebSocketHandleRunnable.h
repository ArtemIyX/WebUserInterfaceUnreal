/**
 * @file CefWebSocketServer\Private\Threads\CefWebSocketHandleRunnable.h
 * @brief Declares CefWebSocketHandleRunnable for module CefWebSocketServer\Private\Threads\CefWebSocketHandleRunnable.h.
 * @details Contains websocket server components used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "HAL/Runnable.h"

class FCefWebSocketServerInstance;
class FEvent;

/** @brief Type declaration. */
class FCefWebSocketHandleRunnable : public FRunnable
{
public:
#pragma region Lifecycle
	/** @brief FCefWebSocketHandleRunnable API. */
	FCefWebSocketHandleRunnable(FCefWebSocketServerInstance* InOwner, FEvent* InWakeEvent);
	virtual ~FCefWebSocketHandleRunnable() override = default;
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

