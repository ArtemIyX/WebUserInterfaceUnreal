/**
 * @file CefWebSocketServer\Private\Threads\CefWebSocketSendRunnable.h
 * @brief Declares CefWebSocketSendRunnable for module CefWebSocketServer\Private\Threads\CefWebSocketSendRunnable.h.
 * @details Contains websocket server components used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "HAL/Runnable.h"

class FCefWebSocketServerInstance;
class FEvent;

/** @brief Type declaration. */
class FCefWebSocketSendRunnable : public FRunnable
{
public:
#pragma region Lifecycle
	/** @brief FCefWebSocketSendRunnable API. */
	FCefWebSocketSendRunnable(FCefWebSocketServerInstance* InOwner, FEvent* InWakeEvent);
	virtual ~FCefWebSocketSendRunnable() override = default;
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

