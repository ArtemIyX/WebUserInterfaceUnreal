/**
 * @file CefWebSocketServer\Public\Pipeline\ICefWebSocketPacketCodec.h
 * @brief Declares ICefWebSocketPacketCodec for module CefWebSocketServer\Public\Pipeline\ICefWebSocketPacketCodec.h.
 * @details Contains interface contracts used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Data/CefWebSocketEnums.h"
#include "Pipeline/CefWebSocketPipelineTypes.h"

class CEFWEBSOCKETSERVER_API ICefWebSocketPacketCodec
{
public:
	virtual ~ICefWebSocketPacketCodec() = default;

	virtual bool DecodeInbound(const FCefWebSocketInboundPacket& InPacket, FCefWebSocketInboundPacket& OutDecodedPacket, FString& OutError) = 0;
	virtual bool EncodeSendRequest(const FCefWebSocketSendRequest& InRequest, TArray<FCefWebSocketWritePacket>& OutWritePackets, FString& OutError) = 0;
	virtual ECefWebSocketPayloadFormat GetPayloadFormat() const = 0;
};

