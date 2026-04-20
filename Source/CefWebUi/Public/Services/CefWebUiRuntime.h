/**
 * @file CefWebUi\Public\Services\CefWebUiRuntime.h
 * @brief Declares CefWebUiRuntime for module CefWebUi\Public\Services\CefWebUiRuntime.h.
 * @details Contains runtime process and IPC orchestration used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "HAL/PlatformProcess.h"
#include "Containers/Ticker.h"

class FCefInputWriter;
class FCefFrameReader;
class FCefControlWriter;
class FCefConsoleLogReader;

/** @brief Type declaration. */
class FCefWebUiRuntime
{
public:
	FCefWebUiRuntime() = default;
	/** @brief FCefWebUiRuntime API. */
	~FCefWebUiRuntime();

	/** @brief EnsureStarted API. */
	void EnsureStarted();
	/** @brief Shutdown API. */
	void Shutdown();

	/** @brief GetFrameReaderRef API. */
	TSharedRef<FCefFrameReader> GetFrameReaderRef() const;
	/** @brief GetFrameReaderPtr API. */
	TWeakPtr<FCefFrameReader> GetFrameReaderPtr() const;

	/** @brief GetInputWriterRef API. */
	TSharedRef<FCefInputWriter> GetInputWriterRef() const;
	/** @brief GetInputWriterPtr API. */
	TWeakPtr<FCefInputWriter> GetInputWriterPtr() const;

	/** @brief GetControlWriterRef API. */
	TSharedRef<FCefControlWriter> GetControlWriterRef() const;
	/** @brief GetControlWriterPtr API. */
	TWeakPtr<FCefControlWriter> GetControlWriterPtr() const;

	/** @brief GetConsoleLogReaderRef API. */
	TSharedRef<FCefConsoleLogReader> GetConsoleLogReaderRef() const;
	/** @brief GetConsoleLogReaderPtr API. */
	TWeakPtr<FCefConsoleLogReader> GetConsoleLogReaderPtr() const;

private:
	/** @brief TickStartRetry API. */
	bool TickStartRetry(float InDeltaSeconds);
	/** @brief LaunchHostProcess API. */
	void LaunchHostProcess();
	/** @brief KillHostProcess API. */
	void KillHostProcess();

private:
	/** @brief bStarted state. */
	bool bStarted = false;
	/** @brief bFrameReaderStarted state. */
	bool bFrameReaderStarted = false;
	/** @brief bConsoleLogReaderStarted state. */
	bool bConsoleLogReaderStarted = false;
	/** @brief FrameReader state. */
	TSharedPtr<FCefFrameReader> FrameReader;
	/** @brief InputWriter state. */
	TSharedPtr<FCefInputWriter> InputWriter;
	/** @brief ControlWriter state. */
	TSharedPtr<FCefControlWriter> ControlWriter;
	/** @brief ConsoleLogReader state. */
	TSharedPtr<FCefConsoleLogReader> ConsoleLogReader;
	/** @brief StartRetryTickerHandle state. */
	FTSTicker::FDelegateHandle StartRetryTickerHandle;
	/** @brief HostProcess state. */
	FProcHandle HostProcess;
	/** @brief JobHandle state. */
	void* JobHandle = nullptr;
};

