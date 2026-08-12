#include "Dispatch/CefDispatchRegistration.h"
#include "CefDispatch.h"

FCefDispatchFactoryRegistrar::FCefDispatchFactoryRegistrar(uint32 InMessageType,
                                                           FCefDispatchRegistry::FCefDispatchFactory InFactory,
                                                           bool bInAllowReplace)
{
	FCefDispatchModule::RegisterDeferredFactory(InMessageType, MoveTemp(InFactory), bInAllowReplace);
}
