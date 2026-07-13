#include "Dispatch/CefDispatchHandlerRegistry.h"

bool FCefDispatchHandlerRegistry::RegisterHandler(uint32 InMessageType, FCefDispatchHandler InHandler, bool bInAllowReplace)
{
	if (!InHandler)
	{
		return false;
	}

	FWriteScopeLock writeLock(HandlersLock);
	if (!bInAllowReplace && Handlers.Contains(InMessageType))
	{
		return false;
	}

	Handlers.Add(InMessageType, MoveTemp(InHandler));
	return true;
}

bool FCefDispatchHandlerRegistry::UnregisterHandler(uint32 InMessageType)
{
	FWriteScopeLock writeLock(HandlersLock);
	return Handlers.Remove(InMessageType) > 0;
}

bool FCefDispatchHandlerRegistry::HasHandler(uint32 InMessageType) const
{
	FReadScopeLock readLock(HandlersLock);
	return Handlers.Contains(InMessageType);
}

int32 FCefDispatchHandlerRegistry::GetHandlerCount() const
{
	FReadScopeLock readLock(HandlersLock);
	return Handlers.Num();
}

ECefDispatchHandlerResult FCefDispatchHandlerRegistry::Handle(uint32 InMessageType, const ICefDispatchValue& InValue, FString& OutError) const
{
	FCefDispatchHandler routeHandler;
	{
		FReadScopeLock readLock(HandlersLock);
		const FCefDispatchHandler* foundHandler = Handlers.Find(InMessageType);
		if (!foundHandler)
		{
			OutError = FString::Printf(TEXT("No dispatch handler for MessageType=%u"), InMessageType);
			return ECefDispatchHandlerResult::HandlerNotFound;
		}
		routeHandler = *foundHandler;
	}

	if (!routeHandler)
	{
		OutError = FString::Printf(TEXT("Invalid dispatch handler for MessageType=%u"), InMessageType);
		return ECefDispatchHandlerResult::InvalidHandler;
	}

	if (!routeHandler(InMessageType, InValue, OutError))
	{
		if (OutError.Contains(TEXT("type mismatch")))
		{
			return ECefDispatchHandlerResult::HandlerTypeMismatch;
		}

		if (OutError.IsEmpty())
		{
			OutError = FString::Printf(TEXT("Dispatch handler failed for MessageType=%u"), InMessageType);
		}
		return ECefDispatchHandlerResult::HandlerFailed;
	}

	return ECefDispatchHandlerResult::Ok;
}

ECefDispatchHandlerResult FCefDispatchHandlerRegistry::Dispatch(uint32 InMessageType, const TArray<uint8>& InPayload, FString& OutError) const
{
	TSharedPtr<FCefDispatchRegistry> decodeRegistry = GetDecodeRegistry();
	if (!decodeRegistry.IsValid())
	{
		OutError = TEXT("Dispatch decode registry is unavailable");
		return ECefDispatchHandlerResult::DecodeRegistryUnavailable;
	}

	TUniquePtr<ICefDispatchValue> decodedValue;
	switch (decodeRegistry->Decode(InMessageType, InPayload, decodedValue, OutError))
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
			OutError = FString::Printf(TEXT("Dispatch decode returned null for MessageType=%u"), InMessageType);
		}
		return ECefDispatchHandlerResult::DecodeFailed;
	}

	return Handle(InMessageType, *decodedValue, OutError);
}
