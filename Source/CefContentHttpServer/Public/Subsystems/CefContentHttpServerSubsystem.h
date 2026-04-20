#pragma once

#include "CoreMinimal.h"
#include "Handlers/CefContentHttpImageRequestHandler.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CefContentHttpServerSubsystem.generated.h"

class UCefContentHttpImageRequestHandler;
class UTexture2D;

UCLASS(BlueprintType)
class CEFCONTENTHTTPSERVER_API UCefContentHttpServerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& InCollection) override;
	virtual void Deinitialize() override;

public:
	UFUNCTION(BlueprintCallable, Category = "CefContentHttpServer")
	UTexture2D* GetImageByPackagePath(const FString& InPackagePath, bool& bOutSuccess);

	UFUNCTION(BlueprintCallable, Category = "CefContentHttpServer")
	void ClearCachedImages();

	UFUNCTION(BlueprintPure, Category = "CefContentHttpServer")
	int32 GetCachedImageCount() const;

	UFUNCTION(BlueprintCallable, Category = "CefContentHttpServer")
	bool StartServer(int32 InPortOverride = 0);

	UFUNCTION(BlueprintCallable, Category = "CefContentHttpServer")
	void StopServer();

	UFUNCTION(BlueprintPure, Category = "CefContentHttpServer")
	bool IsServerRunning() const;

	UFUNCTION(BlueprintCallable, Category = "CefContentHttpServer")
	void SetRequestHandlerClass(TSubclassOf<UCefContentHttpImageRequestHandler> InRequestHandlerClass, bool bInRestartIfRunning = true);

private:
	bool CreateRequestHandlerInstance();

private:
	UPROPERTY(EditAnywhere, Category = "CefContentHttpServer")
	int32 HttpPort = 18080;

	UPROPERTY(EditAnywhere, Category = "CefContentHttpServer")
	TSubclassOf<UCefContentHttpImageRequestHandler> RequestHandlerClass;

	UPROPERTY(Transient)
	TObjectPtr<UCefContentHttpImageRequestHandler> RequestHandlerInstance;
};
