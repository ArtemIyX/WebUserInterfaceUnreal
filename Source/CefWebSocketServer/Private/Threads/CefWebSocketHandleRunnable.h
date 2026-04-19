#pragma once

#include "HAL/Runnable.h"

class FCefWebSocketServerInstance;
class FEvent;

class FCefWebSocketHandleRunnable : public FRunnable
{
public:
#pragma region Lifecycle
	FCefWebSocketHandleRunnable(FCefWebSocketServerInstance* InOwner, FEvent* InWakeEvent);
	virtual ~FCefWebSocketHandleRunnable() override = default;
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
