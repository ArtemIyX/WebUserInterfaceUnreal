/**
 * @file CefProtobuf\Public\Protocol\CefWsEnvelope.h
 * @brief Declares CefWsEnvelope for module CefProtobuf\Public\Protocol\CefWsEnvelope.h.
 * @details Contains message codec and envelope definitions used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"

/** @brief Type declaration. */
enum class ECefProtoEncodeResult : uint8
{
	Ok = 0,
	InvalidInput,
	UnsupportedVersion,
	SerializeFailed,
};

/** @brief Type declaration. */
enum class ECefProtoDecodeResult : uint8
{
	Ok = 0,
	InvalidInput,
	UnsupportedVersion,
	ParseFailed,
};

/** @brief Type declaration. */
struct CEFPROTOBUF_API FCefWsEnvelope
{
	/** @brief MessageType state. */
	uint32 MessageType = 0;
	/** @brief SchemaVersion state. */
	uint16 SchemaVersion = 1;
	/** @brief Flags state. */
	uint16 Flags = 0;
	/** @brief RequestId state. */
	uint64 RequestId = 0;
	/** @brief Payload state. */
	TArray<uint8> Payload;
};

