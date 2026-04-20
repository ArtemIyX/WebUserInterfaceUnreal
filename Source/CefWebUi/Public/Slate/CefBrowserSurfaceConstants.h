/**
 * @file CefWebUi\Public\Slate\CefBrowserSurfaceConstants.h
 * @brief Declares CefBrowserSurfaceConstants for module CefWebUi\Public\Slate\CefBrowserSurfaceConstants.h.
 * @details Contains types and APIs used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"

namespace CefWebUi::BrowserSurface
{
/** @brief EnableCadenceFeedback state. */
inline constexpr bool EnableCadenceFeedback = false;
/** @brief TelemetryLogPeriodSec state. */
inline constexpr double TelemetryLogPeriodSec = 2.0;
/** @brief HostTuningPushPeriodSec state. */
inline constexpr double HostTuningPushPeriodSec = 0.5;
/** @brief HostMaxInFlightBeginFrames state. */
inline constexpr uint32 HostMaxInFlightBeginFrames = 0;
/** @brief HostFlushIntervalFrames state. */
inline constexpr uint32 HostFlushIntervalFrames = 1;
/** @brief HostKeyframeIntervalUs state. */
inline constexpr uint32 HostKeyframeIntervalUs = 0;
/** @brief HostFrameRate state. */
inline constexpr uint32 HostFrameRate = 60;
/** @brief CursorUpdateRateHz state. */
inline constexpr double CursorUpdateRateHz = 120.0;
/** @brief ActiveRepaintPeriodSec state. */
inline constexpr float ActiveRepaintPeriodSec = 1.0f / 60.0f;
/** @brief UseGpuFenceCheck state. */
inline constexpr bool UseGpuFenceCheck = false;
/** @brief EnableAutoResizeToWidget state. */
inline constexpr bool EnableAutoResizeToWidget = true;
/** @brief AutoResizeMinDimensionPx state. */
inline constexpr int32 AutoResizeMinDimensionPx = 64;
/** @brief AutoResizeMinDeltaPx state. */
inline constexpr int32 AutoResizeMinDeltaPx = 2;
/** @brief AutoResizeThrottlePeriodSec state. */
inline constexpr double AutoResizeThrottlePeriodSec = 0.10;
/** @brief AutoResizeDebouncePeriodSec state. */
inline constexpr double AutoResizeDebouncePeriodSec = 0.20;
/** @brief AutoResizeApplyTimeoutSec state. */
inline constexpr double AutoResizeApplyTimeoutSec = 0.50;
/** @brief AutoResizeMaxRetries state. */
inline constexpr int32 AutoResizeMaxRetries = 2;

/** @brief CefEventFlagCapsLockOn state. */
inline constexpr uint32 CefEventFlagCapsLockOn = 1u << 0;
/** @brief CefEventFlagShiftDown state. */
inline constexpr uint32 CefEventFlagShiftDown = 1u << 1;
/** @brief CefEventFlagControlDown state. */
inline constexpr uint32 CefEventFlagControlDown = 1u << 2;
/** @brief CefEventFlagAltDown state. */
inline constexpr uint32 CefEventFlagAltDown = 1u << 3;
/** @brief CefEventFlagCommandDown state. */
inline constexpr uint32 CefEventFlagCommandDown = 1u << 7;
/** @brief CefEventFlagNumLockOn state. */
inline constexpr uint32 CefEventFlagNumLockOn = 1u << 8;
/** @brief CefEventFlagIsKeyPad state. */
inline constexpr uint32 CefEventFlagIsKeyPad = 1u << 9;
/** @brief CefEventFlagIsLeft state. */
inline constexpr uint32 CefEventFlagIsLeft = 1u << 10;
/** @brief CefEventFlagIsRight state. */
inline constexpr uint32 CefEventFlagIsRight = 1u << 11;
/** @brief CefEventFlagIsRepeat state. */
inline constexpr uint32 CefEventFlagIsRepeat = 1u << 13;
}

