/**
 * @file CefWebUi\Public\Services\CefControlWriter.h
 * @brief Declares CefControlWriter for module CefWebUi\Public\Services\CefControlWriter.h.
 * @details Contains IPC reader/writer primitives used by the plugin runtime and gameplay-facing systems.
 */
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/** @brief Type declaration. */
class CEFWEBUI_API FCefControlWriter
{
public:
	/** @brief FCefControlWriter API. */
	FCefControlWriter();
	/** @brief FCefControlWriter API. */
	virtual ~FCefControlWriter();

	/** @brief Open API. */
	bool Open();
	/** @brief Close API. */
	void Close();
	bool IsOpen() const { return PData != nullptr; }

#pragma region Write API

	/** @brief GoBack API. */
	void GoBack();
	/** @brief GoForward API. */
	void GoForward();
	/** @brief StopLoad API. */
	void StopLoad();
	/** @brief Reload API. */
	void Reload();
	/** @brief SetURL API. */
	void SetURL(const FString& InURL);

	/** @brief SetPaused API. */
	void SetPaused(bool bInPaused);
	/** @brief SetHidden API. */
	void SetHidden(bool bInHidden);
	/** @brief SetFocus API. */
	void SetFocus(bool bInFocus);
	/** @brief SetZoomLevel API. */
	void SetZoomLevel(float InLevel);
	/** @brief SetFrameRate API. */
	void SetFrameRate(uint32 InRate);
	/** @brief ScrollTo API. */
	void ScrollTo(int32 InX, int32 InY);
	/** @brief Resize API. */
	void Resize(uint32 InWidth, uint32 InHeight);

	/** @brief SetMuted API. */
	void SetMuted(bool bInMuted);

	/** @brief OpenDevTools API. */
	void OpenDevTools();
	/** @brief CloseDevTools API. */
	void CloseDevTools();

	/** @brief SetInputEnabled API. */
	void SetInputEnabled(bool bInEnabled);

	/** @brief ExecuteJS API. */
	void ExecuteJS(const FString& InScript);
	/** @brief OpenLocalFile API. */
	void OpenLocalFile(const FString& InLocalFilePath);
	/** @brief LoadHtmlString API. */
	void LoadHtmlString(const FString& InHtml);

	/** @brief ClearCookies API. */
	void ClearCookies();
	/** @brief SetConsumerCadenceUs API. */
	void SetConsumerCadenceUs(uint32 InCadenceUs);
	/** @brief SetMaxInFlightBeginFrames API. */
	void SetMaxInFlightBeginFrames(uint32 InMaxInFlight);
	/** @brief SetFlushIntervalFrames API. */
	void SetFlushIntervalFrames(uint32 InFlushIntervalFrames);
	/** @brief SetKeyframeIntervalUs API. */
	void SetKeyframeIntervalUs(uint32 InKeyframeIntervalUs);

#pragma endregion

public:
	struct FCefControlEvent;

private:
	/** @brief WriteEvent API. */
	void WriteEvent(const FCefControlEvent& InEvent);
	/** @brief CloseHandles API. */
	void CloseHandles();

	/** @brief HMap state. */
	Windows::HANDLE HMap = nullptr;
	/** @brief HEvent state. */
	Windows::HANDLE HEvent = nullptr;
	/** @brief PData state. */
	void* PData = nullptr;

	/** @brief WriteLock state. */
	FCriticalSection WriteLock;
};

