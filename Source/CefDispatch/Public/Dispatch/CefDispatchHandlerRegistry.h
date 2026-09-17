/**
 * @file CefDispatch\Public\Dispatch\CefDispatchHandlerRegistry.h
 * @brief Declares CefDispatchHandlerRegistry for module CefDispatch.
 * @details Contains typed handler routing built on top of FCefDispatchRegistry decode results.
 */
#pragma once

#include "CoreMinimal.h"
#include "Dispatch/CefDispatchMetadata.h"
#include "Dispatch/CefDispatchRegistry.h"
#include "Dispatch/CefDispatchValue.h"
#include <functional>
#include <type_traits>
#include <utility>

/** @brief Result of attempting to decode and invoke a handler. */
enum class ECefDispatchHandlerResult : uint8
{
	Ok,
	DecodeRegistryUnavailable,
	DecodeRouteNotFound,
	DecodeFailed,
	HandlerNotFound,
	HandlerTypeMismatch,
	HandlerFailed,
	InvalidHandler,
	InvalidRouteKey
};

inline FString ToString(ECefDispatchHandlerResult InValue)
{
	switch (InValue)
	{
		case ECefDispatchHandlerResult::Ok:
			return "Ok";
		case ECefDispatchHandlerResult::DecodeRegistryUnavailable:
			return "DecodeRegistryUnavailable";
		case ECefDispatchHandlerResult::DecodeRouteNotFound:
			return "DecodeRouteNotFound";
		case ECefDispatchHandlerResult::DecodeFailed:
			return "DecodeFailed";
		case ECefDispatchHandlerResult::HandlerNotFound:
			return "HandlerNotFound";
		case ECefDispatchHandlerResult::HandlerTypeMismatch:
			return "HandlerTypeMismatch";
		case ECefDispatchHandlerResult::HandlerFailed:
			return "HandlerFailed";
		case ECefDispatchHandlerResult::InvalidHandler:
			return "InvalidHandler";
		case ECefDispatchHandlerResult::InvalidRouteKey:
			return "InvalidRouteKey";
		default:
			return "INVALID";
	}
}

/** @brief Type declaration. */
class CEFDISPATCH_API FCefDispatchHandlerRegistry
{
public:
	using FCefDispatchHandler = TFunction<bool(const FCefDispatchRouteKey& InRouteKey, const ICefDispatchValue& InValue, FString& OutError)>;
	using FCefDispatchMetadataHandler = TFunction<bool(const FCefDispatchRouteKey& InRouteKey, const ICefDispatchValue& InValue, const ICefDispatchMetadata* InMetadata, FString& OutError)>;

	FCefDispatchHandlerRegistry() = default;

	explicit FCefDispatchHandlerRegistry(TSharedPtr<FCefDispatchRegistry> InDecodeRegistry)
		: DecodeRegistry(MoveTemp(InDecodeRegistry)) {}

	void SetDecodeRegistry(TSharedPtr<FCefDispatchRegistry> InDecodeRegistry)
	{
		FWriteScopeLock writeLock(DecodeRegistryLock);
		DecodeRegistry = MoveTemp(InDecodeRegistry);
	}

	TSharedPtr<FCefDispatchRegistry> GetDecodeRegistry() const
	{
		FReadScopeLock readLock(DecodeRegistryLock);
		return DecodeRegistry;
	}

	bool RegisterHandler(const FCefDispatchRouteKey& InRouteKey, FCefDispatchHandler InHandler, bool bInAllowReplace = false);
	bool RegisterHandler(const FCefDispatchRouteKey& InRouteKey, FCefDispatchMetadataHandler InHandler, bool bInAllowReplace = false);

	template <typename TKey>
		requires (!std::is_same_v<std::decay_t<TKey>, FCefDispatchRouteKey>)
	bool RegisterHandler(TKey&& InKey, FCefDispatchHandler InHandler, bool bInAllowReplace = false) { return RegisterHandler(MakeCefDispatchRouteKey(Forward<TKey>(InKey)), MoveTemp(InHandler), bInAllowReplace); }

	template <typename TKey>
		requires (!std::is_same_v<std::decay_t<TKey>, FCefDispatchRouteKey>)
	bool RegisterHandler(TKey&& InKey, FCefDispatchMetadataHandler InHandler, bool bInAllowReplace = false) { return RegisterHandler(MakeCefDispatchRouteKey(Forward<TKey>(InKey)), MoveTemp(InHandler), bInAllowReplace); }

	bool UnregisterHandler(const FCefDispatchRouteKey& InRouteKey);

	template <typename TKey>
		requires (!std::is_same_v<std::decay_t<TKey>, FCefDispatchRouteKey>)
	bool UnregisterHandler(TKey&& InKey) { return UnregisterHandler(MakeCefDispatchRouteKey(Forward<TKey>(InKey))); }

