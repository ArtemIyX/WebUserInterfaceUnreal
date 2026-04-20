#include "Handlers/CefContentHttpImageRequestHandler.h"

bool UCefContentHttpImageRequestHandler::HandleImageRequest(const FCefContentHttpImageRequestContext& InRequestContext, FCefContentHttpImageResponse& OutResponse, FString& OutError)
{
	(void)InRequestContext;
	OutResponse.StatusCode = 501;
	OutResponse.ContentType = TEXT("application/json");
	OutError = TEXT("Image request handler is not implemented");
	OutResponse.Body.Reset();
	return false;
}
