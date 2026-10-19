#include "Threads/CefWebSocketSendRunnable.h"

#include "Config/CefWebSocketCVars.h"
#include "Server/CefWebSocketServerInstance.h"
#include "HAL/Event.h"
#include "Math/UnrealMathUtility.h"

FCefWebSocketSendRunnable::FCefWebSocketSendRunnable(FCefWebSocketServerInstance* InOwner, FEvent* InWakeEvent)
	: Owner(InOwner)
	, WakeEvent(InWakeEvent)
{
}

uint32 FCefWebSocketSendRunnable::Run()
{
	while (!bStopRequested.Load())
	{
		const bool bDidWork = Owner ? Owner->PumpOutgoingOnSendThread() : false;
		if (!bDidWork && WakeEvent)
		{
			const uint32 waitMs = static_cast<uint32>(FMath::Max(1, CefWebSocketCVars::GetWriteIdleSleepMs()));
			WakeEvent->Wait(waitMs);
		}
	}
	return 0;
}

void FCefWebSocketSendRunnable::Stop()
{
	bStopRequested.Store(true);
	if (WakeEvent)
	{
		WakeEvent->Trigger();
	}
}
