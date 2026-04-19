#include "CefDispatch.h"
#include "Dispatch/CefDispatchRegistry.h"

#define LOCTEXT_NAMESPACE "FCefDispatchModule"

FCefDispatchModule& FCefDispatchModule::Get()
{
	return FModuleManager::LoadModuleChecked<FCefDispatchModule>("CefDispatch");
}

bool FCefDispatchModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded("CefDispatch");
}

void FCefDispatchModule::StartupModule()
{
	Registry = MakeShared<FCefDispatchRegistry>();
}

void FCefDispatchModule::ShutdownModule()
{
	Registry.Reset();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCefDispatchModule, CefDispatch)
