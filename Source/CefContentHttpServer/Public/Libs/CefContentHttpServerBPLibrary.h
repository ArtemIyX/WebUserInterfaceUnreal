#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CefContentHttpServerBPLibrary.generated.h"

class UCefContentHttpServerSubsystem;

UCLASS()
class CEFCONTENTHTTPSERVER_API UCefContentHttpServerBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="CefContentHttpServer", meta=(WorldContext="InWorldContextObject"))
    static UCefContentHttpServerSubsystem* GetCefContentHttpServerSubsystem(const UObject* InWorldContextObject);

    UFUNCTION(BlueprintCallable, Category="CefContentHttpServer", meta=(WorldContext="InWorldContextObject"))
    static bool InitDefaultImageCacher(const UObject* InWorldContextObject);
};
