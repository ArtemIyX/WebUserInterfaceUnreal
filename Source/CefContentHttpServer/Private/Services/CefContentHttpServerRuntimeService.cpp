#include "Services/CefContentHttpServerRuntimeService.h"

#include "CefContentHttpServer.h"
#include "Dom/JsonObject.h"
#include "Handlers/CefContentHttpImageRequestHandler.h"
#include "HttpPath.h"
#include "HttpServerModule.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "IHttpRouter.h"

#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

bool FCefContentHttpServerRuntimeService::Start(int32 InPort, TWeakObjectPtr<UCefContentHttpImageRequestHandler> InHandler)
{
	if (IsRunning())
	{
		UE_LOG(LogCefContentHttpServer, Warning, TEXT("Http server start skipped: already running on port %d"), ListenPort);
		return true;
	}

	if (InPort <= 0)
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("Http server start failed: invalid port %d"), InPort);
		return false;
	}

	if (!InHandler.IsValid())
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("Http server start failed: request handler is invalid"));
		return false;
	}

	FHttpServerModule& httpServerModule = FHttpServerModule::Get();
	HttpRouter = httpServerModule.GetHttpRouter(static_cast<uint32>(InPort));
	if (!HttpRouter.IsValid())
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("Http server start failed: router is null for port %d"), InPort);
		return false;
	}

	RequestHandler = InHandler;
	ImgRouteHandle = HttpRouter->BindRoute(
		FHttpPath(TEXT("/img")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateRaw(this, &FCefContentHttpServerRuntimeService::HandleImageRoute));

	if (!ImgRouteHandle.IsValid())
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("Http server start failed: /img route bind failed"));
		RequestHandler.Reset();
		HttpRouter.Reset();
		return false;
	}

	httpServerModule.StartAllListeners();
	ListenPort = InPort;
	UE_LOG(LogCefContentHttpServer, Log, TEXT("Http server started on port %d route /img"), ListenPort);
	return true;
}

void FCefContentHttpServerRuntimeService::Stop()
{
	if (HttpRouter.IsValid() && ImgRouteHandle.IsValid())
	{
		HttpRouter->UnbindRoute(ImgRouteHandle);
		UE_LOG(LogCefContentHttpServer, Log, TEXT("Http route /img unbound"));
	}

	ImgRouteHandle.Reset();
	RequestHandler.Reset();
	HttpRouter.Reset();
	ListenPort = 0;
}

bool FCefContentHttpServerRuntimeService::IsRunning() const
{
	return ListenPort > 0 && ImgRouteHandle.IsValid() && HttpRouter.IsValid();
}

int32 FCefContentHttpServerRuntimeService::GetPort() const
{
	return ListenPort;
}

bool FCefContentHttpServerRuntimeService::HandleImageRoute(const FHttpServerRequest& InRequest, const FHttpResultCallback& InOnComplete)
{
	if (!RequestHandler.IsValid())
	{
		UE_LOG(LogCefContentHttpServer, Error, TEXT("HandleImageRoute failed: request handler is invalid"));
		CompleteJson(InOnComplete, 500, TEXT("Request handler is not set"));
		return true;
	}

	const FString rawBody = ParseBodyAsUtf8(InRequest);
	FCefContentHttpImageRequestContext requestContext;
	requestContext.RawBody = rawBody;
	requestContext.QueryParams = InRequest.QueryParams;
	requestContext.AssetPath = ResolveAssetPath(InRequest, rawBody);

	FCefContentHttpImageResponse response;
	FString error;
	const bool bHandled = RequestHandler->HandleImageRequest(requestContext, response, error);
	if (!bHandled)
	{
		const int32 statusCode = response.StatusCode > 0 ? response.StatusCode : 500;
		CompleteJson(InOnComplete, statusCode, error.IsEmpty() ? TEXT("Request handling failed") : error);
		return true;
	}

	CompleteBinary(InOnComplete, response.StatusCode, response.ContentType, response.Body);
	return true;
}

FString FCefContentHttpServerRuntimeService::ResolveAssetPath(const FHttpServerRequest& InRequest, const FString& InRawBody) const
{
	if (const FString* const queryValue = InRequest.QueryParams.Find(TEXT("asset")))
	{
		return *queryValue;
	}
	if (const FString* const queryValue = InRequest.QueryParams.Find(TEXT("aasset")))
	{
		return *queryValue;
	}

	FString assetPathFromBody;
	TSharedPtr<FJsonObject> jsonObject;
	const TSharedRef<TJsonReader<>> jsonReader = TJsonReaderFactory<>::Create(InRawBody);
	if (FJsonSerializer::Deserialize(jsonReader, jsonObject) && jsonObject.IsValid())
	{
		if (jsonObject->TryGetStringField(TEXT("asset"), assetPathFromBody))
		{
			return assetPathFromBody;
		}
		if (jsonObject->TryGetStringField(TEXT("asset="), assetPathFromBody))
		{
			return assetPathFromBody;
		}
	}

	if (InRawBody.StartsWith(TEXT("asset=")))
	{
		return InRawBody.RightChop(6);
	}

	return FString();
}

FString FCefContentHttpServerRuntimeService::ParseBodyAsUtf8(const FHttpServerRequest& InRequest) const
{
	if (InRequest.Body.Num() <= 0)
	{
		return FString();
	}

	const UTF8CHAR* const rawBodyData = reinterpret_cast<const UTF8CHAR*>(InRequest.Body.GetData());
	FUTF8ToTCHAR converter(rawBodyData, InRequest.Body.Num());
	return FString(converter.Length(), converter.Get());
}

void FCefContentHttpServerRuntimeService::CompleteJson(const FHttpResultCallback& InOnComplete, int32 InStatusCode, const FString& InMessage)
{
	const FString escapedMessage = InMessage.Replace(TEXT("\""), TEXT("\\\""));
	const FString jsonBody = FString::Printf(TEXT("{\"error\":\"%s\"}"), *escapedMessage);
	FTCHARToUTF8 utf8(*jsonBody);
	TArray<uint8> bytes;
	bytes.Append(reinterpret_cast<const uint8*>(utf8.Get()), utf8.Length());
	CompleteBinary(InOnComplete, InStatusCode, TEXT("application/json"), bytes);
}

void FCefContentHttpServerRuntimeService::CompleteBinary(const FHttpResultCallback& InOnComplete, int32 InStatusCode, const FString& InContentType, const TArray<uint8>& InBody)
{
	TArray<uint8> responseBody = InBody;
	TUniquePtr<FHttpServerResponse> response = FHttpServerResponse::Create(MoveTemp(responseBody), InContentType);
	response->Code = static_cast<EHttpServerResponseCodes>(InStatusCode);
	InOnComplete(MoveTemp(response));
}
