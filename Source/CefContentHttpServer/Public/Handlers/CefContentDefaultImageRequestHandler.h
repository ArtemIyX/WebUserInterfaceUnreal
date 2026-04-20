/**
 * @file CefContentHttpServer/Public/Handlers/CefContentDefaultImageRequestHandler.h
 * @brief Default /img handler implementation.
 */
#pragma once

#include "CoreMinimal.h"
#include "Handlers/CefContentHttpImageRequestHandler.h"
#include "CefContentDefaultImageRequestHandler.generated.h"

/** @brief Default handler: cache lookup -> png encode -> HTTP payload. */
UCLASS(BlueprintType, DisplayName = "Default Image Handler")
class CEFCONTENTHTTPSERVER_API UCefContentDefaultImageRequestHandler : public UCefContentHttpImageRequestHandler
{
	GENERATED_BODY()

public:
	/**
	 * @brief Handles request using module cache and encoder services.
	 * @param InRequestContext Parsed request data.
	 * @param OutResponse Output response payload.
	 * @param OutError Error text on failure.
	 * @return True on success.
	 */
	virtual bool HandleImageRequest_Implementation(const FCefContentHttpImageRequestContext& InRequestContext, FCefContentHttpImageResponse& OutResponse, FString& OutError) override;

	/**
	 * @brief Handles request asynchronously with encoded-bytes dedupe/cache.
	 * @param InRequestContext Parsed request data.
	 * @param InOnCompleted Completion callback.
	 */
	virtual void HandleImageRequestAsync(const FCefContentHttpImageRequestContext& InRequestContext, FOnImageRequestCompleted InOnCompleted) override;
};
