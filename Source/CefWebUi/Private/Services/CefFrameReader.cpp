// CefFrameReader.cpp

#include "Services/CefFrameReader.h"
#include "CefWebUi.h"
#include "Async/Async.h"
#include <atomic>

#include "Windows/AllowWindowsPlatformTypes.h"

#include <Windows.h>

#include "Windows/HideWindowsPlatformTypes.h"

// Must match SharedMemoryLayout.h on CEF side
constexpr uint32 SHM_MAX_WIDTH = 3840;
constexpr uint32 SHM_MAX_HEIGHT = 2160;
constexpr uint32 SHM_FRAME_SIZE = SHM_MAX_WIDTH * SHM_MAX_HEIGHT * 4;

struct FCefFrameHeaderDirtyRect { int32 x, y, w, h; };
struct FCefFrameHeader
{
	uint32 version;
	uint32 slot_count;
	uint32 width;
	uint32 height;
	uint64 frame_id;
	uint64 present_id;
	uint32 sequence;
	uint32 write_slot;
	uint32 flags;
	uint8 cursor_type;
	uint8 load_state;
	uint8 dirty_count;
	uint8 reserved[3];
	FCefFrameHeaderDirtyRect dirty_rects[16];
};

constexpr uint32 SHM_FRAME_SLOT_COUNT = 3;
constexpr uint32 SHM_FRAME_TOTAL = sizeof(FCefFrameHeader) + SHM_FRAME_SIZE * SHM_FRAME_SLOT_COUNT;

FCefFrameReader::FCefFrameReader()
{
}

FCefFrameReader::~FCefFrameReader()
{
	FCefFrameReader::Stop();
}

bool FCefFrameReader::Start()
{
	HMap = OpenFileMappingW(FILE_MAP_READ, false, L"CEFHost_Frame");
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

	HEvent = OpenEventW(SYNCHRONIZE | EVENT_MODIFY_STATE, false, L"CEFHost_FrameReady");
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
		SetEvent(HEvent);

	if (Thread)
	{
		FRunnableThread* ThreadToDelete = Thread;
		Thread = nullptr;
		ThreadToDelete->WaitForCompletion();
		delete ThreadToDelete;
	}

	CloseHandles();
	UE_LOG(LogCefWebUi, Log, TEXT("FCefFrameReader: Stopped."));
}

uint32 FCefFrameReader::Run()
{
	while (bRunning)
	{
		DWORD result = WaitForSingleObject(HEvent, 500);
		//UE_LOG(LogCefWebUi, Log, TEXT("FCefFrameReader: WaitForSingleObject result=%d"), result);
		if (!bRunning)
			break;

		if (result != WAIT_OBJECT_0)
			continue;

		const FCefFrameHeader* header = reinterpret_cast<const FCefFrameHeader*>(PData);
		/*
		UE_LOG(LogCefWebUi, Log, TEXT("FCefFrameReader: seq=%u lastSeq=%u handle=%llu"),
		       header->Sequence, LastSequence, header->SharedTextureHandle);
		       */

		if (header->sequence == LastSequence)
			continue;

		std::atomic_thread_fence(std::memory_order_acquire);

		LastSequence = header->sequence;

		uint32 slotCount = header->slot_count;
		if (slotCount == 0 || slotCount > CEF_SHM_MAX_SLOTS)
			slotCount = CEF_SHM_MAX_SLOTS;

		const bool bFrameGap = (LastFrameId != 0 && header->frame_id != (LastFrameId + 1));
		const bool bForceFull = bFrameGap ||
			((header->flags & (CefFrameFlag_FullFrame | CefFrameFlag_Overflow | CefFrameFlag_Resized)) != 0) ||
			header->dirty_count == 0;
		LastFrameId = header->frame_id;

		{
			FScopeLock Lock(&PendingFrameLock);
			PendingFrame.Version = header->version;
			PendingFrame.SlotCount = slotCount;
			PendingFrame.WriteSlot = header->write_slot;
			PendingFrame.Width = header->width;
			PendingFrame.Height = header->height;
			PendingFrame.Sequence = header->sequence;
			PendingFrame.FrameId = header->frame_id;
			PendingFrame.PresentId = header->present_id;
			PendingFrame.Flags = header->flags;
			PendingFrame.bForceFullRefresh = bForceFull;
			PendingFrame.DirtyCount = bForceFull ? 0 : header->dirty_count;
			if (!bForceFull)
			{
				const uint8 n = FMath::Min<uint8>(header->dirty_count, MAX_CEF_DIRTY_RECTS);
				for (uint8 i = 0; i < n; ++i)
				{
					PendingFrame.DirtyRects[i].X = header->dirty_rects[i].x;
					PendingFrame.DirtyRects[i].Y = header->dirty_rects[i].y;
					PendingFrame.DirtyRects[i].W = header->dirty_rects[i].w;
					PendingFrame.DirtyRects[i].H = header->dirty_rects[i].h;
				}
			}
			PendingFrame.CursorType = static_cast<ECefCustomCursorType>(header->cursor_type);
			PendingFrame.LoadState = static_cast<ECefLoadState>(header->load_state);
		}

		bFramePending = true;

		// Fire load state delegate on game thread if changed
		ECefLoadState CurrentLoad = static_cast<ECefLoadState>(header->load_state);
		if (CurrentLoad != LastLoadState)
		{
			LastLoadState = CurrentLoad;
			AsyncTask(ENamedThreads::GameThread, [this, CurrentLoad]()
			{
				UE_LOG(LogCefWebUi, Log, TEXT("FCefFrameReader: Load state changed to %d"),
				       static_cast<uint8>(CurrentLoad));
				OnLoadStateChanged.Broadcast(static_cast<uint8>(CurrentLoad));
			});
		}
	}

	return 0;
}

bool FCefFrameReader::PollSharedTexture(FCefSharedFrame& OutFrame)
{
	if (!bFramePending)
		return false;

	FScopeLock Lock(&PendingFrameLock);
	OutFrame = PendingFrame;

	// Final continuity check at delivery point (game thread).
	// Reader thread may observe all frames, but game thread can still skip pending
	// updates between ticks. If that happens, force full refresh for safety.
	if (LastDeliveredFrameId != 0 && OutFrame.FrameId != (LastDeliveredFrameId + 1))
	{
		OutFrame.bForceFullRefresh = true;
		OutFrame.DirtyCount = 0;
	}
	LastDeliveredFrameId = OutFrame.FrameId;

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
