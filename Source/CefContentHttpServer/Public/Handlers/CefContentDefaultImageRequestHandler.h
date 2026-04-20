#pragma once

#include "CoreMinimal.h"
#include "Handlers/CefContentHttpImageRequestHandler.h"
#include "CefContentDefaultImageRequestHandler.generated.h"

UCLASS(BlueprintType)
class CEFCONTENTHTTPSERVER_API UCefContentDefaultImageRequestHandler : public UCefContentHttpImageRequestHandler
{
	GENERATED_BODY()

public:
	virtual bool HandleImageRequest(const FCefContentHttpImageRequestContext& InRequestContext, FCefContentHttpImageResponse& OutResponse, FString& OutError) override;
};
