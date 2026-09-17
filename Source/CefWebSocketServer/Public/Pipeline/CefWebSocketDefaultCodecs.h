/**
 * @file CefWebSocketServer\Public\Pipeline\CefWebSocketDefaultCodecs.h
 * @brief Declares the built-in WebSocket payload codecs.
 */
#pragma once

#include "CoreMinimal.h"
#include "Pipeline/ICefWebSocketPacketCodec.h"

/** @brief Codec that forwards binary payloads without transformation. */
class CEFWEBSOCKETSERVER_API FCefWebSocketBinaryPassthroughCodec : public ICefWebSocketPacketCodec
{
public:
	virtual bool DecodeInbound(const FCefWebSocketInboundPacket& InPacket, FCefWebSocketInboundPacket& OutDecodedPacket, FString& OutError) override;
	virtual bool EncodeSendRequest(const FCefWebSocketSendRequest& InRequest, TArray<FCefWebSocketWritePacket>& OutWritePackets, FString& OutError) override;
	virtual ECefWebSocketPayloadFormat GetPayloadFormat() const override { return ECefWebSocketPayloadFormat::Binary; }
};

/** @brief Codec that converts payloads to and from UTF-8 text. */
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
	/** Payload format reported by this codec. */
	ECefWebSocketPayloadFormat PayloadFormat = ECefWebSocketPayloadFormat::Utf8String;
};

