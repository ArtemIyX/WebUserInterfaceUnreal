#include "Threads/CefWebSocketWriteRunnable.h"

#include "Config/CefWebSocketCVars.h"
#include "Server/CefWebSocketServerInstance.h"
#include "HAL/Event.h"
#include "Math/UnrealMathUtility.h"

FCefWebSocketWriteRunnable::FCefWebSocketWriteRunnable(FCefWebSocketServerInstance* InOwner, FEvent* InWakeEvent)
	: Owner(InOwner)
	, WakeEvent(InWakeEvent)
{
}

uint32 FCefWebSocketWriteRunnable::Run()
{
	while (!bStopRequested.Load())
	{
		const bool bDidWork = Owner ? Owner->PumpOutgoingOnWriteThread() : false;
		if (!bDidWork && WakeEvent)
		{
			const uint32 WaitMs = static_cast<uint32>(FMath::Max(1, CefWebSocketCVars::GetWriteIdleSleepMs()));
			WakeEvent->Wait(WaitMs);
		}
	}
	return 0;
}

void FCefWebSocketWriteRunnable::Stop()
{
	bStopRequested.Store(true);
	if (WakeEvent)
	{
		WakeEvent->Trigger();
	}
}
