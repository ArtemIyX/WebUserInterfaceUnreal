#include "Threads/CefWebSocketHandleRunnable.h"

#include "Config/CefWebSocketCVars.h"
#include "Server/CefWebSocketServerInstance.h"
#include "HAL/Event.h"
#include "Math/UnrealMathUtility.h"

FCefWebSocketHandleRunnable::FCefWebSocketHandleRunnable(FCefWebSocketServerInstance* InOwner, FEvent* InWakeEvent)
	: Owner(InOwner)
	, WakeEvent(InWakeEvent)
{
}

uint32 FCefWebSocketHandleRunnable::Run()
{
	while (!bStopRequested.Load())
	{
		const bool bDidWork = Owner ? Owner->PumpInboundOnHandleThread() : false;
		if (!bDidWork && WakeEvent)
		{
			const uint32 waitMs = static_cast<uint32>(FMath::Max(1, CefWebSocketCVars::GetWriteIdleSleepMs()));
			WakeEvent->Wait(waitMs);
		}
	}
	return 0;
}

void FCefWebSocketHandleRunnable::Stop()
{
	bStopRequested.Store(true);
	if (WakeEvent)
	{
		WakeEvent->Trigger();
	}
}
