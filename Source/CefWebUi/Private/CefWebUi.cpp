// CefWebUiModule.cpp
#include "CefWebUi.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

IMPLEMENT_MODULE(FCefWebUiModule, CefWebUi)

DEFINE_LOG_CATEGORY(LogCefWebUi);
DEFINE_LOG_CATEGORY(LogCefWebUiTelemetry);
DEFINE_LOG_CATEGORY(LogCefWebUiJsConsole);

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
	const TSharedPtr<IPlugin> plugin = IPluginManager::Get().FindPlugin(TEXT("CefWebUi"));
	if (!plugin.IsValid())
	{
		return;
	}

	const FString shaderDir = FPaths::Combine(plugin->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/CefWebUi"), shaderDir);
}

void FCefWebUiModule::ShutdownModule()
{
	// Session teardown handles runtime shutdown.
}
