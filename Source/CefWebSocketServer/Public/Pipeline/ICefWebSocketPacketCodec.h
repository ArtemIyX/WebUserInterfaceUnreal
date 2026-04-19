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
