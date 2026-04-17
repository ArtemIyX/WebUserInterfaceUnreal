#pragma once

#include "CoreMinimal.h"

namespace CefWebUi::BrowserSurface
{
inline constexpr bool EnableCadenceFeedback = false;
inline constexpr double TelemetryLogPeriodSec = 2.0;
inline constexpr double HostTuningPushPeriodSec = 0.5;
inline constexpr uint32 HostMaxInFlightBeginFrames = 0;
inline constexpr uint32 HostFlushIntervalFrames = 1;
inline constexpr uint32 HostKeyframeIntervalUs = 0;
inline constexpr uint32 HostFrameRate = 60;
inline constexpr double CursorUpdateRateHz = 120.0;
inline constexpr float ActiveRepaintPeriodSec = 1.0f / 60.0f;
inline constexpr bool UseGpuFenceCheck = false;

inline constexpr uint32 CefEventFlagCapsLockOn = 1u << 0;
inline constexpr uint32 CefEventFlagShiftDown = 1u << 1;
inline constexpr uint32 CefEventFlagControlDown = 1u << 2;
inline constexpr uint32 CefEventFlagAltDown = 1u << 3;
inline constexpr uint32 CefEventFlagCommandDown = 1u << 7;
inline constexpr uint32 CefEventFlagNumLockOn = 1u << 8;
inline constexpr uint32 CefEventFlagIsKeyPad = 1u << 9;
inline constexpr uint32 CefEventFlagIsLeft = 1u << 10;
inline constexpr uint32 CefEventFlagIsRight = 1u << 11;
inline constexpr uint32 CefEventFlagIsRepeat = 1u << 13;
}
