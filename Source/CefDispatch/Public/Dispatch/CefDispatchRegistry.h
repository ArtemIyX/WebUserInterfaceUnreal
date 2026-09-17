/**
 * @file CefDispatch\Public\Dispatch\CefDispatchRegistry.h
 * @brief Declares CefDispatchRegistry for module CefDispatch\Public\Dispatch\CefDispatchRegistry.h.
 * @details Contains dispatch registry and value plumbing used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Dispatch/CefDispatchRouteKey.h"
#include "Dispatch/CefDispatchValue.h"

/** @brief Type declaration. */
enum class ECefDispatchFactoryResult : uint8
{
	Ok,
	RouteNotFound,
	FactoryFailed,
	InvalidFactory,
	InvalidRouteKey
};

inline FString ToString(ECefDispatchFactoryResult InEnumValue)
{
	switch (InEnumValue)
	{
		case ECefDispatchFactoryResult::Ok:
			return "Ok";
		case ECefDispatchFactoryResult::RouteNotFound:
			return "RouteNotFound";
		case ECefDispatchFactoryResult::FactoryFailed:
			return "FactoryFailed";
		case ECefDispatchFactoryResult::InvalidFactory:
			return "InvalidFactory";
		case ECefDispatchFactoryResult::InvalidRouteKey:
			return "InvalidRouteKey";
		default: ;
	}
	return "INVALID";
}

/** @brief Type declaration. */
class CEFDISPATCH_API FCefDispatchRegistry
{
public:
	using FCefDispatchFactory = TFunction<TUniquePtr<ICefDispatchValue>(const FCefDispatchRouteKey& InRouteKey, const TArray<uint8>& InPayload, FString& OutError)>;

	/** @brief RegisterFactory API. */
	bool RegisterFactory(const FCefDispatchRouteKey& InRouteKey, FCefDispatchFactory InFactory, bool bInAllowReplace = false);

	template <typename TKey>
		requires (!std::is_same_v<std::decay_t<TKey>, FCefDispatchRouteKey>)
	bool RegisterFactory(TKey&& InKey, FCefDispatchFactory InFactory, bool bInAllowReplace = false)
	{
		return RegisterFactory(MakeCefDispatchRouteKey(Forward<TKey>(InKey)), MoveTemp(InFactory), bInAllowReplace);
	}

	/** @brief UnregisterFactory API. */
	bool UnregisterFactory(const FCefDispatchRouteKey& InRouteKey);

	template <typename TKey>
		requires (!std::is_same_v<std::decay_t<TKey>, FCefDispatchRouteKey>)
	bool UnregisterFactory(TKey&& InKey) { return UnregisterFactory(MakeCefDispatchRouteKey(Forward<TKey>(InKey))); }

	/** @brief HasFactory API. */
	bool HasFactory(const FCefDispatchRouteKey& InRouteKey) const;

	template <typename TKey>
		requires (!std::is_same_v<std::decay_t<TKey>, FCefDispatchRouteKey>)
	bool HasFactory(TKey&& InKey) const { return HasFactory(MakeCefDispatchRouteKey(Forward<TKey>(InKey))); }

	/** @brief GetFactoryCount API. */
	int32 GetFactoryCount() const;

	ECefDispatchFactoryResult Decode(const FCefDispatchRouteKey& InRouteKey, const TArray<uint8>& InPayload,
		TUniquePtr<ICefDispatchValue>& OutValue, FString& OutError) const;

	template <typename TKey>
		requires (!std::is_same_v<std::decay_t<TKey>, FCefDispatchRouteKey>)
	ECefDispatchFactoryResult Decode(TKey&& InKey, const TArray<uint8>& InPayload,
		TUniquePtr<ICefDispatchValue>& OutValue, FString& OutError) const
	{
		return Decode(MakeCefDispatchRouteKey(Forward<TKey>(InKey)), InPayload, OutValue, OutError);
	}

private:
	/** @brief RoutesLock state. */
	mutable FRWLock RoutesLock;
	/** @brief Routes state. */
	TMap<FCefDispatchRouteKey, FCefDispatchFactory> Routes;
};