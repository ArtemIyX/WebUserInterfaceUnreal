/**
 * @file CefContentHttpServer/Private/Handlers/CefContentDefaultImageRequestHandler.cpp
 * @brief Default handler implementation using module services.
 */
#include "Handlers/CefContentDefaultImageRequestHandler.h"

#include "CefContentHttpServer.h"
#include "Services/CefContentImageCacheService.h"
#include "Services/CefContentImageEncodeService.h"

bool UCefContentDefaultImageRequestHandler::HandleImageRequest_Implementation(const FCefContentHttpImageRequestContext& InRequestContext, FCefContentHttpImageResponse& OutResponse, FString& OutError)
{
	if (InRequestContext.AssetPath.IsEmpty())
	{
		OutResponse.StatusCode = 400;
		OutResponse.ContentType = TEXT("application/json");
		OutError = TEXT("Missing required 'asset' parameter");
		return false;
	}

	if (!FCefContentHttpServerModule::IsAvailable())
	{
		OutResponse.StatusCode = 500;
		OutResponse.ContentType = TEXT("application/json");
		OutError = TEXT("CefContentHttpServer module is not available");
		return false;
	}

	FCefContentImageCacheService* const imageCacher = FCefContentHttpServerModule::Get().GetImageCacher();
	FCefContentImageEncodeService* const imageEncoder = FCefContentHttpServerModule::Get().GetImageEncoder();
	if (!imageCacher || !imageEncoder)
	{
		OutResponse.StatusCode = 500;
		OutResponse.ContentType = TEXT("application/json");
		OutError = TEXT("Module services are not initialized");
		return false;
	}

	UTexture2D* image = nullptr;
	if (!imageCacher->GetImageByPackagePath(InRequestContext.AssetPath, image) || !image)
	{
		OutResponse.StatusCode = 404;
		OutResponse.ContentType = TEXT("application/json");
		OutError = FString::Printf(TEXT("Image asset not found: '%s'"), *InRequestContext.AssetPath);
		return false;
	}

	TArray<uint8> pngBytes;
	if (!imageEncoder->EncodeTextureToPngBytes(image, pngBytes, OutError))
	{
		OutResponse.StatusCode = 500;
		OutResponse.ContentType = TEXT("application/json");
		return false;
	}

	OutResponse.StatusCode = 200;
	OutResponse.ContentType = TEXT("image/png");
	OutResponse.Body = MoveTemp(pngBytes);
	return true;
}
