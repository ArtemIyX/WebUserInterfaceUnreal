#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Dispatch/CefDispatchRegistry.h"
#include "Dispatch/CefDispatchValue.h"

class FCefDispatchModule : public IModuleInterface
{
public:
	static FCefDispatchModule& Get();
	static bool IsAvailable();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	TSharedPtr<class FCefDispatchRegistry> GetRegistry() const { return Registry; }

private:
	TSharedPtr<class FCefDispatchRegistry> Registry;
};
