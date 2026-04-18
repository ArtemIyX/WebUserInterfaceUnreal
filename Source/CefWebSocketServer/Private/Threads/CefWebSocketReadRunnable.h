#pragma once

#include "HAL/Runnable.h"

class FCefWebSocketServerInstance;

class FCefWebSocketReadRunnable : public FRunnable
{
public:
#pragma region Lifecycle
	explicit FCefWebSocketReadRunnable(FCefWebSocketServerInstance* InOwner);
	virtual ~FCefWebSocketReadRunnable() override = default;
#pragma endregion

#pragma region FRunnable
	virtual uint32 Run() override;
	virtual void Stop() override;
#pragma endregion

private:
#pragma region InternalState
	FCefWebSocketServerInstance* Owner = nullptr;
	TAtomic<bool> bStopRequested = false;
#pragma endregion
};
