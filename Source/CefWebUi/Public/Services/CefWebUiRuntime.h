#pragma once

#include "HAL/PlatformProcess.h"
#include "Containers/Ticker.h"

class FCefInputWriter;
class FCefFrameReader;
class FCefControlWriter;
class FCefConsoleLogReader;

class FCefWebUiRuntime
{
public:
	FCefWebUiRuntime() = default;
	~FCefWebUiRuntime();

	void EnsureStarted();
	void Shutdown();

	TSharedRef<FCefFrameReader> GetFrameReaderRef() const;
	TWeakPtr<FCefFrameReader> GetFrameReaderPtr() const;

	TSharedRef<FCefInputWriter> GetInputWriterRef() const;
	TWeakPtr<FCefInputWriter> GetInputWriterPtr() const;

	TSharedRef<FCefControlWriter> GetControlWriterRef() const;
	TWeakPtr<FCefControlWriter> GetControlWriterPtr() const;

	TSharedRef<FCefConsoleLogReader> GetConsoleLogReaderRef() const;
	TWeakPtr<FCefConsoleLogReader> GetConsoleLogReaderPtr() const;

private:
	bool TickStartRetry(float InDeltaSeconds);
	void LaunchHostProcess();
	void KillHostProcess();

private:
	bool bStarted = false;
	bool bFrameReaderStarted = false;
	bool bConsoleLogReaderStarted = false;
	TSharedPtr<FCefFrameReader> FrameReader;
	TSharedPtr<FCefInputWriter> InputWriter;
	TSharedPtr<FCefControlWriter> ControlWriter;
	TSharedPtr<FCefConsoleLogReader> ConsoleLogReader;
	FTSTicker::FDelegateHandle StartRetryTickerHandle;
	FProcHandle HostProcess;
	void* JobHandle = nullptr;
};
