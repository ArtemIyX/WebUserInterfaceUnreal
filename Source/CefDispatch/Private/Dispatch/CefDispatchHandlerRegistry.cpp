#include "Dispatch/CefDispatchHandlerRegistry.h"

bool FCefDispatchHandlerRegistry::RegisterHandler(const FCefDispatchRouteKey& InRouteKey, FCefDispatchHandler InHandler, bool bInAllowReplace)
{
	if (!InHandler)
	{
		return false;
	}

	return RegisterHandler(
		InRouteKey,
		[Handler = MoveTemp(InHandler)](const FCefDispatchRouteKey& InRouteKey, const ICefDispatchValue& InValue,
			const ICefDispatchMetadata*, FString& OutError) mutable
		{
			return Handler(InRouteKey, InValue, OutError);
		},
		bInAllowReplace);
}

bool FCefDispatchHandlerRegistry::RegisterHandler(const FCefDispatchRouteKey& InRouteKey, FCefDispatchMetadataHandler InHandler, bool bInAllowReplace)
{
	if (!InRouteKey.IsValid() || !InHandler)
	{
		return false;
	}

	FWriteScopeLock writeLock(HandlersLock);
	if (!bInAllowReplace && Handlers.Contains(InRouteKey))
	{
		return false;
	}

	Handlers.Add(InRouteKey, MoveTemp(InHandler));
	return true;
}

bool FCefDispatchHandlerRegistry::UnregisterHandler(const FCefDispatchRouteKey& InRouteKey)
{
	if (!InRouteKey.IsValid()) return false;
	FWriteScopeLock writeLock(HandlersLock);
	return Handlers.Remove(InRouteKey) > 0;
}

bool FCefDispatchHandlerRegistry::HasHandler(const FCefDispatchRouteKey& InRouteKey) const
{
	if (!InRouteKey.IsValid()) return false;
	FReadScopeLock readLock(HandlersLock);
	return Handlers.Contains(InRouteKey);
}

int32 FCefDispatchHandlerRegistry::GetHandlerCount() const
{
	FReadScopeLock readLock(HandlersLock);
	return Handlers.Num();
}

ECefDispatchHandlerResult FCefDispatchHandlerRegistry::Handle(const FCefDispatchRouteKey& InRouteKey, const ICefDispatchValue& InValue, FString& OutError) const
{
	return Handle(InRouteKey, InValue, nullptr, OutError);
}

ECefDispatchHandlerResult FCefDispatchHandlerRegistry::Handle(const FCefDispatchRouteKey& InRouteKey, const ICefDispatchValue& InValue,
	TSharedPtr<const ICefDispatchMetadata> InMetadata, FString& OutError) const
{
	OutError.Empty();
	if (!InRouteKey.IsValid()) return ECefDispatchHandlerResult::InvalidRouteKey;
	FCefDispatchMetadataHandler routeHandler;
	{
		FReadScopeLock readLock(HandlersLock);
		const FCefDispatchMetadataHandler* foundHandler = Handlers.Find(InRouteKey);
		if (!foundHandler)
		{
			OutError = FString::Printf(TEXT("No dispatch handler for route %s"), *InRouteKey.GetDiagnosticText());
			return ECefDispatchHandlerResult::HandlerNotFound;
		}
		routeHandler = *foundHandler;
	}

	if (!routeHandler)
	{
		OutError = FString::Printf(TEXT("Invalid dispatch handler for route %s"), *InRouteKey.GetDiagnosticText());
		return ECefDispatchHandlerResult::InvalidHandler;
	}

	if (!routeHandler(InRouteKey, InValue, InMetadata.Get(), OutError))
	{
		if (OutError == TEXT("CefDispatch.TypedHandler.TypeMismatch"))
		{
			return ECefDispatchHandlerResult::HandlerTypeMismatch;
		}

		if (OutError.IsEmpty())
		{
			OutError = FString::Printf(TEXT("Dispatch handler failed for route %s"), *InRouteKey.GetDiagnosticText());
		}
		return ECefDispatchHandlerResult::HandlerFailed;
	}

	return ECefDispatchHandlerResult::Ok;
}

ECefDispatchHandlerResult FCefDispatchHandlerRegistry::Dispatch(const FCefDispatchRouteKey& InRouteKey, const TArray<uint8>& InPayload, FString& OutError) const
{
	return Dispatch(InRouteKey, InPayload, nullptr, OutError);
}

ECefDispatchHandlerResult FCefDispatchHandlerRegistry::Dispatch(const FCefDispatchRouteKey& InRouteKey, const TArray<uint8>& InPayload,
	TSharedPtr<const ICefDispatchMetadata> InMetadata, FString& OutError) const
{
	OutError.Empty();
	if (!InRouteKey.IsValid()) return ECefDispatchHandlerResult::InvalidRouteKey;
	TSharedPtr<FCefDispatchRegistry> decodeRegistry = GetDecodeRegistry();
	if (!decodeRegistry.IsValid())
	{
		OutError = TEXT("Dispatch decode registry is unavailable");
		return ECefDispatchHandlerResult::DecodeRegistryUnavailable;
	}

	TUniquePtr<ICefDispatchValue> decodedValue;
	switch (decodeRegistry->Decode(InRouteKey, InPayload, decodedValue, OutError))
	{
	case ECefDispatchFactoryResult::Ok:
		break;
	case ECefDispatchFactoryResult::RouteNotFound:
		return ECefDispatchHandlerResult::DecodeRouteNotFound;
	case ECefDispatchFactoryResult::FactoryFailed:
	case ECefDispatchFactoryResult::InvalidFactory:
	default:
		return ECefDispatchHandlerResult::DecodeFailed;
	}

	if (!decodedValue.IsValid())
	{
		if (OutError.IsEmpty())
		{
			OutError = FString::Printf(TEXT("Dispatch decode returned null for route %s"), *InRouteKey.GetDiagnosticText());
		}
		return ECefDispatchHandlerResult::DecodeFailed;
	}

	return Handle(InRouteKey, *decodedValue, MoveTemp(InMetadata), OutError);
}
