// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"


class FCefInputWriter;
class FCefFrameReader;
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
	
	TSharedRef<FCefInputWriter> GetInputWriteRef() const;
	TWeakPtr<FCefInputWriter> GeInputWriterPtr() const;
private:
	TSharedPtr<FCefFrameReader> FrameReader;
	TSharedPtr<FCefInputWriter> InputWriter;
private:
	void LaunchHostProcess();
	void KillHostProcess();

private:
	FProcHandle HostProcess;
	Windows::HANDLE JobHandle = nullptr;
};
