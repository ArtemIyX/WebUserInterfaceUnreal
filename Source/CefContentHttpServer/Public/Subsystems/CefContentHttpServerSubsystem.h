/**
 * @file CefContentHttpServer/Public/Subsystems/CefContentHttpServerSubsystem.h
 * @brief Game instance subsystem API for content HTTP server lifecycle.
 */
#pragma once

#include "CoreMinimal.h"
#include "Handlers/CefContentHttpImageRequestHandler.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CefContentHttpServerSubsystem.generated.h"

class UCefContentHttpImageRequestHandler;
class UTexture2D;

/** @brief Subsystem facade for HTTP image serving and cache access. */
UCLASS(BlueprintType)
class CEFCONTENTHTTPSERVER_API UCefContentHttpServerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UCefContentHttpServerSubsystem();

public:
	/** @brief Starts HTTP server automatically with default settings. */
	virtual void Initialize(FSubsystemCollectionBase& InCollection) override;
	/** @brief Stops HTTP server and clears runtime references. */
	virtual void Deinitialize() override;

public:
	/**
	 * @brief Gets image from module cache service by asset path.
	 * @param InPackagePath Asset path.
	 * @param bOutSuccess True when image exists.
	 * @return Texture or null.
	 */
	UFUNCTION(BlueprintCallable, Category = "CefContentHttpServer")
	UTexture2D* GetImageByPackagePath(const FString& InPackagePath, bool& bOutSuccess);

	/** @brief Clears module image cache. */
	UFUNCTION(BlueprintCallable, Category = "CefContentHttpServer")
	void ClearCachedImages();

	/** @brief Returns current module cache size. */
	UFUNCTION(BlueprintPure, Category = "CefContentHttpServer")
	int32 GetCachedImageCount() const;

	/**
	 * @brief Starts HTTP server on configured port or override.
	 * @param InPortOverride Optional port override. Use 0 to keep configured port.
	 * @return True when server is running.
	 */
	UFUNCTION(BlueprintCallable, Category = "CefContentHttpServer")
	bool StartServer(int32 InPortOverride = 0);

	/** @brief Stops HTTP server if running. */
	UFUNCTION(BlueprintCallable, Category = "CefContentHttpServer")
	void StopServer();

	/** @brief Returns true if HTTP server is running. */
	UFUNCTION(BlueprintPure, Category = "CefContentHttpServer")
	bool IsServerRunning() const;

	/**
	 * @brief Sets request handler class used for /img route handling.
	 * @param InRequestHandlerClass Handler UObject class.
	 * @param bInRestartIfRunning Restart server when handler changes and server is running.
	 */
	UFUNCTION(BlueprintCallable, Category = "CefContentHttpServer")
	void SetRequestHandlerClass(TSubclassOf<UCefContentHttpImageRequestHandler> InRequestHandlerClass, bool bInRestartIfRunning = true);

private:
	/** @brief Creates handler instance from configured class. */
	bool CreateRequestHandlerInstance();

private:
	UPROPERTY(EditAnywhere, Category = "CefContentHttpServer")
	int32 HttpPort = 18080;

	UPROPERTY(EditAnywhere, Category = "CefContentHttpServer")
	TSubclassOf<UCefContentHttpImageRequestHandler> RequestHandlerClass;

	UPROPERTY(Transient)
	TObjectPtr<UCefContentHttpImageRequestHandler> RequestHandlerInstance;
};
