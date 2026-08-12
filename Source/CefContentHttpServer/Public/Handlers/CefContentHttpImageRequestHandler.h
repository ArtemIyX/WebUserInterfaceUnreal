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

	/** Asset path extracted from the request, typically used as the image lookup key. */
	UPROPERTY(BlueprintReadOnly, Category = "CefContentHttpServer")
	FString AssetPath;

	/** Raw HTTP request body forwarded to the handler when needed. */
	UPROPERTY(BlueprintReadOnly, Category = "CefContentHttpServer")
	FString RawBody;

	/** Parsed query-string parameters associated with the image request. */
	UPROPERTY(BlueprintReadOnly, Category = "CefContentHttpServer")
	TMap<FString, FString> QueryParams;
};

/** @brief Output payload produced by image request handlers. */
USTRUCT(BlueprintType)
struct CEFCONTENTHTTPSERVER_API FCefContentHttpImageResponse
{
	GENERATED_BODY()

	/** HTTP-style status code returned by the handler. */
	UPROPERTY(BlueprintReadWrite, Category = "CefContentHttpServer")
	int32 StatusCode = 200;

	/** MIME type of the response body bytes. */
	UPROPERTY(BlueprintReadWrite, Category = "CefContentHttpServer")
	FString ContentType = TEXT("application/octet-stream");

	/** Binary body payload returned to the HTTP layer. */
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

	/** @brief Async completion callback type for image handlers. */
	using FOnImageRequestCompleted = TFunction<void(bool, const FCefContentHttpImageResponse&, const FString&)>;

	/**
	 * @brief Handles /img request asynchronously.
	 * @param InRequestContext Parsed request data.
	 * @param InOnCompleted Completion callback.
	 */
	virtual void HandleImageRequestAsync(const FCefContentHttpImageRequestContext& InRequestContext, FOnImageRequestCompleted InOnCompleted);
};
