// CefFrameReader.cpp

#include "Services/CefFrameReader.h"
#include "CefWebUi.h"
#include "Async/Async.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/PreWindowsApi.h"
#include <Windows.h>
#include "Windows/PostWindowsApi.h"
#include "Windows/HideWindowsPlatformTypes.h"

// Copy layout from CEF side - keep in sync with SharedMemoryLayout.h
constexpr uint32 SHM_MAX_WIDTH = 3840;
constexpr uint32 SHM_MAX_HEIGHT = 2160;
constexpr uint32 SHM_FRAME_SIZE = SHM_MAX_WIDTH * SHM_MAX_HEIGHT * 4;

struct FCefFrameHeader
{
	uint32 Width;
	uint32 Height;
	uint32 Sequence;
	uint32 WriteSlot;
	uint8 CursorType;
	uint8 LoadState;
	uint8 Reserved[2];
};


constexpr uint32 SHM_FRAME_TOTAL = sizeof(FCefFrameHeader) + SHM_FRAME_SIZE * 2;

FCefFrameReader::FCefFrameReader()
{
}

FCefFrameReader::~FCefFrameReader()
{
	FCefFrameReader::Stop();
}

bool FCefFrameReader::Start()
{
	HMap = OpenFileMappingW(FILE_MAP_READ, Windows::FALSE, L"CEFHost_Frame");
	if (!HMap)
	{
		UE_LOG(LogCefWebUi, Warning, TEXT("FCefFrameReader: Shared memory not available yet."));
		return false;
	}

	PData = MapViewOfFile(HMap, FILE_MAP_READ, 0, 0, SHM_FRAME_TOTAL);
	if (!PData)
	{
		UE_LOG(LogCefWebUi, Error, TEXT("FCefFrameReader: MapViewOfFile failed."));
		CloseHandles();
		return false;
	}

	HEvent = OpenEventW(SYNCHRONIZE | EVENT_MODIFY_STATE,
	                    Windows::FALSE, L"CEFHost_FrameReady");
	if (!HEvent)
	{
		UE_LOG(LogCefWebUi, Error, TEXT("FCefFrameReader: OpenEvent failed."));
		CloseHandles();
		return false;
	}

	bRunning = true;
	Thread = FRunnableThread::Create(this, TEXT("CefFrameReaderThread"), 0, TPri_Normal);

	UE_LOG(LogCefWebUi, Log, TEXT("FCefFrameReader: Started."));
	return true;
}

void FCefFrameReader::Stop()
{
	bRunning = false;

	if (HEvent)
		SetEvent(HEvent); // wake thread if waiting

	if (Thread)
	{
		FRunnableThread* threadToDelete = Thread;
		Thread = nullptr; // null BEFORE delete to break recursion
		threadToDelete->WaitForCompletion();
		delete threadToDelete;
	}

	CloseHandles();
	UE_LOG(LogCefWebUi, Log, TEXT("FCefFrameReader: Stopped."));
}

uint32 FCefFrameReader::Run()
{
	while (bRunning)
	{
		DWORD result = WaitForSingleObject(HEvent, 500);

		if (!bRunning)
			break;

		if (result != WAIT_OBJECT_0)
			continue;

		const FCefFrameHeader* header = reinterpret_cast<const FCefFrameHeader*>(PData);

		if (header->Sequence == LastSequence)
			continue;

		LastSequence = header->Sequence;

		const uint32 width = header->Width;
		const uint32 height = header->Height;
		const size_t size = static_cast<size_t>(width) * height * 4;
		const uint32 writeSlot = header->WriteSlot & 1u;
		const uint8* src = reinterpret_cast<const uint8*>(PData)
			+ sizeof(FCefFrameHeader)
			+ static_cast<size_t>(writeSlot) * SHM_FRAME_SIZE;

		{
			FScopeLock lock(&PendingFrameLock);
			PendingFrame.Width = width;
			PendingFrame.Height = height;
			PendingFrame.Sequence = header->Sequence;
			PendingFrame.CursorType = static_cast<ECefCustomCursorType>(header->CursorType);
			PendingFrame.LoadState = static_cast<ECefLoadState>(header->LoadState);
			PendingFrame.Pixels.SetNumUninitialized(size);
			FMemory::Memcpy(PendingFrame.Pixels.GetData(), src, size);
		}

		bFramePending = true;

		// Fire load state delegate on game thread if changed
		ECefLoadState currentLoad = static_cast<ECefLoadState>(header->LoadState);
		if (currentLoad != LastLoadState)
		{
			LastLoadState = currentLoad;
			AsyncTask(ENamedThreads::GameThread, [this, currentLoad]()
			{
				UE_LOG(LogCefWebUi, Log, TEXT("FCefFrameReader: Loading state changed %s (%d)"),
				       *UEnum::GetValueAsString(currentLoad),
				       static_cast<uint8>(currentLoad));
				OnLoadStateChanged.Broadcast(static_cast<uint8>(currentLoad));
			});
		}
	}

	return 0;
}

