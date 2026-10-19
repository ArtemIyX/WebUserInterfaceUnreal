/**
 * @file CefWebSocketServer\Public\Pipeline\ICefWebSocketPacketCodec.h
 * @brief Declares the interface for transforming WebSocket packets at the pipeline boundary.
 */
#pragma once

#include "CoreMinimal.h"
#include "Data/CefWebSocketEnums.h"
#include "Pipeline/CefWebSocketPipelineTypes.h"

/** @brief Encodes and decodes packets for a configured payload format. */
class CEFWEBSOCKETSERVER_API ICefWebSocketPacketCodec
{
public:
	virtual ~ICefWebSocketPacketCodec() = default;

	/** @brief Decodes an inbound packet. @param InPacket Received packet. @param OutDecodedPacket Decoded packet. @param OutError Receives a failure description. @return true when decoding succeeds. */
	virtual bool DecodeInbound(const FCefWebSocketInboundPacket& InPacket, FCefWebSocketInboundPacket& OutDecodedPacket, FString& OutError) = 0;
	/** @brief Encodes an outgoing send request into client write packets. @param InRequest Outgoing request. @param OutWritePackets Receives encoded packets. @param OutError Receives a failure description. @return true when encoding succeeds. */
	virtual bool EncodeSendRequest(const FCefWebSocketSendRequest& InRequest, TArray<FCefWebSocketWritePacket>& OutWritePackets, FString& OutError) = 0;
	/** @brief Returns the payload format handled by this codec. */
	virtual ECefWebSocketPayloadFormat GetPayloadFormat() const = 0;
};

