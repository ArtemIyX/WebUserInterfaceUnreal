#include "Protocol/CefWsBinaryCodec.h"

namespace CefWsBinaryIo
{
	struct FByteWriter
	{
		explicit FByteWriter(TArray<uint8>& InBuffer)
			: Buffer(InBuffer)
		{
		}

		void WriteU8(uint8 InValue)
		{
			Buffer.Add(InValue);
		}

		void WriteU16LE(uint16 InValue)
		{
			WriteU8(static_cast<uint8>(InValue & 0xFFu));
			WriteU8(static_cast<uint8>((InValue >> 8) & 0xFFu));
		}

		void WriteU32LE(uint32 InValue)
		{
			WriteU8(static_cast<uint8>(InValue & 0xFFu));
			WriteU8(static_cast<uint8>((InValue >> 8) & 0xFFu));
			WriteU8(static_cast<uint8>((InValue >> 16) & 0xFFu));
			WriteU8(static_cast<uint8>((InValue >> 24) & 0xFFu));
		}

		void WriteU64LE(uint64 InValue)
		{
			for (int32 index = 0; index < 8; ++index)
			{
				WriteU8(static_cast<uint8>((InValue >> (index * 8)) & 0xFFull));
			}
		}

		void WriteBytes(const TArray<uint8>& InBytes)
		{
			if (!InBytes.IsEmpty())
			{
				Buffer.Append(InBytes);
			}
		}

		TArray<uint8>& Buffer;
	};

	struct FByteReader
	{
		explicit FByteReader(const TArray<uint8>& InBuffer)
			: Buffer(InBuffer)
		{
		}

		bool ReadU8(uint8& OutValue)
		{
			if (Offset + 1 > Buffer.Num())
			{
				return false;
			}
			OutValue = Buffer[Offset++];
			return true;
		}

		bool ReadU16LE(uint16& OutValue)
		{
			uint8 b0 = 0;
			uint8 b1 = 0;
			if (!ReadU8(b0) || !ReadU8(b1))
			{
				return false;
			}
			OutValue = static_cast<uint16>(b0) | (static_cast<uint16>(b1) << 8);
			return true;
		}

		bool ReadU32LE(uint32& OutValue)
		{
			uint8 b0 = 0;
			uint8 b1 = 0;
			uint8 b2 = 0;
			uint8 b3 = 0;
			if (!ReadU8(b0) || !ReadU8(b1) || !ReadU8(b2) || !ReadU8(b3))
			{
				return false;
			}
			OutValue = static_cast<uint32>(b0) |
				(static_cast<uint32>(b1) << 8) |
				(static_cast<uint32>(b2) << 16) |
				(static_cast<uint32>(b3) << 24);
			return true;
		}

		bool ReadU64LE(uint64& OutValue)
		{
			OutValue = 0;
			for (int32 index = 0; index < 8; ++index)
			{
				uint8 value = 0;
				if (!ReadU8(value))
				{
					return false;
				}
				OutValue |= (static_cast<uint64>(value) << (index * 8));
			}
			return true;
		}

		bool ReadBytes(int32 InCount, TArray<uint8>& OutBytes)
		{
			if (InCount < 0 || Offset + InCount > Buffer.Num())
			{
				return false;
			}

			OutBytes.SetNumUninitialized(InCount);
			if (InCount > 0)
			{
				FMemory::Memcpy(OutBytes.GetData(), Buffer.GetData() + Offset, InCount);
			}
			Offset += InCount;
			return true;
		}

		bool IsAtEnd() const
		{
			return Offset == Buffer.Num();
		}

		const TArray<uint8>& Buffer;
		int32 Offset = 0;
	};
}

FCefWsBinaryCodec::FCefWsBinaryCodec(uint16 InSchemaVersion, uint32 InMaxPayloadBytes)
	: SchemaVersion(InSchemaVersion)
	, MaxPayloadBytes(InMaxPayloadBytes)
{
}

ECefProtoEncodeResult FCefWsBinaryCodec::Encode(const FCefWsEnvelope& InEnvelope, TArray<uint8>& OutBytes) const
{
	if (InEnvelope.MessageType == 0)
	{
		return ECefProtoEncodeResult::InvalidInput;
	}

	if (InEnvelope.SchemaVersion != SchemaVersion)
	{
		return ECefProtoEncodeResult::UnsupportedVersion;
	}

	if (static_cast<uint32>(InEnvelope.Payload.Num()) > MaxPayloadBytes)
	{
		return ECefProtoEncodeResult::InvalidInput;
	}

	const uint32 payloadSize = static_cast<uint32>(InEnvelope.Payload.Num());
	OutBytes.Reset();
	OutBytes.Reserve(HeaderSize + InEnvelope.Payload.Num());

	CefWsBinaryIo::FByteWriter writer(OutBytes);
	writer.WriteU32LE(EnvelopeMagic);
	writer.WriteU16LE(InEnvelope.SchemaVersion);
	writer.WriteU16LE(InEnvelope.Flags);
	writer.WriteU32LE(InEnvelope.MessageType);
	writer.WriteU64LE(InEnvelope.RequestId);
	writer.WriteU32LE(payloadSize);
	writer.WriteBytes(InEnvelope.Payload);

	return ECefProtoEncodeResult::Ok;
}

ECefProtoDecodeResult FCefWsBinaryCodec::Decode(const TArray<uint8>& InBytes, FCefWsEnvelope& OutEnvelope) const
{
	if (InBytes.Num() < static_cast<int32>(HeaderSize))
	{
		return ECefProtoDecodeResult::ParseFailed;
	}

	CefWsBinaryIo::FByteReader reader(InBytes);

	uint32 magic = 0;
	uint16 version = 0;
	uint16 flags = 0;
	uint32 messageType = 0;
	uint64 requestId = 0;
	uint32 payloadSize = 0;
	if (!reader.ReadU32LE(magic) ||
		!reader.ReadU16LE(version) ||
		!reader.ReadU16LE(flags) ||
		!reader.ReadU32LE(messageType) ||
		!reader.ReadU64LE(requestId) ||
		!reader.ReadU32LE(payloadSize))
	{
		return ECefProtoDecodeResult::ParseFailed;
	}

	if (magic != EnvelopeMagic)
	{
		return ECefProtoDecodeResult::ParseFailed;
	}

	if (version != SchemaVersion)
	{
		return ECefProtoDecodeResult::UnsupportedVersion;
	}

	if (messageType == 0 || payloadSize > MaxPayloadBytes)
	{
		return ECefProtoDecodeResult::InvalidInput;
	}

	TArray<uint8> payload;
	if (!reader.ReadBytes(static_cast<int32>(payloadSize), payload) || !reader.IsAtEnd())
	{
		return ECefProtoDecodeResult::ParseFailed;
	}

	OutEnvelope.MessageType = messageType;
	OutEnvelope.SchemaVersion = version;
	OutEnvelope.Flags = flags;
	OutEnvelope.RequestId = requestId;
	OutEnvelope.Payload = MoveTemp(payload);

	return ECefProtoDecodeResult::Ok;
}

uint16 FCefWsBinaryCodec::GetSchemaVersion() const
{
	return SchemaVersion;
}