bool FCefFrameReader::PollFrame(FCefFrame& OutFrame)
{
	if (!bFramePending)
		return false;

	FScopeLock lock(&PendingFrameLock);
	OutFrame = MoveTemp(PendingFrame);
	bFramePending = false;
	return true;
}

EMouseCursor::Type FCefFrameReader::MapCefCursor(ECefCustomCursorType Type)
{
	switch (Type)
	{
	case ECefCustomCursorType::CT_POINTER: return EMouseCursor::Default;
	case ECefCustomCursorType::CT_CROSS: return EMouseCursor::Crosshairs;
	case ECefCustomCursorType::CT_HAND: return EMouseCursor::Hand;
	case ECefCustomCursorType::CT_IBEAM: return EMouseCursor::TextEditBeam;
	case ECefCustomCursorType::CT_WAIT: return EMouseCursor::Default; // no UE equivalent, use Default
	case ECefCustomCursorType::CT_EASTRESIZE:
	case ECefCustomCursorType::CT_WESTRESIZE:
	case ECefCustomCursorType::CT_EASTWESTRESIZE:
	case ECefCustomCursorType::CT_COLUMNRESIZE: return EMouseCursor::ResizeLeftRight;
	case ECefCustomCursorType::CT_NORTHRESIZE:
	case ECefCustomCursorType::CT_SOUTHRESIZE:
	case ECefCustomCursorType::CT_NORTHSOUTHRESIZE:
	case ECefCustomCursorType::CT_ROWRESIZE: return EMouseCursor::ResizeUpDown;
	case ECefCustomCursorType::CT_NORTHEASTRESIZE:
	case ECefCustomCursorType::CT_SOUTHWESTRESIZE:
	case ECefCustomCursorType::CT_NORTHEASTSOUTHWESTRESIZE: return EMouseCursor::ResizeSouthWest;
	case ECefCustomCursorType::CT_NORTHWESTRESIZE:
	case ECefCustomCursorType::CT_SOUTHEASTRESIZE:
	case ECefCustomCursorType::CT_NORTHWESTSOUTHEASTRESIZE: return EMouseCursor::ResizeSouthEast;
	case ECefCustomCursorType::CT_MOVE: return EMouseCursor::CardinalCross;
	case ECefCustomCursorType::CT_GRAB: return EMouseCursor::GrabHand;
	case ECefCustomCursorType::CT_GRABBING: return EMouseCursor::GrabHandClosed;
	case ECefCustomCursorType::CT_NODROP:
	case ECefCustomCursorType::CT_NOTALLOWED: return EMouseCursor::SlashedCircle;
	case ECefCustomCursorType::CT_NONE: return EMouseCursor::None;
	case ECefCustomCursorType::CT_ALIAS:
	case ECefCustomCursorType::CT_PROGRESS:
	case ECefCustomCursorType::CT_COPY:
	case ECefCustomCursorType::CT_ZOOMIN:
	case ECefCustomCursorType::CT_ZOOMOUT:
	case ECefCustomCursorType::CT_CELL:
	case ECefCustomCursorType::CT_CONTEXTMENU:
	case ECefCustomCursorType::CT_VERTICALTEXT:
	case ECefCustomCursorType::CT_HELP:
	case ECefCustomCursorType::CT_MIDDLEPANNING:
	case ECefCustomCursorType::CT_EASTPANNING:
	case ECefCustomCursorType::CT_NORTHPANNING:
	case ECefCustomCursorType::CT_NORTHEASTPANNING:
	case ECefCustomCursorType::CT_NORTHWESTPANNING:
	case ECefCustomCursorType::CT_SOUTHPANNING:
	case ECefCustomCursorType::CT_SOUTHEASTPANNING:
	case ECefCustomCursorType::CT_SOUTHWESTPANNING:
	case ECefCustomCursorType::CT_WESTPANNING:
	case ECefCustomCursorType::CT_MIDDLE_PANNING_VERTICAL:
	case ECefCustomCursorType::CT_MIDDLE_PANNING_HORIZONTAL:
	case ECefCustomCursorType::CT_DND_NONE:
	case ECefCustomCursorType::CT_DND_MOVE:
	case ECefCustomCursorType::CT_DND_COPY:
	case ECefCustomCursorType::CT_DND_LINK:
	case ECefCustomCursorType::CT_CUSTOM:
	default: return EMouseCursor::Default;
	}
}


void FCefFrameReader::CloseHandles()
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
