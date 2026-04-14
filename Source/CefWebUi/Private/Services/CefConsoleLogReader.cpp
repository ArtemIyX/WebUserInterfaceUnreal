#include "Services/CefConsoleLogReader.h"

#include "CefWebUi.h"
#include "Async/Async.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include <atomic>
#include "Windows/HideWindowsPlatformTypes.h"

namespace
{
static constexpr uint32 CEF_CONSOLE_RING_CAPACITY = 256;
static constexpr uint32 CEF_CONSOLE_MESSAGE_MAX = 1024;
static constexpr uint32 CEF_CONSOLE_SOURCE_MAX = 256;
static constexpr const wchar_t* CEF_SHM_CONSOLE_NAME = L"CEFHost_Console";
static constexpr const wchar_t* CEF_EVT_CONSOLE_READY = L"CEFHost_ConsoleReady";

#pragma pack(push, 1)
struct FCefConsoleEvent
{
	uint8 Level;
	uint8 Reserved[3];
	int32 Line;
	char16_t Source[CEF_CONSOLE_SOURCE_MAX];
	char16_t Message[CEF_CONSOLE_MESSAGE_MAX];
};

struct FCefConsoleRingBuffer
{
	std::atomic<uint32> WriteIndex;
	std::atomic<uint32> ReadIndex;
	uint32 Capacity;
	uint32 Reserved;
	FCefConsoleEvent Events[CEF_CONSOLE_RING_CAPACITY];
};
#pragma pack(pop)
}

FCefConsoleLogReader::FCefConsoleLogReader()
{
}

FCefConsoleLogReader::~FCefConsoleLogReader()
{
	FCefConsoleLogReader::Stop();
}

bool FCefConsoleLogReader::Start()
{
	HMap = OpenFileMappingW(FILE_MAP_READ, false, CEF_SHM_CONSOLE_NAME);
	if (!HMap)
	{
		UE_LOG(LogCefWebUi, Verbose, TEXT("FCefConsoleLogReader: Shared memory not available yet."));
		return false;
	}

	PData = MapViewOfFile(HMap, FILE_MAP_READ, 0, 0, sizeof(FCefConsoleRingBuffer));
	if (!PData)
	{
		UE_LOG(LogCefWebUi, Error, TEXT("FCefConsoleLogReader: MapViewOfFile failed."));
		CloseHandles();
		return false;
	}

	HEvent = OpenEventW(SYNCHRONIZE, false, CEF_EVT_CONSOLE_READY);
	if (!HEvent)
	{
		UE_LOG(LogCefWebUi, Error, TEXT("FCefConsoleLogReader: OpenEvent failed."));
		CloseHandles();
		return false;
	}

	bRunning = true;
	Thread = FRunnableThread::Create(this, TEXT("CefConsoleLogReaderThread"), 0, TPri_Normal);
	UE_LOG(LogCefWebUi, Log, TEXT("FCefConsoleLogReader: Started."));
	return true;
}

void FCefConsoleLogReader::Stop()
{
	bRunning = false;

	if (Thread)
	{
		FRunnableThread* threadToDelete = Thread;
		Thread = nullptr;
		threadToDelete->WaitForCompletion();
		delete threadToDelete;
	}

	CloseHandles();
}

uint32 FCefConsoleLogReader::Run()
{
	while (bRunning)
	{
		if (!HEvent || !PData)
		{
			break;
		}

		const DWORD waitResult = WaitForSingleObject(HEvent, 500);
		if (!bRunning)
		{
			break;
		}
		if (waitResult != WAIT_OBJECT_0)
		{
			continue;
		}

		FCefConsoleRingBuffer* ring = reinterpret_cast<FCefConsoleRingBuffer*>(PData);
		while (bRunning)
		{
			const uint32 readIndex = ring->ReadIndex.load(std::memory_order_acquire);
			const uint32 writeIndex = ring->WriteIndex.load(std::memory_order_acquire);
			if (readIndex == writeIndex)
			{
				break;
			}

			const FCefConsoleEvent event = ring->Events[readIndex % CEF_CONSOLE_RING_CAPACITY];
			ring->ReadIndex.store(readIndex + 1, std::memory_order_release);

			ECefConsoleLogLevel level = ECefConsoleLogLevel::Log;
			if (event.Level == 2)
			{
				level = ECefConsoleLogLevel::Error;
			}
			else if (event.Level == 1)
			{
				level = ECefConsoleLogLevel::Warning;
			}

			const FString message(reinterpret_cast<const UTF16CHAR*>(event.Message));
			const FString source(reinterpret_cast<const UTF16CHAR*>(event.Source));
			DispatchEvent(level, message, source, event.Line);
		}
	}

	return 0;
}

void FCefConsoleLogReader::DispatchEvent(ECefConsoleLogLevel level, const FString& message, const FString& source, int32 line)
{
	AsyncTask(ENamedThreads::GameThread, [this, level, message, source, line]()
	{
		OnConsoleLogMessage.Broadcast(level, message, source, line);
	});
}

void FCefConsoleLogReader::CloseHandles()
{
	if (PData)
	{
		UnmapViewOfFile(PData);
		PData = nullptr;
	}
	if (HMap)
	{
		CloseHandle(HMap);
		HMap = nullptr;
	}
	if (HEvent)
	{
		CloseHandle(HEvent);
		HEvent = nullptr;
	}
}
