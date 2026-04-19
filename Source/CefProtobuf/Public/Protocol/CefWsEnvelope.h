#pragma once

#include "CoreMinimal.h"

enum class ECefProtoEncodeResult : uint8
{
	Ok = 0,
	InvalidInput,
	UnsupportedVersion,
	SerializeFailed,
};

enum class ECefProtoDecodeResult : uint8
{
	Ok = 0,
	InvalidInput,
	UnsupportedVersion,
	ParseFailed,
};

struct CEFPROTOBUF_API FCefWsEnvelope
{
	uint32 MessageType = 0;
	uint16 SchemaVersion = 1;
	uint16 Flags = 0;
	uint64 RequestId = 0;
	TArray<uint8> Payload;
};
