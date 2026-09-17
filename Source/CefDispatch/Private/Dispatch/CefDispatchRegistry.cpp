#include "Dispatch/CefDispatchRegistry.h"

bool FCefDispatchRegistry::RegisterFactory(const FCefDispatchRouteKey& InRouteKey, FCefDispatchFactory InFactory, bool bInAllowReplace)
{
	if (!InRouteKey.IsValid() || !InFactory)
	{
		return false;
	}

	FWriteScopeLock writeLock(RoutesLock);
	if (!bInAllowReplace && Routes.Contains(InRouteKey))
	{
		return false;
	}

	Routes.Add(InRouteKey, MoveTemp(InFactory));
	return true;
}

bool FCefDispatchRegistry::UnregisterFactory(const FCefDispatchRouteKey& InRouteKey)
{
	if (!InRouteKey.IsValid()) return false;
	FWriteScopeLock writeLock(RoutesLock);
	return Routes.Remove(InRouteKey) > 0;
}

bool FCefDispatchRegistry::HasFactory(const FCefDispatchRouteKey& InRouteKey) const
{
	if (!InRouteKey.IsValid()) return false;
	FReadScopeLock readLock(RoutesLock);
	return Routes.Contains(InRouteKey);
}

int32 FCefDispatchRegistry::GetFactoryCount() const
{
	FReadScopeLock readLock(RoutesLock);
	return Routes.Num();
}

ECefDispatchFactoryResult FCefDispatchRegistry::Decode(const FCefDispatchRouteKey& InRouteKey, const TArray<uint8>& InPayload,
                                                       TUniquePtr<ICefDispatchValue>& OutValue, FString& OutError) const
{
	if (!InRouteKey.IsValid())
	{
		OutValue.Reset();
		OutError = TEXT("Invalid dispatch route key");
		return ECefDispatchFactoryResult::InvalidRouteKey;
	}
	FCefDispatchFactory routeFactory;
	{
		FReadScopeLock readLock(RoutesLock);
		const FCefDispatchFactory* foundFactory = Routes.Find(InRouteKey);
		if (!foundFactory)
		{
			OutValue.Reset();
			OutError = FString::Printf(TEXT("No dispatch factory for route %s"), *InRouteKey.GetDiagnosticText());
			return ECefDispatchFactoryResult::RouteNotFound;
		}
		routeFactory = *foundFactory;
	}

	if (!routeFactory)
	{
		OutValue.Reset();
		OutError = FString::Printf(TEXT("Invalid dispatch factory for route %s"), *InRouteKey.GetDiagnosticText());
		return ECefDispatchFactoryResult::InvalidFactory;
	}

	OutValue = routeFactory(InRouteKey, InPayload, OutError);
	if (!OutValue.IsValid())
	{
		if (OutError.IsEmpty())
		{
			OutError = FString::Printf(TEXT("Factory returned null for route %s"), *InRouteKey.GetDiagnosticText());
		}
		return ECefDispatchFactoryResult::FactoryFailed;
	}

	return ECefDispatchFactoryResult::Ok;
}
