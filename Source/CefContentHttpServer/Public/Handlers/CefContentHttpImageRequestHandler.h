#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CefContentHttpImageRequestHandler.generated.h"

USTRUCT(BlueprintType)
struct CEFCONTENTHTTPSERVER_API FCefContentHttpImageRequestContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "CefContentHttpServer")
	FString AssetPath;

	UPROPERTY(BlueprintReadOnly, Category = "CefContentHttpServer")
	FString RawBody;

	UPROPERTY(BlueprintReadOnly, Category = "CefContentHttpServer")
	TMap<FString, FString> QueryParams;
};

USTRUCT(BlueprintType)
struct CEFCONTENTHTTPSERVER_API FCefContentHttpImageResponse
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "CefContentHttpServer")
	int32 StatusCode = 200;

	UPROPERTY(BlueprintReadWrite, Category = "CefContentHttpServer")
	FString ContentType = TEXT("application/octet-stream");

	UPROPERTY(BlueprintReadWrite, Category = "CefContentHttpServer")
	TArray<uint8> Body;
};

UCLASS(Abstract, Blueprintable, BlueprintType, DisplayName="Image Handler (Abstract)")
class CEFCONTENTHTTPSERVER_API UCefContentHttpImageRequestHandler : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	bool HandleImageRequest(const FCefContentHttpImageRequestContext& InRequestContext, FCefContentHttpImageResponse& OutResponse, FString& OutError);
};
