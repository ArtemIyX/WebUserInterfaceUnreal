#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Protocol/CefWsBinaryCodec.h"
#include "Protocol/CefWsCodec.h"
#include "Protocol/CefWsEnvelope.h"

class FCefProtobufModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
