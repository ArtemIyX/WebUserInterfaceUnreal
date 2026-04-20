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

void UCefContentDefaultImageRequestHandler::HandleImageRequestAsync(const FCefContentHttpImageRequestContext& InRequestContext, FOnImageRequestCompleted InOnCompleted)
{
	if (InRequestContext.AssetPath.IsEmpty())
	{
		FCefContentHttpImageResponse response;
		response.StatusCode = 400;
		response.ContentType = TEXT("application/json");
		InOnCompleted(false, response, TEXT("Missing required 'asset' parameter"));
		return;
	}

	if (!FCefContentHttpServerModule::IsAvailable())
	{
		FCefContentHttpImageResponse response;
		response.StatusCode = 500;
		response.ContentType = TEXT("application/json");
		InOnCompleted(false, response, TEXT("CefContentHttpServer module is not available"));
		return;
	}

	FCefContentImageEncodeService* const imageEncoder = FCefContentHttpServerModule::Get().GetImageEncoder();
	if (!imageEncoder)
	{
		FCefContentHttpImageResponse response;
		response.StatusCode = 500;
		response.ContentType = TEXT("application/json");
		InOnCompleted(false, response, TEXT("Image encode service is not initialized"));
		return;
	}

	imageEncoder->RequestPngBytesByAssetPathAsync(InRequestContext.AssetPath, [InOnCompleted](bool bInSuccess, const TArray<uint8>& InPngBytes, const FString& InError) {
		FCefContentHttpImageResponse response;
		const bool bNotFound = InError.Contains(TEXT("not found"));
		response.StatusCode = bInSuccess ? 200 : (bNotFound ? 404 : 500);
		response.ContentType = bInSuccess ? TEXT("image/png") : TEXT("application/json");
		if (bInSuccess)
		{
			response.Body = InPngBytes;
		}
		InOnCompleted(bInSuccess, response, InError);
	});
}