	bool HasHandler(const FCefDispatchRouteKey& InRouteKey) const;

	template <typename TKey>
		requires (!std::is_same_v<std::decay_t<TKey>, FCefDispatchRouteKey>)
	bool HasHandler(TKey&& InKey) const { return HasHandler(MakeCefDispatchRouteKey(Forward<TKey>(InKey))); }

	int32 GetHandlerCount() const;

	ECefDispatchHandlerResult Handle(const FCefDispatchRouteKey& InRouteKey, const ICefDispatchValue& InValue, FString& OutError) const;
	ECefDispatchHandlerResult Handle(const FCefDispatchRouteKey& InRouteKey, const ICefDispatchValue& InValue,
		TSharedPtr<const ICefDispatchMetadata> InMetadata, FString& OutError) const;

	template <typename TKey>
		requires (!std::is_same_v<std::decay_t<TKey>, FCefDispatchRouteKey>)
	ECefDispatchHandlerResult Handle(TKey&& InKey, const ICefDispatchValue& InValue, FString& OutError) const { return Handle(MakeCefDispatchRouteKey(Forward<TKey>(InKey)), InValue, OutError); }

	template <typename TKey>
		requires (!std::is_same_v<std::decay_t<TKey>, FCefDispatchRouteKey>)
	ECefDispatchHandlerResult Handle(TKey&& InKey, const ICefDispatchValue& InValue,
		TSharedPtr<const ICefDispatchMetadata> InMetadata, FString& OutError) const { return Handle(MakeCefDispatchRouteKey(Forward<TKey>(InKey)), InValue, MoveTemp(InMetadata), OutError); }

	ECefDispatchHandlerResult Dispatch(const FCefDispatchRouteKey& InRouteKey, const TArray<uint8>& InPayload, FString& OutError) const;
	ECefDispatchHandlerResult Dispatch(const FCefDispatchRouteKey& InRouteKey, const TArray<uint8>& InPayload,
		TSharedPtr<const ICefDispatchMetadata> InMetadata, FString& OutError) const;

	template <typename TKey>
		requires (!std::is_same_v<std::decay_t<TKey>, FCefDispatchRouteKey>)
	ECefDispatchHandlerResult Dispatch(TKey&& InKey, const TArray<uint8>& InPayload, FString& OutError) const { return Dispatch(MakeCefDispatchRouteKey(Forward<TKey>(InKey)), InPayload, OutError); }

	template <typename TKey>
		requires (!std::is_same_v<std::decay_t<TKey>, FCefDispatchRouteKey>)
	ECefDispatchHandlerResult Dispatch(TKey&& InKey, const TArray<uint8>& InPayload,
		TSharedPtr<const ICefDispatchMetadata> InMetadata, FString& OutError) const { return Dispatch(MakeCefDispatchRouteKey(Forward<TKey>(InKey)), InPayload, MoveTemp(InMetadata), OutError); }

	template <typename T, typename CallableType>
	bool RegisterTypedHandler(const FCefDispatchRouteKey& InRouteKey, CallableType&& InHandler, bool bInAllowReplace = false)
	{
		return RegisterHandler(InRouteKey, MakeTypedHandler<T>(Forward<CallableType>(InHandler)), bInAllowReplace);
	}

	template <typename T, typename TKey, typename CallableType>
		requires (!std::is_same_v<std::decay_t<TKey>, FCefDispatchRouteKey>)
	bool RegisterTypedHandler(TKey&& InKey, CallableType&& InHandler, bool bInAllowReplace = false)
	{
		return RegisterHandler(MakeCefDispatchRouteKey(Forward<TKey>(InKey)), MakeTypedHandler<T>(Forward<CallableType>(InHandler)), bInAllowReplace);
	}

