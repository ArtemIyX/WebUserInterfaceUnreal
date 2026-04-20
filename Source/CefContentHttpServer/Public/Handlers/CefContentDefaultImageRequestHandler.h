#pragma once

#include "CoreMinimal.h"
#include "Handlers/CefContentHttpImageRequestHandler.h"
#include "CefContentDefaultImageRequestHandler.generated.h"

UCLASS(BlueprintType, DisplayName="Default Image Handler")
class CEFCONTENTHTTPSERVER_API UCefContentDefaultImageRequestHandler : public UCefContentHttpImageRequestHandler
{
	GENERATED_BODY()

public:
	virtual bool HandleImageRequest_Implementation(const FCefContentHttpImageRequestContext& InRequestContext, FCefContentHttpImageResponse& OutResponse, FString& OutError) override;
};
