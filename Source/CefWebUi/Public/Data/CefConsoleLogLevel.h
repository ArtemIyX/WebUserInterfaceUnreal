/**
 * @file CefWebUi\Public\Data\CefConsoleLogLevel.h
 * @brief Declares CefConsoleLogLevel for module CefWebUi\Public\Data\CefConsoleLogLevel.h.
 * @details Contains types and APIs used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "CefConsoleLogLevel.generated.h"

UENUM(BlueprintType)
/** @brief Type declaration. */
enum class ECefConsoleLogLevel : uint8
{
	Log = 0,
	Warning = 1,
	Error = 2,
};

