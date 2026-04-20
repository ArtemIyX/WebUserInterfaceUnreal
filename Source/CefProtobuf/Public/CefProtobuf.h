/**
 * @file CefProtobuf\Public\CefProtobuf.h
 * @brief Declares CefProtobuf for module CefProtobuf\Public\CefProtobuf.h.
 * @details Contains types and APIs used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Protocol/CefWsBinaryCodec.h"
#include "Protocol/CefWsCodec.h"
#include "Protocol/CefWsEnvelope.h"

/** @brief Type declaration. */
class FCefProtobufModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

