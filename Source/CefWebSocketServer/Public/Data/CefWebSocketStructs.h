#pragma once

#include "CoreMinimal.h"
#include "Data/CefWebSocketEnums.h"
#include "CefWebSocketStructs.generated.h"

USTRUCT(BlueprintType)
struct CEFWEBSOCKETSERVER_API FCefWebSocketServerCreateOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CefWebSocket")
	FName NameId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CefWebSocket")
	int32 RequestedPort = 0;
};

USTRUCT(BlueprintType)
struct CEFWEBSOCKETSERVER_API FCefWebSocketServerCreateResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket")
	ECefWebSocketCreateResult Result = ECefWebSocketCreateResult::Failed;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket")
	int32 BoundPort = 0;
};

USTRUCT(BlueprintType)
struct CEFWEBSOCKETSERVER_API FCefWebSocketClientInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket")
	int64 ClientId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket")
	FString RemoteAddress;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket")
	FDateTime ConnectedAt = FDateTime::MinValue();
};

USTRUCT(BlueprintType)
struct CEFWEBSOCKETSERVER_API FCefWebSocketServerStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	int32 ActiveClients = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	int64 RxBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	int64 TxBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	int64 DroppedMessages = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	int64 QueueDepth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	float RxBytesPerSec = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	float TxBytesPerSec = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	float AvgQueueDepth = 0.0f;
};
