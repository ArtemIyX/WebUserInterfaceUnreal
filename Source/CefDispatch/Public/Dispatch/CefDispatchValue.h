#pragma once

#include "CoreMinimal.h"

struct FCefDispatchTypeId
{
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
		static const uint8 token = 0;
		FCefDispatchTypeId outTypeId;
		outTypeId.Value = &token;
		return outTypeId;
	}
};

class CEFDISPATCH_API ICefDispatchValue
{
public:
	virtual ~ICefDispatchValue() = default;
	virtual FCefDispatchTypeId GetTypeId() const = 0;
};

template <typename T>
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
