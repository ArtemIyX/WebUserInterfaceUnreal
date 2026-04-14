// CefWebUiModule.cpp
#include "CefWebUi.h"

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
	// Session owns runtime lifecycle.
}

void FCefWebUiModule::ShutdownModule()
{
	// Session teardown handles runtime shutdown.
}
