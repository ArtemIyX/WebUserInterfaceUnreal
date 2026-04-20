#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CefContentHttpServerSubsystem.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class CEFCONTENTHTTPSERVER_API UCefContentHttpServerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

public:
	UFUNCTION(BlueprintCallable, Category = "CefContentHttpServer")
	UTexture2D* GetImageByPackagePath(const FString& InPackagePath, bool& bOutSuccess);

	UFUNCTION(BlueprintCallable, Category = "CefContentHttpServer")
	void ClearCachedImages();

	UFUNCTION(BlueprintPure, Category = "CefContentHttpServer")
	int32 GetCachedImageCount() const;
};
