#pragma once

#include "CoreMinimal.h"
#include "Pipeline/ICefWebSocketPacketCodec.h"

class CEFWEBSOCKETSERVER_API FCefWebSocketBinaryPassthroughCodec : public ICefWebSocketPacketCodec
{
public:
	virtual bool DecodeInbound(const FCefWebSocketInboundPacket& InPacket, FCefWebSocketInboundPacket& OutDecodedPacket, FString& OutError) override;
	virtual bool EncodeSendRequest(const FCefWebSocketSendRequest& InRequest, TArray<FCefWebSocketWritePacket>& OutWritePackets, FString& OutError) override;
	virtual ECefWebSocketPayloadFormat GetPayloadFormat() const override { return ECefWebSocketPayloadFormat::Binary; }
};

class CEFWEBSOCKETSERVER_API FCefWebSocketUtf8StringCodec : public ICefWebSocketPacketCodec
{
public:
	explicit FCefWebSocketUtf8StringCodec(ECefWebSocketPayloadFormat InPayloadFormat = ECefWebSocketPayloadFormat::Utf8String)
		: PayloadFormat(InPayloadFormat)
	{
	}

	virtual bool DecodeInbound(const FCefWebSocketInboundPacket& InPacket, FCefWebSocketInboundPacket& OutDecodedPacket, FString& OutError) override;
	virtual bool EncodeSendRequest(const FCefWebSocketSendRequest& InRequest, TArray<FCefWebSocketWritePacket>& OutWritePackets, FString& OutError) override;
	virtual ECefWebSocketPayloadFormat GetPayloadFormat() const override { return PayloadFormat; }

private:
	ECefWebSocketPayloadFormat PayloadFormat = ECefWebSocketPayloadFormat::Utf8String;
};
