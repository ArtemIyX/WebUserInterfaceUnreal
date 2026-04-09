// Fill out your copyright notice in the Description page of Project Settings.


#include "Services/CefInputWriter.h"

// Fill out your copyright notice in the Description page of Project Settings.

#include "Services/CefInputWriter.h"
#include "CefWebUi.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include <atomic>
#include "Windows/HideWindowsPlatformTypes.h"

// ---- Mirror of SharedMemoryLayout.h (keep in sync) -------------------------

static constexpr uint32 INPUT_RING_CAPACITY = 256;
static constexpr const wchar_t* SHM_INPUT_NAME = L"CEFHost_Input";
static constexpr const wchar_t* EVT_INPUT_READY = L"CEFHost_InputReady";

#pragma pack(push, 1)

struct FCefInputWriter::FCefInputEvent
{
	ECefInputEventType Type;
	uint8 Reserved[3];

	union
	{
		struct
		{
			int32 X, Y;
			uint8 Button;
		} Mouse;

		struct
		{
			int32 X, Y;
			float DeltaX, DeltaY;
		} Scroll;

		struct
		{
			uint32 WindowsKeyCode;
			uint32 Modifiers;
		} Key;

		struct
		{
			uint16 Character;
		} Char;
	};
};

struct FInputRingBuffer
{
	std::atomic<uint32> WriteIndex;
	std::atomic<uint32> ReadIndex;
	uint32 Capacity;
	uint32 Reserved;
	FCefInputWriter::FCefInputEvent Events[INPUT_RING_CAPACITY];
};

#pragma pack(pop)

// -----------------------------------------------------------------------------

FCefInputWriter::FCefInputWriter()
{
}

FCefInputWriter::~FCefInputWriter()
{
	Close();
}

bool FCefInputWriter::Open()
{
	HMap = OpenFileMappingW(FILE_MAP_READ | FILE_MAP_WRITE, Windows::FALSE, SHM_INPUT_NAME);
	if (!HMap)
	{
		UE_LOG(LogCefWebUi, Warning, TEXT("FCefInputWriter: Input shared memory not available yet."));
		return false;
	}

	PData = MapViewOfFile(HMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(FInputRingBuffer));
	if (!PData)
	{
		UE_LOG(LogCefWebUi, Error, TEXT("FCefInputWriter: MapViewOfFile failed."));
		CloseHandles();
		return false;
	}

	HEvent = OpenEventW(SYNCHRONIZE | EVENT_MODIFY_STATE, Windows::FALSE, EVT_INPUT_READY);
	if (!HEvent)
	{
		UE_LOG(LogCefWebUi, Error, TEXT("FCefInputWriter: OpenEvent failed."));
		CloseHandles();
		return false;
	}

	UE_LOG(LogCefWebUi, Log, TEXT("FCefInputWriter: Opened."));
	return true;
}

void FCefInputWriter::Close()
{
	CloseHandles();
}

void FCefInputWriter::WriteEvent(const FCefInputEvent& InEvent)
{
	if (!PData) return;

	FScopeLock lock(&WriteLock);

	FInputRingBuffer* ring = reinterpret_cast<FInputRingBuffer*>(PData);
	const uint32 w = ring->WriteIndex.load(std::memory_order_relaxed);
	const uint32 r = ring->ReadIndex.load(std::memory_order_acquire);

	// Drop event if ring is full.
	if (w - r >= INPUT_RING_CAPACITY)
	{
		UE_LOG(LogCefWebUi, Warning, TEXT("FCefInputWriter: Ring buffer full, dropping event."));
		return;
	}

	ring->Events[w % INPUT_RING_CAPACITY] = InEvent;
	ring->WriteIndex.store(w + 1, std::memory_order_release);

	SetEvent(HEvent);
}

// ---- Write API --------------------------------------------------------------

void FCefInputWriter::WriteMouseMove(int32 InX, int32 InY)
{
	FCefInputEvent evt{};
	evt.Type = ECefInputEventType::MouseMove;
	evt.Mouse.X = InX;
	evt.Mouse.Y = InY;
	WriteEvent(evt);
}

void FCefInputWriter::WriteMouseButton(int32 InX, int32 InY, ECefMouseButton InButton, bool bInIsUp)
{
	FCefInputEvent evt{};
	evt.Type = bInIsUp ? ECefInputEventType::MouseUp : ECefInputEventType::MouseDown;
	evt.Mouse.X = InX;
	evt.Mouse.Y = InY;
	evt.Mouse.Button = static_cast<uint8>(InButton);
	WriteEvent(evt);
}

void FCefInputWriter::WriteMouseScroll(int32 InX, int32 InY, float InDeltaX, float InDeltaY)
{
	FCefInputEvent evt{};
	evt.Type = ECefInputEventType::MouseScroll;
	evt.Scroll.X = InX;
	evt.Scroll.Y = InY;
	evt.Scroll.DeltaX = InDeltaX;
	evt.Scroll.DeltaY = InDeltaY;
	WriteEvent(evt);
}

void FCefInputWriter::WriteKey(uint32 InWindowsKeyCode, uint32 InModifiers, bool bInIsDown)
{
	FCefInputEvent evt{};
	evt.Type = bInIsDown ? ECefInputEventType::KeyDown : ECefInputEventType::KeyUp;
	evt.Key.WindowsKeyCode = InWindowsKeyCode;
	evt.Key.Modifiers = InModifiers;
	WriteEvent(evt);
}

void FCefInputWriter::WriteChar(uint16 InCharacter)

{
	FCefInputEvent evt{};
	evt.Type = ECefInputEventType::KeyChar;
	evt.Char.Character = InCharacter;
	WriteEvent(evt);
}

// ---- Internal ---------------------------------------------------------------

void FCefInputWriter::CloseHandles()
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
