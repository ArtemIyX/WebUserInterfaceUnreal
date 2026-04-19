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
			uint32 Value;
		} CadenceUs;

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

void FCefControlWriter::SetURL(const FString& InURL)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetURL;
	FMemory::Memzero(evt.String.Text, sizeof(evt.String.Text));
	FCString::Strncpy(reinterpret_cast<TCHAR*>(evt.String.Text), *InURL,
	                  FMath::Min(InURL.Len() + 1, static_cast<int32>(CONTROL_STRING_MAX)));
	WriteEvent(evt);
}

void FCefControlWriter::SetPaused(bool bInPaused)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetPaused;
	evt.Flag.Value = bInPaused;
	WriteEvent(evt);
}

void FCefControlWriter::SetHidden(bool bInHidden)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetHidden;
	evt.Flag.Value = bInHidden;
	WriteEvent(evt);
}

void FCefControlWriter::SetFocus(bool bInFocus)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetFocus;
	evt.Flag.Value = bInFocus;
	WriteEvent(evt);
}

void FCefControlWriter::SetZoomLevel(float InLevel)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetZoomLevel;
	evt.Zoom.Value = InLevel;
	WriteEvent(evt);
}

void FCefControlWriter::SetFrameRate(uint32 InRate)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetFrameRate;
	evt.FrameRate.Value = InRate;
	WriteEvent(evt);
}

void FCefControlWriter::ScrollTo(int32 InX, int32 InY)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::ScrollTo;
	evt.Scroll.X = InX;
	evt.Scroll.Y = InY;
	WriteEvent(evt);
}

void FCefControlWriter::Resize(uint32 InWidth, uint32 InHeight)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::Resize;
	evt.Resize.Width = InWidth;
	evt.Resize.Height = InHeight;
	WriteEvent(evt);
}

void FCefControlWriter::SetMuted(bool bInMuted)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetMuted;
	evt.Flag.Value = bInMuted;
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

void FCefControlWriter::SetInputEnabled(bool bInEnabled)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetInputEnabled;
	evt.Flag.Value = bInEnabled;
	WriteEvent(evt);
}

void FCefControlWriter::ExecuteJS(const FString& InScript)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::ExecuteJS;
	FMemory::Memzero(evt.String.Text, sizeof(evt.String.Text));
	FCString::Strncpy(reinterpret_cast<TCHAR*>(evt.String.Text), *InScript,
	                  FMath::Min(InScript.Len() + 1, (int32)CONTROL_STRING_MAX));
	WriteEvent(evt);
}

void FCefControlWriter::ClearCookies()
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::ClearCookies;
	WriteEvent(evt);
}

void FCefControlWriter::OpenLocalFile(const FString& InLocalFilePath)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::OpenLocalFile;
	FMemory::Memzero(evt.String.Text, sizeof(evt.String.Text));
	FCString::Strncpy(reinterpret_cast<TCHAR*>(evt.String.Text), *InLocalFilePath,
	                  FMath::Min(InLocalFilePath.Len() + 1, (int32)CONTROL_STRING_MAX));
	WriteEvent(evt);
}

void FCefControlWriter::LoadHtmlString(const FString& InHtml)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::LoadHtmlString;
	FMemory::Memzero(evt.String.Text, sizeof(evt.String.Text));
	FCString::Strncpy(reinterpret_cast<TCHAR*>(evt.String.Text), *InHtml,
	                  FMath::Min(InHtml.Len() + 1, (int32)CONTROL_STRING_MAX));
	WriteEvent(evt);
}

void FCefControlWriter::SetConsumerCadenceUs(uint32 InCadenceUs)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetConsumerCadenceUs;
	evt.CadenceUs.Value = InCadenceUs;
	WriteEvent(evt);
}

void FCefControlWriter::SetMaxInFlightBeginFrames(uint32 InMaxInFlight)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetMaxInFlightBeginFrames;
	evt.FrameRate.Value = InMaxInFlight;
	WriteEvent(evt);
}

void FCefControlWriter::SetFlushIntervalFrames(uint32 InFlushIntervalFrames)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetFlushIntervalFrames;
	evt.FrameRate.Value = InFlushIntervalFrames;
	WriteEvent(evt);
}

void FCefControlWriter::SetKeyframeIntervalUs(uint32 InKeyframeIntervalUs)
{
	FCefControlEvent evt{};
	evt.Type = ECefControlEventType::SetKeyframeIntervalUs;
	evt.CadenceUs.Value = InKeyframeIntervalUs;
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
