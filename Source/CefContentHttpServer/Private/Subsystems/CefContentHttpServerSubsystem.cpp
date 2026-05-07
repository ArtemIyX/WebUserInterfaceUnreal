/**
 * @file CefContentHttpServer/Private/Subsystems/CefContentHttpServerSubsystem.cpp
 * @brief Subsystem lifecycle and module-service forwarding.
 */
#include "Subsystems/CefContentHttpServerSubsystem.h"

#include "CefContentHttpServer.h"
#include "Handlers/CefContentDefaultImageRequestHandler.h"
#include "Services/CefContentImageCacheService.h"
#include "Services/CefContentImageEncodeService.h"
#include "Services/CefContentHttpServerRuntimeService.h"
#include "Stats/CefContentHttpServerStats.h"

UCefContentHttpServerSubsystem::UCefContentHttpServerSubsystem()
{
}

void UCefContentHttpServerSubsystem::Initialize(FSubsystemCollectionBase& InCollection)
{
	Super::Initialize(InCollection);
	if (!RequestHandlerClass)
	{
		RequestHandlerClass = UCefContentDefaultImageRequestHandler::StaticClass();
	}
	StartServer();
}

void UCefContentHttpServerSubsystem::Deinitialize()
{
	UE_LOG(LogCefContentHttpServer, Log, TEXT("CefContentHttpServer subsystem deinitialize"));
	StopServer();
	RequestHandlerInstance = nullptr;
	Super::Deinitialize();
}

UTexture2D* UCefContentHttpServerSubsystem::GetImageByPackagePath(const FString& InPackagePath, bool& bOutSuccess)
{
	bOutSuccess = false;
	if (!FCefContentHttpServerModule::IsAvailable())
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("GetImageByPackagePath failed: module is not available"));
		return nullptr;
	}

	FCefContentImageCacheService* const imageCacher = FCefContentHttpServerModule::Get().GetImageCacher();
	if (!imageCacher)
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("GetImageByPackagePath failed: image cacher is not available"));
		return nullptr;
	}

	UTexture2D* image = nullptr;
	bOutSuccess = imageCacher->GetImageByPackagePath(InPackagePath, image);
	if (!bOutSuccess)
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("GetImageByPackagePath failed for '%s'"), *InPackagePath);
	}
	return image;
}

void UCefContentHttpServerSubsystem::ClearCachedImages()
{
	if (!FCefContentHttpServerModule::IsAvailable())
	{
		UE_LOG(LogCefContentHttpServer, Warning, TEXT("ClearCachedImages skipped: module is not available"));
		SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, 0);
		return;
	}

	FCefContentImageCacheService* const imageCacher = FCefContentHttpServerModule::Get().GetImageCacher();
	if (!imageCacher)
	{
		UE_LOG(LogCefContentHttpServer, Warning, TEXT("ClearCachedImages skipped: image cacher is not available"));
		SET_DWORD_STAT(STAT_CefContentHttpServer_CachedImages, 0);
		return;
	}

	imageCacher->ClearCache();

	FCefContentImageEncodeService* const imageEncoder = FCefContentHttpServerModule::Get().GetImageEncoder();
	if (imageEncoder)
	{
		imageEncoder->ClearEncodedCache();
	}
}

int32 UCefContentHttpServerSubsystem::GetCachedImageCount() const
{
	if (!FCefContentHttpServerModule::IsAvailable())
	{
		return 0;
	}

	FCefContentImageCacheService* const imageCacher = FCefContentHttpServerModule::Get().GetImageCacher();
	if (!imageCacher)
	{
		return 0;
	}

	return imageCacher->GetCachedImageCount();
}

bool UCefContentHttpServerSubsystem::StartServer(int32 InPortOverride)
{
	if (!FCefContentHttpServerModule::IsAvailable())
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("StartServer failed: module is not available"));
		return false;
	}

	if (!CreateRequestHandlerInstance())
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("StartServer failed: request handler creation failed"));
		return false;
	}

	FCefContentHttpServerRuntimeService* const httpServerService = FCefContentHttpServerModule::Get().GetHttpServerService();
	if (!httpServerService)
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("StartServer failed: http server service is null"));
		return false;
	}

	const int32 listenPort = InPortOverride > 0 ? InPortOverride : HttpPort;
	HttpPort = listenPort;
	return httpServerService->Start(listenPort, RequestHandlerInstance);
}

void UCefContentHttpServerSubsystem::StopServer()
{
	if (!FCefContentHttpServerModule::IsAvailable())
	{
		return;
	}

	FCefContentHttpServerRuntimeService* const httpServerService = FCefContentHttpServerModule::Get().GetHttpServerService();
	if (httpServerService)
	{
		httpServerService->Stop();
	}
}

bool UCefContentHttpServerSubsystem::IsServerRunning() const
{
	if (!FCefContentHttpServerModule::IsAvailable())
	{
		return false;
	}

	FCefContentHttpServerRuntimeService* const httpServerService = FCefContentHttpServerModule::Get().GetHttpServerService();
	return httpServerService && httpServerService->IsRunning();
}

void UCefContentHttpServerSubsystem::SetRequestHandlerClass(TSubclassOf<UCefContentHttpImageRequestHandler> InRequestHandlerClass, bool bInRestartIfRunning)
{
	RequestHandlerClass = InRequestHandlerClass;
	const bool bWasRunning = IsServerRunning();
	if (!CreateRequestHandlerInstance())
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("SetRequestHandlerClass failed: could not instantiate request handler"));
		return;
	}

	if (bInRestartIfRunning && bWasRunning)
	{
		StopServer();
		StartServer();
	}
}

bool UCefContentHttpServerSubsystem::CreateRequestHandlerInstance()
{
	if (!RequestHandlerClass)
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("CreateRequestHandlerInstance failed: RequestHandlerClass is null"));
		return false;
	}

	UCefContentHttpImageRequestHandler* const newHandler = NewObject<UCefContentHttpImageRequestHandler>(this, RequestHandlerClass);
	if (!newHandler)
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("CreateRequestHandlerInstance failed: NewObject returned null for class %s"), *RequestHandlerClass->GetName());
		return false;
	}

	RequestHandlerInstance = newHandler;
	UE_LOG(LogCefContentHttpServer, Log, TEXT("Request handler instantiated: %s"), *RequestHandlerClass->GetName());
	return true;
}
