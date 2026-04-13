// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Services/CefControlWriter.h"
#include "Templates/UniquePtr.h"


class FCefInputWriter;
class FCefFrameReader;
class FCefControlWriter;
class FCefWebUiRuntime;
CEFWEBUI_API DECLARE_LOG_CATEGORY_EXTERN(LogCefWebUi, Log, All);
CEFWEBUI_API DECLARE_LOG_CATEGORY_EXTERN(LogCefWebUiTelemetry, Log, All);

class FCefWebUiModule : public IModuleInterface
{
public:
	static FCefWebUiModule& Get();
	static bool IsAvailable();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

public:
	TSharedRef<FCefFrameReader> GetFrameReaderRef() const;
	TWeakPtr<FCefFrameReader> GetFrameReaderPtr() const;

	TSharedRef<FCefInputWriter> GetInputWriterRef() const;
	TWeakPtr<FCefInputWriter> GeInputWriterPtr() const;

	TSharedRef<FCefControlWriter> GetControlWriterRef() const;
	TWeakPtr<FCefControlWriter> GetControlWriterPtr() const;

private:
	void EnsureRuntimeStarted() const;

private:
	mutable TUniquePtr<FCefWebUiRuntime> Runtime;
};
