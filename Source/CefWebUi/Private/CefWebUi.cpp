// CefWebUiModule.cpp
#include "CefWebUi.h"

#include "HAL/PlatformProcess.h"
#include "Libs/CefWebUiBPLibrary.h"
#include "Services/CefFrameReader.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>

#include "Services/CefInputWriter.h"
#include "Windows/HideWindowsPlatformTypes.h"


IMPLEMENT_MODULE(FCefWebUiModule, CefWebUi)

DEFINE_LOG_CATEGORY(LogCefWebUi);

FCefWebUiModule& FCefWebUiModule::Get()
{
	return FModuleManager::LoadModuleChecked<FCefWebUiModule>("CefWebUi");
}

bool FCefWebUiModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded("CefWebUi");
}

void FCefWebUiModule::StartupModule()
{
	LaunchHostProcess();

	FrameReader = MakeShared<FCefFrameReader>();
	InputWriter = MakeShared<FCefInputWriter>();
	if (!InputWriter->Open())
	{
		UE_LOG(LogCefWebUi, Warning, TEXT("FCefInputWriter: Open failed - Host not ready yet."));
	}

	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this](float) -> bool
		{
			return !FrameReader->Start(); // returns false (stop ticking) on success
		}),
		0.2f
	);
}

void FCefWebUiModule::ShutdownModule()
{
	if (FrameReader.IsValid())
	{
		FrameReader->Stop();
		FrameReader.Reset();
	}
	if (InputWriter.IsValid())
	{
		InputWriter.Reset();
	}
	KillHostProcess();
}

void FCefWebUiModule::LaunchHostProcess()
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

	// Wrap into FProcHandle so the rest of your code stays compatible
	HostProcess = FProcHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	UE_LOG(LogCefWebUi, Log, TEXT("CefWebUi: Host.exe launched from %s"), *hostPath);
}

void FCefWebUiModule::KillHostProcess()
{
	// Job kill-on-close covers Host.exe and all its children.
	// Close job first so the OS terminates everything atomically.
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

TSharedRef<FCefFrameReader> FCefWebUiModule::GetFrameReaderRef() const
{
	ensure(FrameReader.IsValid());
	return FrameReader.ToSharedRef();
}

TWeakPtr<FCefFrameReader> FCefWebUiModule::GetFrameReaderPtr() const
{
	ensure(FrameReader.IsValid());
	return FrameReader.ToWeakPtr();
}

TSharedRef<FCefInputWriter> FCefWebUiModule::GetInputWriteRef() const
{
	ensure(InputWriter.IsValid());
	return InputWriter.ToSharedRef();
}

TWeakPtr<FCefInputWriter> FCefWebUiModule::GeInputWriterPtr() const
{
	ensure(InputWriter.IsValid());
	return InputWriter.ToWeakPtr();
}
