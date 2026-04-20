// CefFrameReader.cpp

#include "Services/CefFrameReader.h"
#include "CefWebUi.h"
#include "Async/Async.h"
#include <atomic>

#include "Windows/AllowWindowsPlatformTypes.h"

#include <Windows.h>

#include "Windows/HideWindowsPlatformTypes.h"

namespace
{
constexpr bool kEnableThreadTuning = true;
constexpr int32 kPendingQueueSize = 8;

ULONG_PTR SelectAffinityMask(ULONG_PTR InProcessMask, uint32 InLogicalIndex)
{
	if (InProcessMask == 0) return 0;
	uint32 count = 0;
	for (uint32 i = 0; i < sizeof(ULONG_PTR) * 8; ++i)
	{
		if (InProcessMask & (static_cast<ULONG_PTR>(1) << i))
			++count;
	}
	if (count == 0) return 0;
	const uint32 target = InLogicalIndex % count;
	uint32 seen = 0;
	for (uint32 i = 0; i < sizeof(ULONG_PTR) * 8; ++i)
	{
		const ULONG_PTR bit = static_cast<ULONG_PTR>(1) << i;
		if ((InProcessMask & bit) == 0) continue;
		if (seen == target) return bit;
		++seen;
	}
	return 0;
}

void TryTuneCurrentThread()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	ULONG_PTR processMask = 0, systemMask = 0;
	if (!GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask))
		return;

	const ULONG_PTR mask = SelectAffinityMask(processMask, 0);
	if (mask != 0)
		SetThreadAffinityMask(GetCurrentThread(), mask);
}
}

// Must match SharedMemoryLayout.h on CEF side
constexpr uint32 SHM_MAX_WIDTH = 3840;
constexpr uint32 SHM_MAX_HEIGHT = 2160;
constexpr uint32 SHM_FRAME_SIZE = SHM_MAX_WIDTH * SHM_MAX_HEIGHT * 4;

struct FCefFrameHeaderDirtyRect { int32 x, y, w, h; };
struct FCefFrameHeader
{
	uint32 protocol_magic;
	uint32 version;
	uint32 slot_count;
	uint32 width;
	uint32 height;
	uint64 frame_id;
	uint64 present_id;
	uint64 gpu_fence_value;
	uint32 sequence;
	uint32 write_slot;
	uint32 flags;
	uint8 cursor_type;
	uint8 load_state;
	uint8 popup_visible;
	uint8 dirty_count;
	uint8 reserved[2];
	FCefFrameHeaderDirtyRect popup_rect;
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

	const FCefFrameHeader* startupHeader = reinterpret_cast<const FCefFrameHeader*>(PData);
	if (startupHeader->protocol_magic != CEF_SHM_PROTOCOL_MAGIC || startupHeader->version != CEF_SHM_PROTOCOL_V2)
	{
		UE_LOG(LogCefWebUi, Error,
			TEXT("FCefFrameReader: Protocol handshake failed. Expected magic=0x%08X ver=%u, got magic=0x%08X ver=%u"),
			CEF_SHM_PROTOCOL_MAGIC, CEF_SHM_PROTOCOL_V2, startupHeader->protocol_magic, startupHeader->version);
		CloseHandles();
		return false;
	}
	LastKnownLoadStateRaw.store(startupHeader->load_state, std::memory_order_relaxed);
	LastLoadState = static_cast<ECefLoadState>(startupHeader->load_state);

	HEvent = OpenEventW(SYNCHRONIZE | EVENT_MODIFY_STATE, false, L"CEFHost_FrameReady");
	if (!HEvent)
	{
		UE_LOG(LogCefWebUi, Error, TEXT("FCefFrameReader: OpenEvent failed."));
		CloseHandles();
		return false;
	}

	bRunning = true;
	Thread = FRunnableThread::Create(this, TEXT("CefFrameReaderThread"), 0, TPri_Normal);

	// Publish startup load state once to avoid missing "already ready" cases
	// when no new frame event arrives after reader start.
	const uint8 startupLoadState = startupHeader->load_state;
	AsyncTask(ENamedThreads::GameThread, [this, startupLoadState]()
	{
		OnLoadStateChanged.Broadcast(startupLoadState);
	});

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
	{
		FScopeLock lock(&PendingFrameLock);
		PendingFrames.Reset();
		PendingFrameCount.store(0, std::memory_order_relaxed);
	}

