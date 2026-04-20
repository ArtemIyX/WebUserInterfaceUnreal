/**
 * @file CefProtobuf\Public\Protocol\CefWsCodec.h
 * @brief Declares CefWsCodec for module CefProtobuf\Public\Protocol\CefWsCodec.h.
 * @details Contains message codec and envelope definitions used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Protocol/CefWsEnvelope.h"

/** @brief Type declaration. */
class CEFPROTOBUF_API ICefWsCodec
{
public:
	virtual ~ICefWsCodec() = default;

	/** @brief Encode API. */
	virtual ECefProtoEncodeResult Encode(const FCefWsEnvelope& InEnvelope, TArray<uint8>& OutBytes) const = 0;
	/** @brief Decode API. */
	virtual ECefProtoDecodeResult Decode(const TArray<uint8>& InBytes, FCefWsEnvelope& OutEnvelope) const = 0;
	/** @brief GetSchemaVersion API. */
	virtual uint16 GetSchemaVersion() const = 0;
};

