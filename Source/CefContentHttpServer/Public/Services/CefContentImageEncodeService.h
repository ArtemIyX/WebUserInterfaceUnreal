#pragma once

#include "CoreMinimal.h"

class UTexture2D;

class CEFCONTENTHTTPSERVER_API FCefContentImageEncodeService final
{
public:
	bool EncodeTextureToPngBytes(UTexture2D* InTexture, TArray<uint8>& OutPngBytes, FString& OutError) const;
};
