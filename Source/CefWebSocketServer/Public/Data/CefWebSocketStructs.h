/**
 * @file CefWebSocketServer\Public\Data\CefWebSocketStructs.h
 * @brief Declares CefWebSocketStructs for module CefWebSocketServer\Public\Data\CefWebSocketStructs.h.
 * @details Contains websocket server components used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Data/CefWebSocketEnums.h"
#include "CefWebSocketStructs.generated.h"

USTRUCT(BlueprintType)
/** @brief Type declaration. */
struct CEFWEBSOCKETSERVER_API FCefWebSocketPipelineConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CefWebSocket|Pipeline")
	/** @brief InInboundQueueMax state. */
	int32 InInboundQueueMax = 2048;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CefWebSocket|Pipeline")
	/** @brief InSendQueueMax state. */
	int32 InSendQueueMax = 2048;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CefWebSocket|Pipeline")
	/** @brief InWriteQueueMaxPerClient state. */
	int32 InWriteQueueMaxPerClient = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CefWebSocket|Pipeline")
	/** @brief InPayloadFormat state. */
	ECefWebSocketPayloadFormat InPayloadFormat = ECefWebSocketPayloadFormat::Binary;
};

USTRUCT(BlueprintType)
/** @brief Type declaration. */
struct CEFWEBSOCKETSERVER_API FCefWebSocketServerCreateOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CefWebSocket")
	/** @brief NameId state. */
	FName NameId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CefWebSocket")
	/** @brief RequestedPort state. */
	int32 RequestedPort = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CefWebSocket|Pipeline")
	/** @brief InPipelineConfig state. */
	FCefWebSocketPipelineConfig InPipelineConfig;
};

USTRUCT(BlueprintType)
/** @brief Type declaration. */
struct CEFWEBSOCKETSERVER_API FCefWebSocketServerCreateResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket")
	/** @brief Result state. */
	ECefWebSocketCreateResult Result = ECefWebSocketCreateResult::Failed;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket")
	/** @brief BoundPort state. */
	int32 BoundPort = 0;
};

USTRUCT(BlueprintType)
/** @brief Type declaration. */
struct CEFWEBSOCKETSERVER_API FCefWebSocketClientInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket")
	/** @brief ClientId state. */
	int64 ClientId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket")
	/** @brief RemoteAddress state. */
	FString RemoteAddress;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket")
	/** @brief FDateTime::MinValue API. */
	FDateTime ConnectedAt = FDateTime::MinValue();
};

USTRUCT(BlueprintType)
/** @brief Type declaration. */
struct CEFWEBSOCKETSERVER_API FCefWebSocketServerStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	/** @brief ActiveClients state. */
	int32 ActiveClients = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	/** @brief RxBytes state. */
	int64 RxBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	/** @brief TxBytes state. */
	int64 TxBytes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	/** @brief DroppedMessages state. */
	int64 DroppedMessages = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	/** @brief QueueDepth state. */
	int64 QueueDepth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	/** @brief RxBytesPerSec state. */
	float RxBytesPerSec = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	/** @brief TxBytesPerSec state. */
	float TxBytesPerSec = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	/** @brief AvgQueueDepth state. */
	float AvgQueueDepth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	/** @brief InInboundQueueDepth state. */
	int64 InInboundQueueDepth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	/** @brief InHandleQueueDepth state. */
	int64 InHandleQueueDepth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	/** @brief InSendQueueDepth state. */
	int64 InSendQueueDepth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "CefWebSocket|Stats")
	/** @brief InWriteQueueDepth state. */
	int64 InWriteQueueDepth = 0;
};

