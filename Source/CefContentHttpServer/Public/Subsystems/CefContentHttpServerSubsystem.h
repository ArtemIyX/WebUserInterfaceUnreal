#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CefContentHttpServerSubsystem.generated.h"

class ICefContentImageCacheService;
class UTexture2D;

UCLASS(BlueprintType)
class CEFCONTENTHTTPSERVER_API UCefContentHttpServerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UCefContentHttpServerSubsystem();

public:
	virtual void Initialize(FSubsystemCollectionBase& InCollection) override;
	virtual void Deinitialize() override;

public:
	void SetImageCacher(TSharedPtr<ICefContentImageCacheService> InImageCacher);
	const TSharedPtr<ICefContentImageCacheService>& GetImageCacher() const;

	UFUNCTION(BlueprintCallable, Category = "CefContentHttpServer")
	void InitDefaultImageCacher();

	UFUNCTION(BlueprintCallable, Category = "CefContentHttpServer")
	UTexture2D* GetImageByPackagePath(const FString& InPackagePath, bool& bOutSuccess);

	UFUNCTION(BlueprintCallable, Category = "CefContentHttpServer")
	void ClearCachedImages();

	UFUNCTION(BlueprintPure, Category = "CefContentHttpServer")
	int32 GetCachedImageCount() const;

private:
	TSharedPtr<ICefContentImageCacheService> ImageCacher;
};
