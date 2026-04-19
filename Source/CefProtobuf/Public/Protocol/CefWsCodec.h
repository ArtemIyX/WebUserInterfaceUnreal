#pragma once

#include "CoreMinimal.h"
#include "Protocol/CefWsEnvelope.h"

class CEFPROTOBUF_API ICefWsCodec
{
public:
	virtual ~ICefWsCodec() = default;

	virtual ECefProtoEncodeResult Encode(const FCefWsEnvelope& InEnvelope, TArray<uint8>& OutBytes) const = 0;
	virtual ECefProtoDecodeResult Decode(const TArray<uint8>& InBytes, FCefWsEnvelope& OutEnvelope) const = 0;
	virtual uint16 GetSchemaVersion() const = 0;
};
