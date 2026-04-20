/**
 * @file CefWebUi\Public\Services\CefConsoleLogReader.h
 * @brief Declares CefConsoleLogReader for module CefWebUi\Public\Services\CefConsoleLogReader.h.
 * @details Contains IPC reader/writer primitives used by the plugin runtime and gameplay-facing systems.
 */
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

/** @brief Type declaration. */
class CEFWEBUI_API FCefConsoleLogReader : public FRunnable
{
public:
	/** @brief FCefConsoleLogReader API. */
	FCefConsoleLogReader();
	virtual ~FCefConsoleLogReader() override;

	/** @brief Start API. */
	bool Start();
	virtual void Stop() override;
	virtual uint32 Run() override;

public:
	/** @brief OnConsoleLogMessage state. */
	FOnCefConsoleLogMessage OnConsoleLogMessage;

private:
	/** @brief CloseHandles API. */
	void CloseHandles();
	/** @brief DispatchEvent API. */
	void DispatchEvent(ECefConsoleLogLevel InLevel, const FString& InMessage, const FString& InSource, int32 InLine);

private:
	/** @brief HMap state. */
	Windows::HANDLE HMap = nullptr;
	/** @brief HEvent state. */
	Windows::HANDLE HEvent = nullptr;
	/** @brief PData state. */
	void* PData = nullptr;
	/** @brief Thread state. */
	FRunnableThread* Thread = nullptr;
	std::atomic<bool> bRunning{ false };
};

