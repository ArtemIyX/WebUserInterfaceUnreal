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
	void SetURL(const FString& URL);

	void SetPaused(bool bPaused);
	void SetHidden(bool bHidden);
	void SetFocus(bool bFocus);
	void SetZoomLevel(float Level);
	void SetFrameRate(uint32 Rate);
	void ScrollTo(int32 X, int32 Y);
	void Resize(uint32 Width, uint32 Height);

	void SetMuted(bool bMuted);

	void OpenDevTools();
	void CloseDevTools();

	void SetInputEnabled(bool bEnabled);

	void ExecuteJS(const FString& Script);

	void ClearCookies();

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