	template <typename T, typename CallableType>
	static FCefDispatchMetadataHandler MakeTypedHandler(CallableType&& InHandler)
	{
		using FDecayedCallableType = std::decay_t<CallableType>;

		return [Handler = FDecayedCallableType(Forward<CallableType>(InHandler))](
			const FCefDispatchRouteKey& InRouteKey,
			const ICefDispatchValue& InValue,
			const ICefDispatchMetadata* InMetadata,
			FString& OutError) mutable -> bool {
			const TCefDispatchValue<T>* typedValue = CefDispatchTryGetValue<T>(InValue);
			if (!typedValue)
			{
				OutError = TEXT("CefDispatch.TypedHandler.TypeMismatch");
				return false;
			}

			const T& value = typedValue->GetValue();

			if constexpr (std::is_invocable_r_v<bool, FDecayedCallableType&, const FCefDispatchRouteKey&, const T&, const ICefDispatchMetadata*, FString&>)
			{
				return std::invoke(Handler, InRouteKey, value, InMetadata, OutError);
			}
			else if constexpr (std::is_invocable_r_v<bool, FDecayedCallableType&, const T&, const ICefDispatchMetadata*, FString&>)
			{
				return std::invoke(Handler, value, InMetadata, OutError);
			}
			else if constexpr (std::is_invocable_r_v<bool, FDecayedCallableType&, const FCefDispatchRouteKey&, const T&, const ICefDispatchMetadata*>)
			{
				return std::invoke(Handler, InRouteKey, value, InMetadata);
			}
			else if constexpr (std::is_invocable_r_v<bool, FDecayedCallableType&, const T&, const ICefDispatchMetadata*>)
			{
				return std::invoke(Handler, value, InMetadata);
			}
			else if constexpr (std::is_invocable_r_v<bool, FDecayedCallableType&, const FCefDispatchRouteKey&, const T&, FString&>)
			{
				return std::invoke(Handler, InRouteKey, value, OutError);
			}
			else if constexpr (std::is_invocable_r_v<bool, FDecayedCallableType&, const T&, FString&>)
			{
				return std::invoke(Handler, value, OutError);
			}
			else if constexpr (std::is_invocable_r_v<bool, FDecayedCallableType&, const FCefDispatchRouteKey&, const T&>)
			{
				return std::invoke(Handler, InRouteKey, value);
			}
			else if constexpr (std::is_invocable_r_v<bool, FDecayedCallableType&, const T&>)
			{
				return std::invoke(Handler, value);
			}
			else if constexpr (std::is_invocable_v<FDecayedCallableType&, const FCefDispatchRouteKey&, const T&, const ICefDispatchMetadata*, FString&>)
			{
				std::invoke(Handler, InRouteKey, value, InMetadata, OutError);
				return true;
			}
			else if constexpr (std::is_invocable_v<FDecayedCallableType&, const T&, const ICefDispatchMetadata*, FString&>)
			{
				std::invoke(Handler, value, InMetadata, OutError);
				return true;
			}
			else if constexpr (std::is_invocable_v<FDecayedCallableType&, const FCefDispatchRouteKey&, const T&, const ICefDispatchMetadata*>)
			{
				std::invoke(Handler, InRouteKey, value, InMetadata);
				return true;
			}
			else if constexpr (std::is_invocable_v<FDecayedCallableType&, const T&, const ICefDispatchMetadata*>)
			{
				std::invoke(Handler, value, InMetadata);
				return true;
			}
			else if constexpr (std::is_invocable_v<FDecayedCallableType&, const FCefDispatchRouteKey&, const T&, FString&>)
			{
				std::invoke(Handler, InRouteKey, value, OutError);
				return true;
			}
			else if constexpr (std::is_invocable_v<FDecayedCallableType&, const T&, FString&>)
			{
				std::invoke(Handler, value, OutError);
				return true;
			}
			else if constexpr (std::is_invocable_v<FDecayedCallableType&, const FCefDispatchRouteKey&, const T&>)
			{
				std::invoke(Handler, InRouteKey, value);
				return true;
			}
			else if constexpr (std::is_invocable_v<FDecayedCallableType&, const T&>)
			{
				std::invoke(Handler, value);
				return true;
			}
			else
			{
				static_assert(
					std::is_invocable_v<FDecayedCallableType&, const T&> ||
					std::is_invocable_v<FDecayedCallableType&, const T&, const ICefDispatchMetadata*> ||
					std::is_invocable_v<FDecayedCallableType&, const FCefDispatchRouteKey&, const T&> ||
					std::is_invocable_v<FDecayedCallableType&, const FCefDispatchRouteKey&, const T&, const ICefDispatchMetadata*> ||
					std::is_invocable_v<FDecayedCallableType&, const T&, FString&> ||
					std::is_invocable_v<FDecayedCallableType&, const T&, const ICefDispatchMetadata*, FString&> ||
					std::is_invocable_v<FDecayedCallableType&, const FCefDispatchRouteKey&, const T&, FString&> ||
					std::is_invocable_v<FDecayedCallableType&, const FCefDispatchRouteKey&, const T&, const ICefDispatchMetadata*, FString&>,
					"Typed dispatch handlers must accept one of: (const T&), (const FCefDispatchRouteKey&, const T&), (const T&, FString&), or (const FCefDispatchRouteKey&, const T&, FString&), optionally returning bool.");
				return false;
			}
		};
	}

private:
	mutable FRWLock DecodeRegistryLock;
	TSharedPtr<FCefDispatchRegistry> DecodeRegistry;

	mutable FRWLock HandlersLock;
	TMap<FCefDispatchRouteKey, FCefDispatchMetadataHandler> Handlers;
};
