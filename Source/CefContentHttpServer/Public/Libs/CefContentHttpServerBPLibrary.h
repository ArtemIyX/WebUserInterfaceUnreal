/**
 * @file CefContentHttpServer/Public/Libs/CefContentHttpServerBPLibrary.h
 * @brief Blueprint helper accessors for CefContentHttpServer.
 */
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CefContentHttpServerBPLibrary.generated.h"

class UCefContentHttpServerSubsystem;

/** @brief Blueprint helper library for subsystem lookup. */
UCLASS()
class CEFCONTENTHTTPSERVER_API UCefContentHttpServerBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * @brief Resolves game instance subsystem from world context object.
	 * @param InWorldContextObject Any UObject with world context.
	 * @return Subsystem instance or null.
	 */
	UFUNCTION(BlueprintCallable, Category = "CefContentHttpServer", meta = (WorldContext = "InWorldContextObject"))
	static UCefContentHttpServerSubsystem* GetCefContentHttpServerSubsystem(const UObject* InWorldContextObject);
};
