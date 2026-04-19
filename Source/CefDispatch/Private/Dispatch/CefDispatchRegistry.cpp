#include "Dispatch/CefDispatchRegistry.h"

bool FCefDispatchRegistry::RegisterFactory(uint32 InMessageType, FCefDispatchFactory InFactory, bool bInAllowReplace)
{
	if (!InFactory)
	{
		return false;
	}

	FWriteScopeLock writeLock(RoutesLock);
	if (!bInAllowReplace && Routes.Contains(InMessageType))
	{
		return false;
	}

	Routes.Add(InMessageType, MoveTemp(InFactory));
	return true;
}

bool FCefDispatchRegistry::UnregisterFactory(uint32 InMessageType)
{
	FWriteScopeLock writeLock(RoutesLock);
	return Routes.Remove(InMessageType) > 0;
}

bool FCefDispatchRegistry::HasFactory(uint32 InMessageType) const
{
	FReadScopeLock readLock(RoutesLock);
	return Routes.Contains(InMessageType);
}

int32 FCefDispatchRegistry::GetFactoryCount() const
{
	FReadScopeLock readLock(RoutesLock);
	return Routes.Num();
}

ECefDispatchFactoryResult FCefDispatchRegistry::Decode(uint32 InMessageType, const TArray<uint8>& InPayload,
                                                       TUniquePtr<ICefDispatchValue>& OutValue, FString& OutError) const
{
	FCefDispatchFactory routeFactory;
	{
		FReadScopeLock readLock(RoutesLock);
		const FCefDispatchFactory* foundFactory = Routes.Find(InMessageType);
		if (!foundFactory)
		{
			OutValue.Reset();
			OutError = FString::Printf(TEXT("No dispatch factory for MessageType=%u"), InMessageType);
			return ECefDispatchFactoryResult::RouteNotFound;
		}
		routeFactory = *foundFactory;
	}

	if (!routeFactory)
	{
		OutValue.Reset();
		OutError = FString::Printf(TEXT("Invalid dispatch factory for MessageType=%u"), InMessageType);
		return ECefDispatchFactoryResult::InvalidFactory;
	}

	OutValue = routeFactory(InMessageType, InPayload, OutError);
	if (!OutValue.IsValid())
	{
		if (OutError.IsEmpty())
		{
			OutError = FString::Printf(TEXT("Factory returned null for MessageType=%u"), InMessageType);
		}
		return ECefDispatchFactoryResult::FactoryFailed;
	}

	return ECefDispatchFactoryResult::Ok;
}
