/**
 * @file CefDispatch\Public\Dispatch\CefDispatchHandlerRegistry.h
 * @brief Declares CefDispatchHandlerRegistry for module CefDispatch.
 * @details Contains typed handler routing built on top of FCefDispatchRegistry decode results.
 */
#pragma once

#include "CoreMinimal.h"
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
	InvalidHandler
};

/** @brief Type declaration. */
class CEFDISPATCH_API FCefDispatchHandlerRegistry
{
public:
	using FCefDispatchHandler = TFunction<bool(uint32 InMessageType, const ICefDispatchValue& InValue, FString& OutError)>;

	FCefDispatchHandlerRegistry() = default;
	explicit FCefDispatchHandlerRegistry(TSharedPtr<FCefDispatchRegistry> InDecodeRegistry)
		: DecodeRegistry(MoveTemp(InDecodeRegistry))
	{
	}

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

	bool RegisterHandler(uint32 InMessageType, FCefDispatchHandler InHandler, bool bInAllowReplace = false);
	bool UnregisterHandler(uint32 InMessageType);
	bool HasHandler(uint32 InMessageType) const;
	int32 GetHandlerCount() const;

	ECefDispatchHandlerResult Handle(uint32 InMessageType, const ICefDispatchValue& InValue, FString& OutError) const;
	ECefDispatchHandlerResult Dispatch(uint32 InMessageType, const TArray<uint8>& InPayload, FString& OutError) const;

	template <typename T, typename CallableType>
	bool RegisterTypedHandler(uint32 InMessageType, CallableType&& InHandler, bool bInAllowReplace = false)
	{
		return RegisterHandler(InMessageType, MakeTypedHandler<T>(Forward<CallableType>(InHandler)), bInAllowReplace);
	}

	template <typename T, typename CallableType>
	static FCefDispatchHandler MakeTypedHandler(CallableType&& InHandler)
	{
		using FDecayedCallableType = std::decay_t<CallableType>;

		return [Handler = FDecayedCallableType(Forward<CallableType>(InHandler))](
			uint32 InMessageType,
			const ICefDispatchValue& InValue,
			FString& OutError) mutable -> bool
		{
			const TCefDispatchValue<T>* typedValue = CefDispatchTryGetValue<T>(InValue);
			if (!typedValue)
			{
				OutError = FString::Printf(TEXT("Dispatch type mismatch for MessageType=%u"), InMessageType);
				return false;
			}

			const T& value = typedValue->GetValue();

			if constexpr (std::is_invocable_r_v<bool, FDecayedCallableType&, uint32, const T&, FString&>)
			{
				return std::invoke(Handler, InMessageType, value, OutError);
			}
			else if constexpr (std::is_invocable_r_v<bool, FDecayedCallableType&, const T&, FString&>)
			{
				return std::invoke(Handler, value, OutError);
			}
			else if constexpr (std::is_invocable_r_v<bool, FDecayedCallableType&, uint32, const T&>)
			{
				return std::invoke(Handler, InMessageType, value);
			}
			else if constexpr (std::is_invocable_r_v<bool, FDecayedCallableType&, const T&>)
			{
				return std::invoke(Handler, value);
			}
			else if constexpr (std::is_invocable_v<FDecayedCallableType&, uint32, const T&, FString&>)
			{
				std::invoke(Handler, InMessageType, value, OutError);
				return true;
			}
			else if constexpr (std::is_invocable_v<FDecayedCallableType&, const T&, FString&>)
			{
				std::invoke(Handler, value, OutError);
				return true;
			}
			else if constexpr (std::is_invocable_v<FDecayedCallableType&, uint32, const T&>)
			{
				std::invoke(Handler, InMessageType, value);
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
					std::is_invocable_v<FDecayedCallableType&, uint32, const T&> ||
					std::is_invocable_v<FDecayedCallableType&, const T&, FString&> ||
					std::is_invocable_v<FDecayedCallableType&, uint32, const T&, FString&>,
					"Typed dispatch handlers must accept one of: (const T&), (uint32, const T&), (const T&, FString&), or (uint32, const T&, FString&), optionally returning bool.");
				return false;
			}
		};
	}

private:
	mutable FRWLock DecodeRegistryLock;
	TSharedPtr<FCefDispatchRegistry> DecodeRegistry;

	mutable FRWLock HandlersLock;
	TMap<uint32, FCefDispatchHandler> Handlers;
};
