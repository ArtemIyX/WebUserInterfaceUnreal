// CefWebUiModule.cpp
#include "CefWebUi.h"

#include "Services/CefWebUiRuntime.h"


IMPLEMENT_MODULE(FCefWebUiModule, CefWebUi)

DEFINE_LOG_CATEGORY(LogCefWebUi);
DEFINE_LOG_CATEGORY(LogCefWebUiTelemetry);

FCefWebUiModule& FCefWebUiModule::Get()
{
	return FModuleManager::LoadModuleChecked<FCefWebUiModule>("CefWebUi");
}

bool FCefWebUiModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded("CefWebUi");
}

void FCefWebUiModule::StartupModule()
{
	// Lazy by design: do not launch host/processes during module startup.
}

void FCefWebUiModule::ShutdownModule()
{
	if (Runtime.IsValid())
	{
		Runtime->Shutdown();
		Runtime.Reset();
	}
}

void FCefWebUiModule::EnsureRuntimeStarted() const
{
	if (!Runtime.IsValid())
	{
		Runtime = MakeUnique<FCefWebUiRuntime>();
	}
	Runtime->EnsureStarted();
}

TSharedRef<FCefFrameReader> FCefWebUiModule::GetFrameReaderRef() const
{
	EnsureRuntimeStarted();
	return Runtime->GetFrameReaderRef();
}

TWeakPtr<FCefFrameReader> FCefWebUiModule::GetFrameReaderPtr() const
{
	EnsureRuntimeStarted();
	return Runtime->GetFrameReaderPtr();
}

TSharedRef<FCefInputWriter> FCefWebUiModule::GetInputWriterRef() const
{
	EnsureRuntimeStarted();
	return Runtime->GetInputWriterRef();
}

TWeakPtr<FCefInputWriter> FCefWebUiModule::GeInputWriterPtr() const
{
	EnsureRuntimeStarted();
	return Runtime->GetInputWriterPtr();
}

TSharedRef<FCefControlWriter> FCefWebUiModule::GetControlWriterRef() const
{
	EnsureRuntimeStarted();
	return Runtime->GetControlWriterRef();
}

TWeakPtr<FCefControlWriter> FCefWebUiModule::GetControlWriterPtr() const
{
	EnsureRuntimeStarted();
	return Runtime->GetControlWriterPtr();
}
