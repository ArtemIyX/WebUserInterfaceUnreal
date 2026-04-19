#pragma once

#include "CoreMinimal.h"
#include "Data/CefWebSocketEnums.h"

struct CEFWEBSOCKETSERVER_API FCefWebSocketInboundPacket
{
	int64 ClientId = 0;
	TArray<uint8> Payload;
	bool bBinary = true;
	FDateTime ReceivedAtUtc = FDateTime::UtcNow();
};

struct CEFWEBSOCKETSERVER_API FCefWebSocketSendRequest
{
	TArray<int64> TargetClientIds;
	bool bBroadcast = false;
	int64 ExcludedClientId = 0;
	ECefWebSocketPayloadFormat PayloadFormat = ECefWebSocketPayloadFormat::Binary;
	TArray<uint8> BytesPayload;
	FString TextPayload;
};

struct CEFWEBSOCKETSERVER_API FCefWebSocketWritePacket
{
	int64 ClientId = 0;
	TArray<uint8> Payload;
	bool bBinary = true;
};
