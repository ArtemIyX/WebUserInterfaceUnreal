#include "Dispatch/CefDispatchRegistration.h"
#include "CefDispatch.h"

FCefDispatchFactoryRegistrar::FCefDispatchFactoryRegistrar(uint32 InMessageType,
                                                           FCefDispatchRegistry::FCefDispatchFactory InFactory,
                                                           bool bInAllowReplace)
{
	FCefDispatchModule::RegisterDeferredFactory(InMessageType, MoveTemp(InFactory), bInAllowReplace);
}

FCefDispatchHandlerRegistrar::FCefDispatchHandlerRegistrar(uint32 InMessageType,
                                                           FCefDispatchHandlerRegistry::FCefDispatchHandler InHandler,
                                                           bool bInAllowReplace)
{
	FCefDispatchModule::RegisterDeferredHandler(InMessageType, MoveTemp(InHandler), bInAllowReplace);
}
