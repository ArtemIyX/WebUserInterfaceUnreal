#pragma once

#include "CoreMinimal.h"
#include "HttpRouteHandle.h"

class IHttpRouter;
class UCefContentHttpImageRequestHandler;

class FCefContentHttpServerRuntimeService final
{
public:
	bool Start(int32 InPort, TWeakObjectPtr<UCefContentHttpImageRequestHandler> InHandler);
	void Stop();

	bool IsRunning() const;
	int32 GetPort() const;

private:
	bool HandleImageRoute(const FHttpServerRequest& InRequest, const FHttpResultCallback& InOnComplete);

	FString ResolveAssetPath(const FHttpServerRequest& InRequest, const FString& InRawBody) const;
	FString ParseBodyAsUtf8(const FHttpServerRequest& InRequest) const;

	static void CompleteJson(const FHttpResultCallback& InOnComplete, int32 InStatusCode, const FString& InMessage);
	static void CompleteBinary(const FHttpResultCallback& InOnComplete, int32 InStatusCode, const FString& InContentType, const TArray<uint8>& InBody);

private:
	int32 ListenPort = 0;
	FHttpRouteHandle ImgRouteHandle;
	TSharedPtr<IHttpRouter> HttpRouter;
	TWeakObjectPtr<UCefContentHttpImageRequestHandler> RequestHandler;
};
