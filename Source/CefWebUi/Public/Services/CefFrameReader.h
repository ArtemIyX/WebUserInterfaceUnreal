// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/CefCustomCursorType.h"
#include "Data/CefLoadState.h"


namespace Windows
{
	typedef void* HANDLE;
}

struct FCefFrame
{
	TArray<uint8> Pixels;
	uint32 Width = 0;
	uint32 Height = 0;
	uint32 Sequence = 0;
	ECefCustomCursorType CursorType = ECefCustomCursorType::CT_NONE;
	ECefLoadState LoadState = ECefLoadState::Idle;
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
	void Stop();


	// FRunnable
	virtual uint32 Run() override;

public:
	/**
	 * @brief Call from game thread. Returns true if a new frame was available.
	 * @param OutFrame Output frame data.
	 */
	bool PollFrame(FCefFrame& OutFrame);

	static EMouseCursor::Type MapCefCursor(ECefCustomCursorType Type);

public:
	FOnCefLoadStateChanged OnLoadStateChanged;

private:
	void CloseHandles();

private:
	ECefLoadState LastLoadState = ECefLoadState::Idle;
	FCefFrame PendingFrame;
	FCriticalSection PendingFrameLock;

	Windows::HANDLE HMap = nullptr;
	Windows::HANDLE HEvent = nullptr;
	void* PData = nullptr;

	FRunnableThread* Thread = nullptr;
	std::atomic<bool> bFramePending{false};
	std::atomic<bool> bRunning{false};
	uint32 LastSequence = 0;
};
