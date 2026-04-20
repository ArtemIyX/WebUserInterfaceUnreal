/**
 * @file CefWebUi\Public\CefWebUi.h
 * @brief Declares CefWebUi for module CefWebUi\Public\CefWebUi.h.
 * @details Contains types and APIs used by the plugin runtime and gameplay-facing systems.
 */
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
CEFWEBUI_API DECLARE_LOG_CATEGORY_EXTERN(LogCefWebUi, Log, All);
CEFWEBUI_API DECLARE_LOG_CATEGORY_EXTERN(LogCefWebUiTelemetry, Log, All);
CEFWEBUI_API DECLARE_LOG_CATEGORY_EXTERN(LogCefWebUiJsConsole, Log, All);

class FCefWebUiModule : public IModuleInterface
{
public:
	static FCefWebUiModule& Get();
	static bool IsAvailable();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

