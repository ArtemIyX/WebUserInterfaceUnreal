#include "CefDispatch.h"
#include "Dispatch/CefDispatchRegistry.h"

namespace
{
	struct FCefDeferredFactoryEntry
	{
		uint32 MessageType = 0;
		FCefDispatchRegistry::FCefDispatchFactory Factory;
		bool bAllowReplace = false;
	};

	FCriticalSection GDeferredFactoriesLock;
	TArray<FCefDeferredFactoryEntry> GDeferredFactories;
}

#define LOCTEXT_NAMESPACE "FCefDispatchModule"

FCefDispatchModule& FCefDispatchModule::Get()
{
	return FModuleManager::LoadModuleChecked<FCefDispatchModule>("CefDispatch");
}

bool FCefDispatchModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded("CefDispatch");
}

void FCefDispatchModule::RegisterDeferredFactory(uint32 InMessageType, FCefDispatchRegistry::FCefDispatchFactory InFactory,
	bool bInAllowReplace)
{
	if (IsAvailable())
	{
		FCefDispatchModule& module = Get();
		if (module.DispatchRegistry.IsValid())
		{
			module.DispatchRegistry->RegisterFactory(InMessageType, MoveTemp(InFactory), bInAllowReplace);
			return;
		}
	}

	FCefDeferredFactoryEntry deferredEntry;
	deferredEntry.MessageType = InMessageType;
	deferredEntry.Factory = MoveTemp(InFactory);
	deferredEntry.bAllowReplace = bInAllowReplace;

	FScopeLock lock(&GDeferredFactoriesLock);
	GDeferredFactories.Add(MoveTemp(deferredEntry));
}

void FCefDispatchModule::StartupModule()
{
	DispatchRegistry = MakeShared<FCefDispatchRegistry>();

	TArray<FCefDeferredFactoryEntry> pendingEntries;
	{
		FScopeLock lock(&GDeferredFactoriesLock);
		pendingEntries = MoveTemp(GDeferredFactories);
		GDeferredFactories.Reset();
	}

	for (FCefDeferredFactoryEntry& entry : pendingEntries)
	{
		DispatchRegistry->RegisterFactory(entry.MessageType, MoveTemp(entry.Factory), entry.bAllowReplace);
	}

}

void FCefDispatchModule::ShutdownModule()
{
	DispatchRegistry.Reset();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCefDispatchModule, CefDispatch)
