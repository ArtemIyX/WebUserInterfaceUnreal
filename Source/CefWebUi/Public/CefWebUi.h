/**
 * @file CefWebUi\Public\CefWebUi.h
 * @brief Declares CefWebUi for module CefWebUi\Public\CefWebUi.h.
 * @details Contains types and APIs used by the plugin runtime and gameplay-facing systems.
 */
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
/** @brief DECLARE_LOG_CATEGORY_EXTERN API. */
CEFWEBUI_API DECLARE_LOG_CATEGORY_EXTERN(LogCefWebUi, Log, All);
/** @brief DECLARE_LOG_CATEGORY_EXTERN API. */
CEFWEBUI_API DECLARE_LOG_CATEGORY_EXTERN(LogCefWebUiTelemetry, Log, All);
/** @brief DECLARE_LOG_CATEGORY_EXTERN API. */
CEFWEBUI_API DECLARE_LOG_CATEGORY_EXTERN(LogCefWebUiJsConsole, Log, All);

/** @brief Type declaration. */
class FCefWebUiModule : public IModuleInterface
{
public:
	/** @brief Get API. */
	static FCefWebUiModule& Get();
	/** @brief IsAvailable API. */
	static bool IsAvailable();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

