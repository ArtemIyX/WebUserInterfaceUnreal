/**
 * @file CefWebUi\Public\Services\CefFrameReader.h
 * @brief Declares CefFrameReader for module CefWebUi\Public\Services\CefFrameReader.h.
 * @details Contains IPC reader/writer primitives used by the plugin runtime and gameplay-facing systems.
 */
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/CefCustomCursorType.h"
#include "Data/CefLoadState.h"

namespace Windows
{
	typedef void* HANDLE;
}

/** @brief MAX_CEF_DIRTY_RECTS state. */
constexpr uint32 MAX_CEF_DIRTY_RECTS = 16;
/** @brief CEF_SHM_PROTOCOL_V2 state. */
constexpr uint32 CEF_SHM_PROTOCOL_V2 = 4;
/** @brief CEF_SHM_MAX_SLOTS state. */
constexpr uint32 CEF_SHM_MAX_SLOTS = 3;
constexpr uint32 CEF_SHM_PROTOCOL_MAGIC = 0x43454648; // 'CEFH'

/** @brief Type declaration. */
enum ECefFrameFlags : uint32
{
	CefFrameFlag_None = 0,
	CefFrameFlag_FullFrame = 1u << 0,
	CefFrameFlag_DirtyOnly = 1u << 1,
	CefFrameFlag_Overflow = 1u << 2,
	CefFrameFlag_Resized = 1u << 3,
	CefFrameFlag_PopupPlane = 1u << 4,
};

/** @brief Type declaration. */
struct FCefDirtyRect
{
	/** @brief X state. */
	int32 X = 0, Y = 0, W = 0, H = 0;
};

/** @brief Type declaration. */
struct FCefSharedFrame
{
	/** @brief Version state. */
	uint32 Version = 0;
	/** @brief SlotCount state. */
	uint32 SlotCount = 2;
	/** @brief WriteSlot state. */
	uint32 WriteSlot = 0;
	/** @brief Width state. */
	uint32 Width = 0;
	/** @brief Height state. */
	uint32 Height = 0;
	/** @brief Sequence state. */
	uint32 Sequence = 0;
	/** @brief FrameId state. */
	uint64 FrameId = 0;
	/** @brief PresentId state. */
	uint64 PresentId = 0;
	/** @brief GpuFenceValue state. */
	uint64 GpuFenceValue = 0;
	/** @brief Flags state. */
	uint32 Flags = 0;
	/** @brief bPopupVisible state. */
	bool bPopupVisible = false;
	/** @brief PopupRect state. */
	FCefDirtyRect PopupRect;
	/** @brief CursorType state. */
	ECefCustomCursorType CursorType = ECefCustomCursorType::CT_NONE;
	/** @brief LoadState state. */
	ECefLoadState LoadState = ECefLoadState::Idle;
	/** @brief bForceFullRefresh state. */
	bool bForceFullRefresh = true;
	uint8 DirtyCount = 0; // 0 = full frame
	FCefDirtyRect DirtyRects[MAX_CEF_DIRTY_RECTS];
};
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCefLoadStateChanged, uint8);
DECLARE_MULTICAST_DELEGATE(FOnCefFrameReady);

/** @brief Type declaration. */
class CEFWEBUI_API FCefFrameReader : public FRunnable
{
public:
	/** @brief FCefFrameReader API. */
	FCefFrameReader();
	virtual ~FCefFrameReader() override;

	/**
	 * @brief Opens shared memory and starts background read thread.
	 * @return False if shared memory not available yet.
	 */
	bool Start();

	/**
	 * @brief Stops thread and closes handles.
	 */
	virtual void Stop() override;

	// FRunnable
	virtual uint32 Run() override;

public:
	/**
	 * @brief Call from game thread. Returns true if a new frame is available.
	 *        OutFrame.SharedTextureHandle is a duplicated NT handle valid in this process.
	 *        Caller must CloseHandle() it when done opening the D3D12 resource.
	 */
	/** @brief PollSharedTexture API. */
	bool PollSharedTexture(FCefSharedFrame& OutFrame);
	/** @brief ConsumeDroppedPendingFrames API. */
	uint32 ConsumeDroppedPendingFrames();
	/** @brief GetLastKnownLoadState API. */
	ECefLoadState GetLastKnownLoadState() const;

	/** @brief MapCefCursor API. */
	static EMouseCursor::Type MapCefCursor(ECefCustomCursorType InType);

public:
	/** @brief OnLoadStateChanged state. */
	FOnCefLoadStateChanged OnLoadStateChanged;
	/** @brief OnFrameReady state. */
	FOnCefFrameReady OnFrameReady;

private:
	/** @brief CloseHandles API. */
	void CloseHandles();

private:
	/** @brief LastLoadState state. */
	ECefLoadState LastLoadState = ECefLoadState::Idle;
	/** @brief PendingFrames state. */
	TArray<FCefSharedFrame> PendingFrames;
	/** @brief PendingFrameLock state. */
	FCriticalSection PendingFrameLock;

	/** @brief HMap state. */
	Windows::HANDLE HMap = nullptr;
	/** @brief HEvent state. */
	Windows::HANDLE HEvent = nullptr;
	/** @brief PData state. */
	void* PData = nullptr;

	/** @brief Thread state. */
	FRunnableThread* Thread = nullptr;
	std::atomic<uint32> PendingFrameCount{ 0 };
	std::atomic<bool> bRunning{ false };
	std::atomic<uint32> DroppedPendingFrames{ 0 };
	std::atomic<bool> bFrameReadyDispatchPending{ false };
	std::atomic<uint8> LastKnownLoadStateRaw{ static_cast<uint8>(ECefLoadState::Idle) };
	/** @brief LastSequence state. */
	uint32 LastSequence = 0;
	/** @brief LastFrameId state. */
	uint64 LastFrameId = 0;
	/** @brief LastDeliveredFrameId state. */
	uint64 LastDeliveredFrameId = 0;
	uint64 LastSharedHandle = 0; // track handle value to detect texture recreation
};

