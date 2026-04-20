/**
 * @file CefContentHttpServer/Public/Services/CefContentImageEncodeService.h
 * @brief Texture encoding utilities.
 */
#pragma once

#include "CoreMinimal.h"

class UTexture2D;

/** @brief Encodes textures into HTTP-friendly binary payloads. */
class CEFCONTENTHTTPSERVER_API FCefContentImageEncodeService final
{
public:
	/**
	 * @brief Encodes texture into PNG bytes.
	 * @param InTexture Texture to encode.
	 * @param OutPngBytes Encoded PNG bytes.
	 * @param OutError Error text on failure.
	 * @return True on success.
	 */
	bool EncodeTextureToPngBytes(UTexture2D* InTexture, TArray<uint8>& OutPngBytes, FString& OutError) const;
};
