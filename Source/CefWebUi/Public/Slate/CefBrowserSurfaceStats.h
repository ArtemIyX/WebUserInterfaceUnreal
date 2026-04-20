/**
 * @file CefWebUi\Public\Slate\CefBrowserSurfaceStats.h
 * @brief Declares CefBrowserSurfaceStats for module CefWebUi\Public\Slate\CefBrowserSurfaceStats.h.
 * @details Contains telemetry and tuning configuration used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"

DECLARE_STATS_GROUP(TEXT("CefWebUiTelemetry"), STATGROUP_CefWebUiTelemetry, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("CefSlate: PollFrame"), STAT_CefSlate_PollFrame, STATGROUP_CefWebUiTelemetry);
DECLARE_CYCLE_STAT(TEXT("CefSlate: Draw"), STAT_CefSlate_Draw, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Consumed Frames"), STAT_CefTel_ConsumedFrames, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Frame Gaps"), STAT_CefTel_FrameGaps, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Input->Frame Latency Ms"), STAT_CefTel_InputToFrameLatencyMs, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Input->Frame Max Ms"), STAT_CefTel_InputToFrameLatencyMaxMs, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Fence Not Ready"), STAT_CefTel_FenceNotReady, STATGROUP_CefWebUiTelemetry);

