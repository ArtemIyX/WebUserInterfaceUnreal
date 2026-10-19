/**
 * @file CefWebSocketServer\Private\Threads\CefWebSocketReadRunnable.h
 * @brief Declares CefWebSocketReadRunnable for module CefWebSocketServer\Private\Threads\CefWebSocketReadRunnable.h.
 * @details Contains websocket server components used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "HAL/Runnable.h"

class FCefWebSocketServerInstance;

/** @brief Type declaration. */
class FCefWebSocketReadRunnable : public FRunnable
{
public:
#pragma region Lifecycle
	/** @brief FCefWebSocketReadRunnable API. */
	explicit FCefWebSocketReadRunnable(FCefWebSocketServerInstance* InOwner);
	virtual ~FCefWebSocketReadRunnable() override = default;
#pragma endregion

#pragma region FRunnable
	virtual uint32 Run() override;
	virtual void Stop() override;
#pragma endregion

private:
#pragma region InternalState
	/** @brief Owner state. */
	FCefWebSocketServerInstance* Owner = nullptr;
	/** @brief bStopRequested state. */
	TAtomic<bool> bStopRequested = false;
#pragma endregion
};

