/**
 * @file CefContentHttpServer/Private/Handlers/CefContentHttpImageRequestHandler.cpp
 * @brief Base handler default implementation.
 */
#include "Handlers/CefContentHttpImageRequestHandler.h"

bool UCefContentHttpImageRequestHandler::HandleImageRequest_Implementation(const FCefContentHttpImageRequestContext& InRequestContext, FCefContentHttpImageResponse& OutResponse, FString& OutError)
{
	(void)InRequestContext;
	OutResponse.StatusCode = 501;
	OutResponse.ContentType = TEXT("application/json");
	OutError = TEXT("Image request handler is not implemented");
	OutResponse.Body.Reset();
	return false;
}

void UCefContentHttpImageRequestHandler::HandleImageRequestAsync(const FCefContentHttpImageRequestContext& InRequestContext, FOnImageRequestCompleted InOnCompleted)
{
	FCefContentHttpImageResponse response;
	FString error;
	const bool bHandled = HandleImageRequest(InRequestContext, response, error);
	InOnCompleted(bHandled, response, error);
}
