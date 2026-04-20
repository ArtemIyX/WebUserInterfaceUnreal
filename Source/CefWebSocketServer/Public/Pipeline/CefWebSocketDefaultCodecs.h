/**
 * @file CefWebSocketServer\Public\Pipeline\CefWebSocketDefaultCodecs.h
 * @brief Declares CefWebSocketDefaultCodecs for module CefWebSocketServer\Public\Pipeline\CefWebSocketDefaultCodecs.h.
 * @details Contains websocket server components used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Pipeline/ICefWebSocketPacketCodec.h"

/** @brief Type declaration. */
class CEFWEBSOCKETSERVER_API FCefWebSocketBinaryPassthroughCodec : public ICefWebSocketPacketCodec
{
public:
	virtual bool DecodeInbound(const FCefWebSocketInboundPacket& InPacket, FCefWebSocketInboundPacket& OutDecodedPacket, FString& OutError) override;
	virtual bool EncodeSendRequest(const FCefWebSocketSendRequest& InRequest, TArray<FCefWebSocketWritePacket>& OutWritePackets, FString& OutError) override;
	virtual ECefWebSocketPayloadFormat GetPayloadFormat() const override { return ECefWebSocketPayloadFormat::Binary; }
};

/** @brief Type declaration. */
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
	/** @brief PayloadFormat state. */
	ECefWebSocketPayloadFormat PayloadFormat = ECefWebSocketPayloadFormat::Utf8String;
};

