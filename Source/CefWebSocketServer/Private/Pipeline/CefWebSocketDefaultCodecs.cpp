#include "Pipeline/CefWebSocketDefaultCodecs.h"
#include "Config/CefWebSocketCVars.h"

bool FCefWebSocketBinaryPassthroughCodec::DecodeInbound(const FCefWebSocketInboundPacket& InPacket,
                                                        FCefWebSocketInboundPacket& OutDecodedPacket,
                                                        FString& OutError)
{
	OutError.Reset();
	OutDecodedPacket = InPacket;
	return true;
}

bool FCefWebSocketBinaryPassthroughCodec::EncodeSendRequest(const FCefWebSocketSendRequest& InRequest,
                                                            TArray<FCefWebSocketWritePacket>& OutWritePackets,
                                                            FString& OutError)
{
	OutError.Reset();
	OutWritePackets.Reset();
	if (InRequest.BytesPayload.Num() == 0)
	{
		OutError = TEXT("Empty binary payload");
		return false;
	}

	if (InRequest.bBroadcast)
	{
		for (const int64 clientId : InRequest.TargetClientIds)
		{
			if (clientId == InRequest.ExcludedClientId)
			{
				continue;
			}

			FCefWebSocketWritePacket& writePacket = OutWritePackets.AddDefaulted_GetRef();
			writePacket.ClientId = clientId;
			writePacket.Payload = InRequest.BytesPayload;
			writePacket.bBinary = true;
		}
		return OutWritePackets.Num() > 0;
	}

	if (InRequest.TargetClientIds.Num() <= 0)
	{
		OutError = TEXT("No target clients for binary payload");
		return false;
	}

	for (const int64 clientId : InRequest.TargetClientIds)
	{
		FCefWebSocketWritePacket& writePacket = OutWritePackets.AddDefaulted_GetRef();
		writePacket.ClientId = clientId;
		writePacket.Payload = InRequest.BytesPayload;
		writePacket.bBinary = true;
	}
	return true;
}

bool FCefWebSocketUtf8StringCodec::DecodeInbound(const FCefWebSocketInboundPacket& InPacket,
                                                 FCefWebSocketInboundPacket& OutDecodedPacket,
                                                 FString& OutError)
{
	OutError.Reset();
	OutDecodedPacket = InPacket;
	if (InPacket.bBinary)
	{
		return true;
	}
	if (!CefWebSocketCVars::GetValidateUtf8())
	{
		return true;
	}

	if (InPacket.Payload.Num() <= 0)
	{
		OutError = TEXT("Empty text payload");
		return false;
	}

	FUTF8ToTCHAR converter(reinterpret_cast<const ANSICHAR*>(InPacket.Payload.GetData()), InPacket.Payload.Num());
	if (converter.Length() <= 0)
	{
		OutError = TEXT("Invalid UTF-8 payload");
		return false;
	}

	return true;
}

bool FCefWebSocketUtf8StringCodec::EncodeSendRequest(const FCefWebSocketSendRequest& InRequest,
                                                     TArray<FCefWebSocketWritePacket>& OutWritePackets,
                                                     FString& OutError)
{
	OutError.Reset();
	OutWritePackets.Reset();
	if (InRequest.TextPayload.IsEmpty())
	{
		OutError = TEXT("Empty text payload");
		return false;
	}

	FTCHARToUTF8 utf8(*InRequest.TextPayload);
	if (utf8.Length() <= 0)
	{
		OutError = TEXT("Invalid UTF-8 source payload");
		return false;
	}

	TArray<uint8> encoded;
	encoded.Append(reinterpret_cast<const uint8*>(utf8.Get()), utf8.Length());

	if (InRequest.bBroadcast)
	{
		for (const int64 clientId : InRequest.TargetClientIds)
		{
			if (clientId == InRequest.ExcludedClientId)
			{
				continue;
			}
			FCefWebSocketWritePacket& writePacket = OutWritePackets.AddDefaulted_GetRef();
			writePacket.ClientId = clientId;
			writePacket.Payload = encoded;
			writePacket.bBinary = false;
		}
		return OutWritePackets.Num() > 0;
	}

	if (InRequest.TargetClientIds.Num() <= 0)
	{
		OutError = TEXT("No target clients for text payload");
		return false;
	}

	for (const int64 clientId : InRequest.TargetClientIds)
	{
		FCefWebSocketWritePacket& writePacket = OutWritePackets.AddDefaulted_GetRef();
		writePacket.ClientId = clientId;
		writePacket.Payload = encoded;
		writePacket.bBinary = false;
	}
	return true;
}
