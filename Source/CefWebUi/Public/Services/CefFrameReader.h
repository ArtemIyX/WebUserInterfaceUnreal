// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/CefCustomCursorType.h"
#include "Data/CefLoadState.h"

namespace Windows
{
	typedef void* HANDLE;
}

constexpr uint32 MAX_CEF_DIRTY_RECTS = 16;
constexpr uint32 CEF_SHM_PROTOCOL_V2 = 3;
constexpr uint32 CEF_SHM_MAX_SLOTS = 3;
constexpr uint32 CEF_SHM_PROTOCOL_MAGIC = 0x43454648; // 'CEFH'

enum ECefFrameFlags : uint32
{
	CefFrameFlag_None = 0,
	CefFrameFlag_FullFrame = 1u << 0,
	CefFrameFlag_DirtyOnly = 1u << 1,
	CefFrameFlag_Overflow = 1u << 2,
	CefFrameFlag_Resized = 1u << 3,
};

struct FCefDirtyRect
{
	int32 X = 0, Y = 0, W = 0, H = 0;
};

struct FCefSharedFrame
{
	uint32 Version = 0;
	uint32 SlotCount = 2;
	uint32 WriteSlot = 0;
	uint32 Width = 0;
	uint32 Height = 0;
	uint32 Sequence = 0;
	uint64 FrameId = 0;
	uint64 PresentId = 0;
	uint64 GpuFenceValue = 0;
	uint32 Flags = 0;
	ECefCustomCursorType CursorType = ECefCustomCursorType::CT_NONE;
	ECefLoadState LoadState = ECefLoadState::Idle;
	bool bForceFullRefresh = true;
	uint8 DirtyCount = 0; // 0 = full frame
	FCefDirtyRect DirtyRects[MAX_CEF_DIRTY_RECTS];
};
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCefLoadStateChanged, uint8);

class CEFWEBUI_API FCefFrameReader : public FRunnable
{
public:
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
	bool PollSharedTexture(FCefSharedFrame& OutFrame);

	static EMouseCursor::Type MapCefCursor(ECefCustomCursorType Type);

public:
	FOnCefLoadStateChanged OnLoadStateChanged;

private:
	void CloseHandles();

private:
	ECefLoadState LastLoadState = ECefLoadState::Idle;

	FCefSharedFrame PendingFrame;
	FCriticalSection PendingFrameLock;

	Windows::HANDLE HMap = nullptr;
	Windows::HANDLE HEvent = nullptr;
	void* PData = nullptr;

	FRunnableThread* Thread = nullptr;
	std::atomic<bool> bFramePending{ false };
	std::atomic<bool> bRunning{ false };
	uint32 LastSequence = 0;
	uint64 LastFrameId = 0;
	uint64 LastDeliveredFrameId = 0;
	uint64 LastSharedHandle = 0; // track handle value to detect texture recreation
};
