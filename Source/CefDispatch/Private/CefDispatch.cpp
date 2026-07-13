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

struct FCefDeferredHandlerEntry
{
	uint32 MessageType = 0;
	FCefDispatchHandlerRegistry::FCefDispatchHandler Handler;
	bool bAllowReplace = false;
};

FCriticalSection GDeferredFactoriesLock;
TArray<FCefDeferredFactoryEntry> GDeferredFactories;
FCriticalSection GDeferredHandlersLock;
TArray<FCefDeferredHandlerEntry> GDeferredHandlers;
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
		if (module.Registry.IsValid())
		{
			module.Registry->RegisterFactory(InMessageType, MoveTemp(InFactory), bInAllowReplace);
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

void FCefDispatchModule::RegisterDeferredHandler(uint32 InMessageType, FCefDispatchHandlerRegistry::FCefDispatchHandler InHandler,
                                                 bool bInAllowReplace)
{
	if (IsAvailable())
	{
		FCefDispatchModule& module = Get();
		if (module.HandlerRegistry.IsValid())
		{
			module.HandlerRegistry->RegisterHandler(InMessageType, MoveTemp(InHandler), bInAllowReplace);
			return;
		}
	}

	FCefDeferredHandlerEntry deferredEntry;
	deferredEntry.MessageType = InMessageType;
	deferredEntry.Handler = MoveTemp(InHandler);
	deferredEntry.bAllowReplace = bInAllowReplace;

	FScopeLock lock(&GDeferredHandlersLock);
	GDeferredHandlers.Add(MoveTemp(deferredEntry));
}

void FCefDispatchModule::StartupModule()
{
	Registry = MakeShared<FCefDispatchRegistry>();
	HandlerRegistry = MakeShared<FCefDispatchHandlerRegistry>(Registry);

	TArray<FCefDeferredFactoryEntry> pendingEntries;
	{
		FScopeLock lock(&GDeferredFactoriesLock);
		pendingEntries = MoveTemp(GDeferredFactories);
		GDeferredFactories.Reset();
	}

	TArray<FCefDeferredHandlerEntry> pendingHandlers;
	{
		FScopeLock lock(&GDeferredHandlersLock);
		pendingHandlers = MoveTemp(GDeferredHandlers);
		GDeferredHandlers.Reset();
	}

	for (FCefDeferredFactoryEntry& entry : pendingEntries)
	{
		Registry->RegisterFactory(entry.MessageType, MoveTemp(entry.Factory), entry.bAllowReplace);
	}

	for (FCefDeferredHandlerEntry& entry : pendingHandlers)
	{
		HandlerRegistry->RegisterHandler(entry.MessageType, MoveTemp(entry.Handler), entry.bAllowReplace);
	}
}

void FCefDispatchModule::ShutdownModule()
{
	HandlerRegistry.Reset();
	Registry.Reset();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCefDispatchModule, CefDispatch)
