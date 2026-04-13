#include "Services/CefWebUiRuntime.h"

#include "CefWebUi.h"
#include "Libs/CefWebUiBPLibrary.h"
#include "Services/CefControlWriter.h"
#include "Services/CefFrameReader.h"
#include "Services/CefInputWriter.h"
#include "Misc/Paths.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"

FCefWebUiRuntime::~FCefWebUiRuntime()
{
	Shutdown();
}

void FCefWebUiRuntime::EnsureStarted()
{
	if (bStarted)
		return;

	FrameReader = MakeShared<FCefFrameReader>();
	InputWriter = MakeShared<FCefInputWriter>();
	ControlWriter = MakeShared<FCefControlWriter>();
	bStarted = true;
	bFrameReaderStarted = false;

	LaunchHostProcess();
	TickStartRetry(0.0f);

	if (!StartRetryTickerHandle.IsValid())
	{
		StartRetryTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateRaw(this, &FCefWebUiRuntime::TickStartRetry),
			0.2f);
	}
}

void FCefWebUiRuntime::Shutdown()
{
	if (StartRetryTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(StartRetryTickerHandle);
		StartRetryTickerHandle.Reset();
	}

	if (FrameReader.IsValid())
	{
		FrameReader->Stop();
		FrameReader.Reset();
	}
	if (InputWriter.IsValid())
	{
		InputWriter.Reset();
	}
	if (ControlWriter.IsValid())
	{
		ControlWriter.Reset();
	}

	KillHostProcess();
	bStarted = false;
	bFrameReaderStarted = false;
}

bool FCefWebUiRuntime::TickStartRetry(float)
{
	if (!bStarted)
		return false;

	bool bInputOk = false;
	bool bControlOk = false;
	bool bFrameOk = bFrameReaderStarted;

	if (InputWriter.IsValid())
	{
		bInputOk = InputWriter->IsOpen() || InputWriter->Open();
		if (!bInputOk)
			UE_LOG(LogCefWebUi, Verbose, TEXT("FCefInputWriter: Open retry failed (host not ready yet)."));
	}

	if (ControlWriter.IsValid())
	{
		bControlOk = ControlWriter->IsOpen() || ControlWriter->Open();
		if (!bControlOk)
			UE_LOG(LogCefWebUi, Verbose, TEXT("FCefControlWriter: Open retry failed (host not ready yet)."));
	}

	if (!bFrameReaderStarted && FrameReader.IsValid())
	{
		bFrameReaderStarted = FrameReader->Start();
		bFrameOk = bFrameReaderStarted;
	}

	const bool bReady = bInputOk && bControlOk && bFrameOk;
	if (bReady)
	{
		UE_LOG(LogCefWebUi, Log, TEXT("CefWebUi: Runtime connected (host + IPC ready)."));
		StartRetryTickerHandle.Reset();
		return false;
	}
	return true;
}

void FCefWebUiRuntime::LaunchHostProcess()
{
	FString hostPath = UCefWebUiBPLibrary::GetHostExePath();
	if (!FPaths::FileExists(hostPath))
	{
		UE_LOG(LogCefWebUi, Error, TEXT("CefWebUi: Host.exe not found at %s"), *hostPath);
		return;
	}

	JobHandle = CreateJobObject(nullptr, nullptr);
	if (JobHandle)
	{
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = {};
		jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
		SetInformationJobObject(JobHandle, JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo));
	}

	STARTUPINFO si = {};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = {};

	FString cmdLine = FString::Printf(TEXT("\"%s\""), *hostPath);
	if (!CreateProcess(
		nullptr,
		cmdLine.GetCharArray().GetData(),
		nullptr, nullptr,
		Windows::FALSE, 0,
		nullptr, nullptr,
		&si, &pi))
	{
		UE_LOG(LogCefWebUi, Error, TEXT("CefWebUi: Failed to launch Host.exe (error %d)"), GetLastError());
		return;
	}

	if (JobHandle)
	{
		AssignProcessToJobObject(JobHandle, pi.hProcess);
	}

	HostProcess = FProcHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	UE_LOG(LogCefWebUi, Log, TEXT("CefWebUi: Host.exe launched from %s"), *hostPath);
}

void FCefWebUiRuntime::KillHostProcess()
{
	if (JobHandle)
	{
		CloseHandle(JobHandle);
		JobHandle = nullptr;
		UE_LOG(LogCefWebUi, Log, TEXT("CefWebUi: Host job closed, all child processes terminated"));
	}

	if (HostProcess.IsValid())
	{
		HANDLE rawHandle = HostProcess.Get();
		if (WaitForSingleObject(rawHandle, 1000) == WAIT_TIMEOUT)
		{
			TerminateProcess(rawHandle, 1);
		}
		FPlatformProcess::CloseProc(HostProcess);
		UE_LOG(LogCefWebUi, Log, TEXT("CefWebUi: Host.exe handle closed"));
	}
}

TSharedRef<FCefFrameReader> FCefWebUiRuntime::GetFrameReaderRef() const
{
	ensure(FrameReader.IsValid());
	return FrameReader.ToSharedRef();
}

TWeakPtr<FCefFrameReader> FCefWebUiRuntime::GetFrameReaderPtr() const
{
	ensure(FrameReader.IsValid());
	return FrameReader.ToWeakPtr();
}

TSharedRef<FCefInputWriter> FCefWebUiRuntime::GetInputWriterRef() const
{
	ensure(InputWriter.IsValid());
	return InputWriter.ToSharedRef();
}

TWeakPtr<FCefInputWriter> FCefWebUiRuntime::GetInputWriterPtr() const
{
	ensure(InputWriter.IsValid());
	return InputWriter.ToWeakPtr();
}

TSharedRef<FCefControlWriter> FCefWebUiRuntime::GetControlWriterRef() const
{
	ensure(ControlWriter.IsValid());
	return ControlWriter.ToSharedRef();
}

TWeakPtr<FCefControlWriter> FCefWebUiRuntime::GetControlWriterPtr() const
{
	ensure(ControlWriter.IsValid());
	return ControlWriter.ToWeakPtr();
}
