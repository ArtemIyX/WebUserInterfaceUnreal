/**
 * @file CefDispatch\Public\Dispatch\CefDispatchRegistration.h
 * @brief Declares CefDispatchRegistration for module CefDispatch\Public\Dispatch\CefDispatchRegistration.h.
 * @details Contains dispatch registry and value plumbing used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Dispatch/CefDispatchHandlerRegistry.h"
#include "Dispatch/CefDispatchRegistry.h"

/** @brief Type declaration. */
class CEFDISPATCH_API FCefDispatchFactoryRegistrar
{
public:
	FCefDispatchFactoryRegistrar(uint32 InMessageType, FCefDispatchRegistry::FCefDispatchFactory InFactory,
	                             /** @brief Function API. */
	                             bool bInAllowReplace = false);
};

/** @brief Type declaration. */
class CEFDISPATCH_API FCefDispatchHandlerRegistrar
{
public:
	FCefDispatchHandlerRegistrar(uint32 InMessageType, FCefDispatchHandlerRegistry::FCefDispatchHandler InHandler,
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

#define CEF_DISPATCH_REGISTER_HANDLER(InMessageType, InHandler)                                                     \
	namespace                                                                                                      \
	{                                                                                                              \
	static FCefDispatchHandlerRegistrar CEF_DISPATCH_CONCAT(GCefDispatchHandlerRegistrar_, __LINE__)(            \
		InMessageType, InHandler, false);                                                                         \
	}

#define CEF_DISPATCH_REGISTER_HANDLER_REPLACE(InMessageType, InHandler)                                             \
	namespace                                                                                                      \
	{                                                                                                              \
	static FCefDispatchHandlerRegistrar CEF_DISPATCH_CONCAT(GCefDispatchHandlerRegistrarReplace_, __LINE__)(     \
		InMessageType, InHandler, true);                                                                          \
	}

#define CEF_DISPATCH_REGISTER_TYPED_HANDLER(InMessageType, InValueType, InHandler)                                  \
	namespace                                                                                                      \
	{                                                                                                              \
	static FCefDispatchHandlerRegistrar CEF_DISPATCH_CONCAT(GCefDispatchTypedHandlerRegistrar_, __LINE__)(       \
		InMessageType,                                                                                             \
		FCefDispatchHandlerRegistry::MakeTypedHandler<InValueType>(InHandler),                                    \
		false);                                                                                                    \
	}

#define CEF_DISPATCH_REGISTER_TYPED_HANDLER_REPLACE(InMessageType, InValueType, InHandler)                          \
	namespace                                                                                                      \
	{                                                                                                              \
	static FCefDispatchHandlerRegistrar CEF_DISPATCH_CONCAT(GCefDispatchTypedHandlerRegistrarReplace_, __LINE__)(\
		InMessageType,                                                                                             \
		FCefDispatchHandlerRegistry::MakeTypedHandler<InValueType>(InHandler),                                    \
		true);                                                                                                     \
	}
