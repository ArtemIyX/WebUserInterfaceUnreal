// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"


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
	uint8 CursorType = 0;
};

enum class ECefCustomCursorType : uint8
{
	CT_POINTER,
	CT_CROSS,
	CT_HAND,
	CT_IBEAM,
	CT_WAIT,
	CT_HELP,
	CT_EASTRESIZE,
	CT_NORTHRESIZE,
	CT_NORTHEASTRESIZE,
	CT_NORTHWESTRESIZE,
	CT_SOUTHRESIZE,
	CT_SOUTHEASTRESIZE,
	CT_SOUTHWESTRESIZE,
	CT_WESTRESIZE,
	CT_NORTHSOUTHRESIZE,
	CT_EASTWESTRESIZE,
	CT_NORTHEASTSOUTHWESTRESIZE,
	CT_NORTHWESTSOUTHEASTRESIZE,
	CT_COLUMNRESIZE,
	CT_ROWRESIZE,
	CT_MIDDLEPANNING,
	CT_EASTPANNING,
	CT_NORTHPANNING,
	CT_NORTHEASTPANNING,
	CT_NORTHWESTPANNING,
	CT_SOUTHPANNING,
	CT_SOUTHEASTPANNING,
	CT_SOUTHWESTPANNING,
	CT_WESTPANNING,
	CT_MOVE,
	CT_VERTICALTEXT,
	CT_CELL,
	CT_CONTEXTMENU,
	CT_ALIAS,
	CT_PROGRESS,
	CT_NODROP,
	CT_COPY,
	CT_NONE,
	CT_NOTALLOWED,
	CT_ZOOMIN,
	CT_ZOOMOUT,
	CT_GRAB,
	CT_GRABBING,
	CT_MIDDLE_PANNING_VERTICAL,
	CT_MIDDLE_PANNING_HORIZONTAL,
	CT_CUSTOM,
	CT_DND_NONE,
	CT_DND_MOVE,
	CT_DND_COPY,
	CT_DND_LINK,
	CT_NUM_VALUES,
};

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
private:
	void CloseHandles();

private:
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
