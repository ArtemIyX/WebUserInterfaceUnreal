/**
 * @file CefDispatch\Public\Dispatch\CefDispatchValue.h
 * @brief Declares CefDispatchValue for module CefDispatch\Public\Dispatch\CefDispatchValue.h.
 * @details Contains dispatch registry and value plumbing used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include <type_traits>

/** @brief Type declaration. */
struct FCefDispatchTypeId
{
	/** @brief Value state. */
	const void* Value = nullptr;

	bool IsValid() const
	{
		return Value != nullptr;
	}

	bool operator==(const FCefDispatchTypeId& InOther) const
	{
		return Value == InOther.Value;
	}

	bool operator!=(const FCefDispatchTypeId& InOther) const
	{
		return !(*this == InOther);
	}

	template <typename T>
	static FCefDispatchTypeId Of()
	{
		/** @brief token state. */
		static const uint8 token = 0;
		/** @brief outTypeId state. */
		FCefDispatchTypeId outTypeId;
		/** @brief Value state. */
		outTypeId.Value = &token;
		return outTypeId;
	}
};

/** @brief Type declaration. */
class CEFDISPATCH_API ICefDispatchValue
{
public:
	virtual ~ICefDispatchValue() = default;
	/** @brief GetTypeId API. */
	virtual FCefDispatchTypeId GetTypeId() const = 0;
};

template <typename T>
/** @brief Type declaration. */
class TCefDispatchValue final : public ICefDispatchValue
{
public:
	explicit TCefDispatchValue(const T& InValue)
		: Value(InValue)
	{
	}

	explicit TCefDispatchValue(T&& InValue)
		: Value(MoveTemp(InValue))
	{
	}

	virtual FCefDispatchTypeId GetTypeId() const override
	{
		return FCefDispatchTypeId::Of<T>();
	}

	const T& GetValue() const
	{
		return Value;
	}

	T& GetValue()
	{
		return Value;
	}

private:
	/** @brief Value state. */
	T Value;
};

template <typename T>
const TCefDispatchValue<T>* CefDispatchTryGetValue(const ICefDispatchValue& InValue)
{
	if (InValue.GetTypeId() != FCefDispatchTypeId::Of<T>())
	{
		return nullptr;
	}
	return static_cast<const TCefDispatchValue<T>*>(&InValue);
}

template <typename T>
TCefDispatchValue<T>* CefDispatchTryGetValue(ICefDispatchValue& InValue)
{
	if (InValue.GetTypeId() != FCefDispatchTypeId::Of<T>())
	{
		return nullptr;
	}
	return static_cast<TCefDispatchValue<T>*>(&InValue);
}

template <typename T>
TUniquePtr<ICefDispatchValue> MakeCefDispatchValue(T&& InValue)
{
	using FValueType = std::decay_t<T>;
	return MakeUnique<TCefDispatchValue<FValueType>>(Forward<T>(InValue));
}

