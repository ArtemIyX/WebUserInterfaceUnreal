#include "Threads/CefWebSocketReadRunnable.h"

#include "Server/CefWebSocketServerInstance.h"
#include "HAL/PlatformProcess.h"

FCefWebSocketReadRunnable::FCefWebSocketReadRunnable(FCefWebSocketServerInstance* InOwner)
	: Owner(InOwner)
{
}

uint32 FCefWebSocketReadRunnable::Run()
{
	while (!bStopRequested.Load())
	{
		if (Owner)
		{
			Owner->TickBackendOnReadThread();
		}
		FPlatformProcess::SleepNoStats(0.001f);
	}
	return 0;
}

void FCefWebSocketReadRunnable::Stop()
{
	bStopRequested.Store(true);
}
