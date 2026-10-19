#include "Threads/CefWebSocketReadRunnable.h"

#include "Config/CefWebSocketCVars.h"
#include "Server/CefWebSocketServerInstance.h"
#include "HAL/PlatformProcess.h"
#include "Math/UnrealMathUtility.h"

FCefWebSocketReadRunnable::FCefWebSocketReadRunnable(FCefWebSocketServerInstance* InOwner)
	: Owner(InOwner)
{
}

uint32 FCefWebSocketReadRunnable::Run()
{
	int32 idleSleepMs = FMath::Max(1, CefWebSocketCVars::GetReadBusySleepMs());
	while (!bStopRequested.Load())
	{
		bool bHadActivity = false;
		if (Owner)
		{
			bHadActivity = Owner->TickBackendOnReadThread();
		}

		const int32 busySleepMs = FMath::Max(1, CefWebSocketCVars::GetReadBusySleepMs());
		const int32 idleMaxSleepMs = FMath::Max(busySleepMs, CefWebSocketCVars::GetReadIdleMaxSleepMs());
		if (bHadActivity)
		{
			idleSleepMs = busySleepMs;
		}
		else
		{
			idleSleepMs = FMath::Min(idleMaxSleepMs, idleSleepMs * 2);
		}

		FPlatformProcess::SleepNoStats(static_cast<float>(idleSleepMs) / 1000.0f);
	}
	return 0;
}

void FCefWebSocketReadRunnable::Stop()
{
	bStopRequested.Store(true);
}
