/**
 * @file CefContentHttpServer/Public/Handlers/CefContentHttpImageRequestHandler.h
 * @brief Base request handler types for /img route processing.
 */
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CefContentHttpImageRequestHandler.generated.h"

/** @brief Input payload used by image request handlers. */
USTRUCT(BlueprintType)
struct CEFCONTENTHTTPSERVER_API FCefContentHttpImageRequestContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CefContentHttpServer")
	FString AssetPath;

	UPROPERTY(BlueprintReadOnly, Category = "CefContentHttpServer")
	FString RawBody;

	UPROPERTY(BlueprintReadOnly, Category = "CefContentHttpServer")
	TMap<FString, FString> QueryParams;
};

/** @brief Output payload produced by image request handlers. */
USTRUCT(BlueprintType)
struct CEFCONTENTHTTPSERVER_API FCefContentHttpImageResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "CefContentHttpServer")
	int32 StatusCode = 200;

	UPROPERTY(BlueprintReadWrite, Category = "CefContentHttpServer")
	FString ContentType = TEXT("application/octet-stream");

	UPROPERTY(BlueprintReadWrite, Category = "CefContentHttpServer")
	TArray<uint8> Body;
};

/** @brief Pluggable image request handler contract. */
UCLASS(Abstract, Blueprintable, BlueprintType, DisplayName = "Image Handler (Abstract)")
class CEFCONTENTHTTPSERVER_API UCefContentHttpImageRequestHandler : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief Handles /img request.
	 * @param InRequestContext Parsed request data.
	 * @param OutResponse Response payload.
	 * @param OutError Error text for failed handling.
	 * @return True when request is handled successfully.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool HandleImageRequest(const FCefContentHttpImageRequestContext& InRequestContext, FCefContentHttpImageResponse& OutResponse, FString& OutError);
};
