// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CefControlEventType.generated.h"

UENUM(Blueprintable, BlueprintType)
enum class ECefControlEventType : uint8
{
	GoBack = 0,
	GoForward = 1,
	StopLoad = 2,
	Reload = 3,
	SetURL = 4,
	SetPaused = 5,
	SetHidden = 6,
	SetFocus = 7,
	SetZoomLevel = 8,
	SetFrameRate = 9,
	ScrollTo = 10,
	Resize = 11,
	SetMuted = 12,
	OpenDevTools = 13,
	CloseDevTools = 14,
	SetInputEnabled = 15,
	ExecuteJS = 16,
	ClearCookies = 17,
};
