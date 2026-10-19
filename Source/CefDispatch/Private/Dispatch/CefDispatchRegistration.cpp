#include "Dispatch/CefDispatchRegistration.h"
#include "CefDispatch.h"

FCefDispatchFactoryRegistrar::FCefDispatchFactoryRegistrar(FCefDispatchRouteKey InRouteKey,
                                                           FCefDispatchRegistry::FCefDispatchFactory InFactory,
                                                           bool bInAllowReplace)
{
	FCefDispatchModule::RegisterDeferredFactory(MoveTemp(InRouteKey), MoveTemp(InFactory), bInAllowReplace);
}