	CloseHandles();
	UE_LOG(LogCefWebUi, Log, TEXT("FCefFrameReader: Stopped."));
}

uint32 FCefFrameReader::Run()
{
	if (kEnableThreadTuning)
	{
		TryTuneCurrentThread();
	}

	while (bRunning)
	{
		DWORD result = WaitForSingleObject(HEvent, 500);
		//UE_LOG(LogCefWebUi, Log, TEXT("FCefFrameReader: WaitForSingleObject result=%d"), result);
		if (!bRunning)
			break;

		if (result != WAIT_OBJECT_0)
			continue;

		const FCefFrameHeader* header = reinterpret_cast<const FCefFrameHeader*>(PData);
		if (header->protocol_magic != CEF_SHM_PROTOCOL_MAGIC || header->version != CEF_SHM_PROTOCOL_V2)
		{
			UE_LOG(LogCefWebUi, Error,
				TEXT("FCefFrameReader: Protocol mismatch at runtime. magic=0x%08X ver=%u"),
				header->protocol_magic, header->version);
			bRunning = false;
			break;
		}
		/*
		UE_LOG(LogCefWebUi, Log, TEXT("FCefFrameReader: seq=%u lastSeq=%u handle=%llu"),
		       header->Sequence, LastSequence, header->SharedTextureHandle);
		       */

		const uint32 beginSequence = header->sequence;
		if (beginSequence == LastSequence)
			continue;

		std::atomic_thread_fence(std::memory_order_acquire);
		FCefFrameHeader headerSnapshot = *header;
		std::atomic_thread_fence(std::memory_order_acquire);
		const uint32 endSequence = header->sequence;
		if (beginSequence != endSequence || endSequence == LastSequence)
		{
			continue;
		}

		LastSequence = endSequence;

		uint32 slotCount = headerSnapshot.slot_count;
		if (slotCount == 0 || slotCount > CEF_SHM_MAX_SLOTS)
			slotCount = CEF_SHM_MAX_SLOTS;

		const bool bFrameGap = (LastFrameId != 0 && headerSnapshot.frame_id != (LastFrameId + 1));
		const bool bForceFull = bFrameGap ||
			((headerSnapshot.flags & (CefFrameFlag_FullFrame | CefFrameFlag_Overflow | CefFrameFlag_Resized)) != 0) ||
			headerSnapshot.dirty_count == 0;
		LastFrameId = headerSnapshot.frame_id;

		{
			FScopeLock lock(&PendingFrameLock);
			if (PendingFrames.Num() >= kPendingQueueSize)
			{
				PendingFrames.RemoveAt(0, 1, EAllowShrinking::No);
				DroppedPendingFrames.fetch_add(1, std::memory_order_relaxed);
			}

			FCefSharedFrame queuedFrame;
			queuedFrame.Version = headerSnapshot.version;
			queuedFrame.SlotCount = slotCount;
			queuedFrame.WriteSlot = headerSnapshot.write_slot;
			queuedFrame.Width = headerSnapshot.width;
			queuedFrame.Height = headerSnapshot.height;
			queuedFrame.Sequence = headerSnapshot.sequence;
			queuedFrame.FrameId = headerSnapshot.frame_id;
			queuedFrame.PresentId = headerSnapshot.present_id;
			queuedFrame.GpuFenceValue = headerSnapshot.gpu_fence_value;
			queuedFrame.Flags = headerSnapshot.flags;
			queuedFrame.bPopupVisible = (headerSnapshot.popup_visible != 0);
			queuedFrame.PopupRect.X = headerSnapshot.popup_rect.x;
			queuedFrame.PopupRect.Y = headerSnapshot.popup_rect.y;
			queuedFrame.PopupRect.W = headerSnapshot.popup_rect.w;
			queuedFrame.PopupRect.H = headerSnapshot.popup_rect.h;
			queuedFrame.bForceFullRefresh = bForceFull;
			queuedFrame.DirtyCount = bForceFull ? 0 : headerSnapshot.dirty_count;
			if (!bForceFull)
			{
				const uint8 n = FMath::Min<uint8>(headerSnapshot.dirty_count, MAX_CEF_DIRTY_RECTS);
				for (uint8 i = 0; i < n; ++i)
				{
					queuedFrame.DirtyRects[i].X = headerSnapshot.dirty_rects[i].x;
					queuedFrame.DirtyRects[i].Y = headerSnapshot.dirty_rects[i].y;
					queuedFrame.DirtyRects[i].W = headerSnapshot.dirty_rects[i].w;
					queuedFrame.DirtyRects[i].H = headerSnapshot.dirty_rects[i].h;
				}
			}
			queuedFrame.CursorType = static_cast<ECefCustomCursorType>(headerSnapshot.cursor_type);
			queuedFrame.LoadState = static_cast<ECefLoadState>(headerSnapshot.load_state);
			PendingFrames.Add(queuedFrame);
			PendingFrameCount.store(static_cast<uint32>(PendingFrames.Num()), std::memory_order_relaxed);
		}
		LastKnownLoadStateRaw.store(headerSnapshot.load_state, std::memory_order_relaxed);

		AsyncTask(ENamedThreads::GameThread, [this]()
		{
			OnFrameReady.Broadcast();
		});

		// Fire load state delegate on game thread if changed
		ECefLoadState currentLoad = static_cast<ECefLoadState>(headerSnapshot.load_state);
		if (currentLoad != LastLoadState)
		{
			LastLoadState = currentLoad;
			AsyncTask(ENamedThreads::GameThread, [this, currentLoad]()
			{
				UE_LOG(LogCefWebUi, Log, TEXT("FCefFrameReader: Load state changed to %d"),
				       static_cast<uint8>(currentLoad));
				OnLoadStateChanged.Broadcast(static_cast<uint8>(currentLoad));
			});
		}
	}

	return 0;
}

