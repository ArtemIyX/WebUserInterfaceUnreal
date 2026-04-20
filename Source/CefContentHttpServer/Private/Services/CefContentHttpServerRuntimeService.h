/**
 * @file CefContentHttpServer/Private/Services/CefContentHttpServerRuntimeService.h
 * @brief Internal HTTP runtime service for binding and serving /img route.
 */
#pragma once

#include "CoreMinimal.h"
#include "HttpRouteHandle.h"

class IHttpRouter;
class UCefContentHttpImageRequestHandler;

/** @brief Internal wrapper over Unreal HttpServer route lifecycle. */
class FCefContentHttpServerRuntimeService final
{
public:
	/**
	 * @brief Starts listener and binds /img route.
	 * @param InPort Listen port.
	 * @param InHandler Request handler instance.
	 * @return True on success.
	 */
	bool Start(int32 InPort, TWeakObjectPtr<UCefContentHttpImageRequestHandler> InHandler);
	/** @brief Unbinds route and stops listeners. */
	void Stop();

	/** @brief Returns true when listener and route are active. */
	bool IsRunning() const;
	/** @brief Returns currently active listen port or 0. */
	int32 GetPort() const;

private:
	/** @brief Handles incoming /img requests. */
	bool HandleImageRoute(const FHttpServerRequest& InRequest, const FHttpResultCallback& InOnComplete);

	/** @brief Resolves asset path from query or body payload. */
	FString ResolveAssetPath(const FHttpServerRequest& InRequest, const FString& InRawBody) const;
	/** @brief Decodes raw request body as UTF-8 string. */
	FString ParseBodyAsUtf8(const FHttpServerRequest& InRequest) const;

	/** @brief Completes request with JSON error body. */
	static void CompleteJson(const FHttpResultCallback& InOnComplete, int32 InStatusCode, const FString& InMessage);
	/** @brief Completes request with binary body. */
	static void CompleteBinary(const FHttpResultCallback& InOnComplete, int32 InStatusCode, const FString& InContentType, const TArray<uint8>& InBody);

private:
	int32 ListenPort = 0;
	FHttpRouteHandle ImgRouteHandle;
	TSharedPtr<IHttpRouter> HttpRouter;
	TWeakObjectPtr<UCefContentHttpImageRequestHandler> RequestHandler;
};
