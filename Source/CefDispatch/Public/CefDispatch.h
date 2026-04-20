/**
 * @file CefDispatch\Public\CefDispatch.h
 * @brief Declares CefDispatch for module CefDispatch\Public\CefDispatch.h.
 * @details Contains dispatch registry and value plumbing used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Dispatch/CefDispatchRegistry.h"
#include "Dispatch/CefDispatchRegistration.h"
#include "Dispatch/CefDispatchValue.h"

/** @brief Type declaration. */
class FCefDispatchModule : public IModuleInterface
{
public:
	/** @brief Get API. */
	static FCefDispatchModule& Get();
	/** @brief IsAvailable API. */
	static bool IsAvailable();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	TSharedPtr<class FCefDispatchRegistry> GetRegistry() const { return Registry; }
	static void RegisterDeferredFactory(uint32 InMessageType, FCefDispatchRegistry::FCefDispatchFactory InFactory,
	                                    /** @brief Function API. */
	                                    bool bInAllowReplace = false);

private:
	/** @brief Registry state. */
	TSharedPtr<class FCefDispatchRegistry> Registry;
};

