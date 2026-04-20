/**
 * @file CefDispatch\Public\Dispatch\CefDispatchRegistration.h
 * @brief Declares CefDispatchRegistration for module CefDispatch\Public\Dispatch\CefDispatchRegistration.h.
 * @details Contains dispatch registry and value plumbing used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Dispatch/CefDispatchRegistry.h"

class CEFDISPATCH_API FCefDispatchFactoryRegistrar
{
public:
	FCefDispatchFactoryRegistrar(uint32 InMessageType, FCefDispatchRegistry::FCefDispatchFactory InFactory,
	                             bool bInAllowReplace = false);
};

#define CEF_DISPATCH_CONCAT_INNER(InA, InB) InA##InB
#define CEF_DISPATCH_CONCAT(InA, InB) CEF_DISPATCH_CONCAT_INNER(InA, InB)

#define CEF_DISPATCH_REGISTER_FACTORY(InMessageType, InFactory)                                                     \
	namespace                                                                                                      \
	{                                                                                                              \
	static FCefDispatchFactoryRegistrar CEF_DISPATCH_CONCAT(GCefDispatchFactoryRegistrar_, __LINE__)(            \
		InMessageType, InFactory, false);                                                                         \
	}

#define CEF_DISPATCH_REGISTER_FACTORY_REPLACE(InMessageType, InFactory)                                             \
	namespace                                                                                                      \
	{                                                                                                              \
	static FCefDispatchFactoryRegistrar CEF_DISPATCH_CONCAT(GCefDispatchFactoryRegistrarReplace_, __LINE__)(     \
		InMessageType, InFactory, true);                                                                          \
	}

