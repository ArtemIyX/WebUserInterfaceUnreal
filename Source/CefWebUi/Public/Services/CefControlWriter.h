/**
 * @file CefWebUi\Public\Services\CefControlWriter.h
 * @brief Declares CefControlWriter for module CefWebUi\Public\Services\CefControlWriter.h.
 * @details Contains IPC reader/writer primitives used by the plugin runtime and gameplay-facing systems.
 */
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class CEFWEBUI_API FCefControlWriter
{
public:
	FCefControlWriter();
	virtual ~FCefControlWriter();

	bool Open();
	void Close();
	bool IsOpen() const { return PData != nullptr; }

#pragma region Write API

	void GoBack();
	void GoForward();
	void StopLoad();
	void Reload();
	void SetURL(const FString& InURL);

	void SetPaused(bool bInPaused);
	void SetHidden(bool bInHidden);
	void SetFocus(bool bInFocus);
	void SetZoomLevel(float InLevel);
	void SetFrameRate(uint32 InRate);
	void ScrollTo(int32 InX, int32 InY);
	void Resize(uint32 InWidth, uint32 InHeight);

	void SetMuted(bool bInMuted);

	void OpenDevTools();
	void CloseDevTools();

	void SetInputEnabled(bool bInEnabled);

	void ExecuteJS(const FString& InScript);
	void OpenLocalFile(const FString& InLocalFilePath);
	void LoadHtmlString(const FString& InHtml);

	void ClearCookies();
	void SetConsumerCadenceUs(uint32 InCadenceUs);
	void SetMaxInFlightBeginFrames(uint32 InMaxInFlight);
	void SetFlushIntervalFrames(uint32 InFlushIntervalFrames);
	void SetKeyframeIntervalUs(uint32 InKeyframeIntervalUs);

#pragma endregion

public:
	struct FCefControlEvent;

private:
	void WriteEvent(const FCefControlEvent& InEvent);
	void CloseHandles();

	Windows::HANDLE HMap = nullptr;
	Windows::HANDLE HEvent = nullptr;
	void* PData = nullptr;

	FCriticalSection WriteLock;
};

