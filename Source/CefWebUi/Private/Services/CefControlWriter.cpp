// Fill out your copyright notice in the Description page of Project Settings.


#include "Services/CefControlWriter.h"
#include "CefWebUi.h"

#include "Windows/AllowWindowsPlatformTypes.h"

#include <Windows.h>
#include <atomic>

#include "Data/CefControlEventType.h"

#include "Windows/HideWindowsPlatformTypes.h"

static constexpr uint32 CONTROL_RING_CAPACITY = 64;
static constexpr uint32 CONTROL_STRING_MAX = 2048;
static constexpr const wchar_t* SHM_CONTROL_NAME = L"CEFHost_Control";
static constexpr const wchar_t* EVT_CONTROL_READY = L"CEFHost_ControlReady";

#pragma pack(push, 1)

struct FCefControlWriter::FCefControlEvent
{
	ECefControlEventType Type;
	uint8 Reserved[3];

	union
	{
		struct
		{
			uint32 Width;
			uint32 Height;
		} Resize;

		struct
		{
			int32 X;
			int32 Y;
		} Scroll;

		struct
		{
			float Value;
		} Zoom;

		struct
		{
			uint32 Value;
		} FrameRate;

		struct
		{
			bool Value;
		} Flag;

		struct
		{
			char16_t Text[CONTROL_STRING_MAX];
		} String;
	};
};

struct FControlRingBuffer
{
	std::atomic<uint32> WriteIndex;
	std::atomic<uint32> ReadIndex;
	uint32 Capacity;
	uint32 Reserved;
	FCefControlWriter::FCefControlEvent Events[CONTROL_RING_CAPACITY];
};

#pragma pack(pop)

FCefControlWriter::FCefControlWriter()
{
}

FCefControlWriter::~FCefControlWriter()
{
	Close();
}

bool FCefControlWriter::Open()
{
	HMap = OpenFileMappingW(FILE_MAP_READ | FILE_MAP_WRITE, Windows::FALSE, SHM_CONTROL_NAME);
	if (!HMap)
	{
		UE_LOG(LogCefWebUi, Warning, TEXT("FCefControlWriter: Control shared memory not available yet."));
		return false;
	}

	PData = MapViewOfFile(HMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(FControlRingBuffer));
	if (!PData)
	{
		UE_LOG(LogCefWebUi, Error, TEXT("FCefControlWriter: MapViewOfFile failed."));
		CloseHandles();
		return false;
	}

	HEvent = OpenEventW(SYNCHRONIZE | EVENT_MODIFY_STATE, Windows::FALSE, EVT_CONTROL_READY);
	if (!HEvent)
	{
		UE_LOG(LogCefWebUi, Error, TEXT("FCefControlWriter: OpenEvent failed."));
		CloseHandles();
		return false;
	}

	UE_LOG(LogCefWebUi, Log, TEXT("FCefControlWriter: Opened."));
	return true;
}

void FCefControlWriter::Close()
{
	CloseHandles();
}

void FCefControlWriter::WriteEvent(const FCefControlEvent& InEvent)
{
	if (!PData) return;

	FScopeLock lock(&WriteLock);

	FControlRingBuffer* ring = reinterpret_cast<FControlRingBuffer*>(PData);
	const uint32 w = ring->WriteIndex.load(std::memory_order_relaxed);
	const uint32 r = ring->ReadIndex.load(std::memory_order_acquire);

	if (w - r >= CONTROL_RING_CAPACITY)
	{
		UE_LOG(LogCefWebUi, Warning, TEXT("FCefControlWriter: Ring buffer full, dropping event."));
		return;
	}

	ring->Events[w % CONTROL_RING_CAPACITY] = InEvent;
	ring->WriteIndex.store(w + 1, std::memory_order_release);

	SetEvent(HEvent);
}

// ---- Write API --------------------------------------------------------------

void FCefControlWriter::GoBack()
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::GoBack;
	WriteEvent(evt);
}

void FCefControlWriter::GoForward()
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::GoForward;
	WriteEvent(evt);
}

void FCefControlWriter::StopLoad()
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::StopLoad;
	WriteEvent(evt);
}

void FCefControlWriter::Reload()
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::Reload;
	WriteEvent(evt);
}

void FCefControlWriter::SetURL(const FString& URL)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetURL;
	FMemory::Memzero(evt.String.Text, sizeof(evt.String.Text));
	FCString::Strncpy(reinterpret_cast<TCHAR*>(evt.String.Text), *URL,
	                  FMath::Min(URL.Len() + 1, static_cast<int32>(CONTROL_STRING_MAX)));
	WriteEvent(evt);
}

void FCefControlWriter::SetPaused(bool bPaused)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetPaused;
	evt.Flag.Value = bPaused;
	WriteEvent(evt);
}

void FCefControlWriter::SetHidden(bool bHidden)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetHidden;
	evt.Flag.Value = bHidden;
	WriteEvent(evt);
}

void FCefControlWriter::SetFocus(bool bFocus)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetFocus;
	evt.Flag.Value = bFocus;
	WriteEvent(evt);
}

void FCefControlWriter::SetZoomLevel(float Level)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetZoomLevel;
	evt.Zoom.Value = Level;
	WriteEvent(evt);
}

void FCefControlWriter::SetFrameRate(uint32 Rate)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetFrameRate;
	evt.FrameRate.Value = Rate;
	WriteEvent(evt);
}

void FCefControlWriter::ScrollTo(int32 X, int32 Y)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::ScrollTo;
	evt.Scroll.X = X;
	evt.Scroll.Y = Y;
	WriteEvent(evt);
}

void FCefControlWriter::Resize(uint32 Width, uint32 Height)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::Resize;
	evt.Resize.Width = Width;
	evt.Resize.Height = Height;
	WriteEvent(evt);
}

void FCefControlWriter::SetMuted(bool bMuted)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetMuted;
	evt.Flag.Value = bMuted;
	WriteEvent(evt);
}

void FCefControlWriter::OpenDevTools()
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::OpenDevTools;
	WriteEvent(evt);
}

void FCefControlWriter::CloseDevTools()
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::CloseDevTools;
	WriteEvent(evt);
}

void FCefControlWriter::SetInputEnabled(bool bEnabled)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetInputEnabled;
	evt.Flag.Value = bEnabled;
	WriteEvent(evt);
}

void FCefControlWriter::ExecuteJS(const FString& Script)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::ExecuteJS;
	FMemory::Memzero(evt.String.Text, sizeof(evt.String.Text));
	FCString::Strncpy(reinterpret_cast<TCHAR*>(evt.String.Text), *Script,
	                  FMath::Min(Script.Len() + 1, (int32)CONTROL_STRING_MAX));
	WriteEvent(evt);
}

void FCefControlWriter::ClearCookies()
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::ClearCookies;
	WriteEvent(evt);
}

void FCefControlWriter::CloseHandles()
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
