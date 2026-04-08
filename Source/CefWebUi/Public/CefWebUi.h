// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Services/CefControlWriter.h"


class FCefInputWriter;
class FCefFrameReader;
class FCefControlWriter;
CEFWEBUI_API DECLARE_LOG_CATEGORY_EXTERN(LogCefWebUi, Log, All);

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
	TSharedPtr<FCefFrameReader> FrameReader;
	TSharedPtr<FCefInputWriter> InputWriter;
	TSharedPtr<FCefControlWriter> ControlWriter;

private:
	void LaunchHostProcess();
	void KillHostProcess();

private:
	FProcHandle HostProcess;
	Windows::HANDLE JobHandle = nullptr;
};
