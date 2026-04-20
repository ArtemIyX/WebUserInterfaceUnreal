/**
 * @file CefWebUi\Public\Services\CefInputWriter.h
 * @brief Declares CefInputWriter for module CefWebUi\Public\Services\CefInputWriter.h.
 * @details Contains IPC reader/writer primitives used by the plugin runtime and gameplay-facing systems.
 */
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

namespace Windows
{
	typedef void* HANDLE;
}

// Mirror of SharedMemoryLayout.h — keep in sync with CEF side.
enum class ECefInputEventType : uint8
{
	MouseMove = 0,
	MouseDown = 1,
	MouseUp = 2,
	MouseScroll = 3,
	KeyDown = 4,
	KeyUp = 5,
	KeyChar = 6,
};

enum class ECefMouseButton : uint8
{
	Left = 0,
	Middle = 1,
	Right = 2,
};

class CEFWEBUI_API FCefInputWriter
{
public:
	FCefInputWriter();
	virtual ~FCefInputWriter();

	/**
		 * @brief Opens the shared memory input ring. Call after CEF Host.exe is running.
		 * @return False if shared memory not available yet.
		 */
	bool Open();

	/**
	 * @brief Closes shared memory handles.
	 */
	void Close();

	/** @return True if shared memory is open and ready. */
	bool IsOpen() const { return PData != nullptr; }

#pragma region Write API

	/**
	 * @brief Sends a mouse move event.
	 * @param InX X position in browser pixels.
	 * @param InY Y position in browser pixels.
	 */
	void WriteMouseMove(int32 InX, int32 InY);

	/**
	 * @brief Sends a mouse button press or release.
	 * @param InX X position in browser pixels.
	 * @param InY Y position in browser pixels.
	 * @param InButton Mouse button.
	 * @param bInIsUp True for release, false for press.
	 */
	void WriteMouseButton(int32 InX, int32 InY, ECefMouseButton InButton, bool bInIsUp);

	/**
	 * @brief Sends a mouse scroll event.
	 * @param InX X position in browser pixels.
	 * @param InY Y position in browser pixels.
	 * @param InDeltaX Horizontal scroll delta.
	 * @param InDeltaY Vertical scroll delta.
	 */
	void WriteMouseScroll(int32 InX, int32 InY, float InDeltaX, float InDeltaY);

	/**
	 * @brief Sends a key down or up event.
	 * @param InWindowsKeyCode Virtual-key code (VK_*).
	 * @param InModifiers CEF modifier flags.
	 * @param bInIsDown True for key down, false for key up.
	 */
	void WriteKey(uint32 InWindowsKeyCode, uint32 InModifiers, bool bInIsDown);

	/**
	 * @brief Sends a character input event (for text entry).
	 * @param InCharacter UTF-16 character code.
	 */
	void WriteChar(uint16 InCharacter);

#pragma endregion Write API

public:
	struct FCefInputEvent;

private:
	void WriteEvent(const FCefInputEvent& InEvent);
	void CloseHandles();

	Windows::HANDLE HMap = nullptr;
	Windows::HANDLE HEvent = nullptr;
	void* PData = nullptr;

	FCriticalSection WriteLock;
};