bool FCefFrameReader::PollSharedTexture(FCefSharedFrame& OutFrame)
{
	if (PendingFrameCount.load(std::memory_order_relaxed) == 0)
		return false;

	FScopeLock lock(&PendingFrameLock);
	if (PendingFrames.Num() == 0)
	{
		PendingFrameCount.store(0, std::memory_order_relaxed);
		return false;
	}

	if (PendingFrames.Num() > 1)
	{
		DroppedPendingFrames.fetch_add(static_cast<uint32>(PendingFrames.Num() - 1), std::memory_order_relaxed);
	}

	OutFrame = PendingFrames.Last();
	PendingFrames.Reset();
	PendingFrameCount.store(0, std::memory_order_relaxed);

	if (LastDeliveredFrameId != 0 && OutFrame.FrameId <= LastDeliveredFrameId)
	{
		const bool bResizeReset = ((OutFrame.Flags & CefFrameFlag_Resized) != 0);
		if (!bResizeReset)
		{
			return false;
		}

		UE_LOG(LogCefWebUi, Warning,
			TEXT("FCefFrameReader: FrameId reset on resize (last=%llu new=%llu), accepting new sequence."),
			static_cast<unsigned long long>(LastDeliveredFrameId),
			static_cast<unsigned long long>(OutFrame.FrameId));
		LastDeliveredFrameId = 0;
	}
	if (LastDeliveredFrameId != 0 && OutFrame.FrameId != (LastDeliveredFrameId + 1))
	{
		OutFrame.bForceFullRefresh = true;
		OutFrame.DirtyCount = 0;
	}
	LastDeliveredFrameId = OutFrame.FrameId;
	return true;
}

uint32 FCefFrameReader::ConsumeDroppedPendingFrames()
{
	return DroppedPendingFrames.exchange(0, std::memory_order_relaxed);
}

ECefLoadState FCefFrameReader::GetLastKnownLoadState() const
{
	return static_cast<ECefLoadState>(LastKnownLoadStateRaw.load(std::memory_order_relaxed));
}

EMouseCursor::Type FCefFrameReader::MapCefCursor(ECefCustomCursorType InType)
{
	switch (InType)
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
