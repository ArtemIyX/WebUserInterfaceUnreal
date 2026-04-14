#pragma once

#include "CoreMinimal.h"
#include "Data/CefConsoleLogLevel.h"

namespace Windows
{
	typedef void* HANDLE;
}

DECLARE_MULTICAST_DELEGATE_FourParams(
	FOnCefConsoleLogMessage,
	ECefConsoleLogLevel /*Level*/,
	const FString& /*Message*/,
	const FString& /*Source*/,
	int32 /*Line*/);

class CEFWEBUI_API FCefConsoleLogReader : public FRunnable
{
public:
	FCefConsoleLogReader();
	virtual ~FCefConsoleLogReader() override;

	bool Start();
	virtual void Stop() override;
	virtual uint32 Run() override;

public:
	FOnCefConsoleLogMessage OnConsoleLogMessage;

private:
	void CloseHandles();
	void DispatchEvent(ECefConsoleLogLevel level, const FString& message, const FString& source, int32 line);

private:
	Windows::HANDLE HMap = nullptr;
	Windows::HANDLE HEvent = nullptr;
	void* PData = nullptr;
	FRunnableThread* Thread = nullptr;
	std::atomic<bool> bRunning{ false };
};
