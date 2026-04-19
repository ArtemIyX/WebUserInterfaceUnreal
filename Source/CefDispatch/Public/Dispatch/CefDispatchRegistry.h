#pragma once

#include "CoreMinimal.h"
#include "Dispatch/CefDispatchValue.h"

enum class ECefDispatchFactoryResult : uint8
{
	Ok,
	RouteNotFound,
	FactoryFailed,
	InvalidFactory
};

class CEFDISPATCH_API FCefDispatchRegistry
{
public:
	using FCefDispatchFactory = TFunction<TUniquePtr<ICefDispatchValue>(uint32 InMessageType, const TArray<uint8>& InPayload, FString& OutError)>;

	bool RegisterFactory(uint32 InMessageType, FCefDispatchFactory InFactory, bool bInAllowReplace = false);
	bool UnregisterFactory(uint32 InMessageType);
	bool HasFactory(uint32 InMessageType) const;
	int32 GetFactoryCount() const;

	ECefDispatchFactoryResult Decode(uint32 InMessageType, const TArray<uint8>& InPayload,
	                                 TUniquePtr<ICefDispatchValue>& OutValue, FString& OutError) const;

private:
	mutable FRWLock RoutesLock;
	TMap<uint32, FCefDispatchFactory> Routes;
};
