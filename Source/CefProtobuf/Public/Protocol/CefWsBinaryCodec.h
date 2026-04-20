/**
 * @file CefProtobuf\Public\Protocol\CefWsBinaryCodec.h
 * @brief Declares CefWsBinaryCodec for module CefProtobuf\Public\Protocol\CefWsBinaryCodec.h.
 * @details Contains message codec and envelope definitions used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Protocol/CefWsCodec.h"

/** @brief Type declaration. */
class CEFPROTOBUF_API FCefWsBinaryCodec final : public ICefWsCodec
{
public:
	/** @brief HeaderSize state. */
	static constexpr uint32 HeaderSize = 24;
	static constexpr uint32 EnvelopeMagic = 0x43575342u; // "CWSB"

	/** @brief FCefWsBinaryCodec API. */
	explicit FCefWsBinaryCodec(uint16 InSchemaVersion = 1, uint32 InMaxPayloadBytes = 16u * 1024u * 1024u);

	virtual ECefProtoEncodeResult Encode(const FCefWsEnvelope& InEnvelope, TArray<uint8>& OutBytes) const override;
	virtual ECefProtoDecodeResult Decode(const TArray<uint8>& InBytes, FCefWsEnvelope& OutEnvelope) const override;
	virtual uint16 GetSchemaVersion() const override;

private:
	/** @brief SchemaVersion state. */
	uint16 SchemaVersion = 1;
	/** @brief MaxPayloadBytes state. */
	uint32 MaxPayloadBytes = 16u * 1024u * 1024u;
};

