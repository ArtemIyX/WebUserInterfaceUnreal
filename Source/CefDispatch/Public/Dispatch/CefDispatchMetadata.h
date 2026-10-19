#pragma once

#include "CoreMinimal.h"
#include "Dispatch/CefDispatchValue.h"
#include <type_traits>

class CEFDISPATCH_API ICefDispatchMetadata
{
public:
	virtual ~ICefDispatchMetadata() = default;
	virtual FCefDispatchTypeId GetTypeId() const = 0;
};

template <typename T>
class TCefDispatchMetadata final : public ICefDispatchMetadata
{
public:
	explicit TCefDispatchMetadata(T InValue)
		: Value(MoveTemp(InValue))
	{
	}

	FCefDispatchTypeId GetTypeId() const override
	{
		return FCefDispatchTypeId::Of<T>();
	}

	const T& GetValue() const
	{
		return Value;
	}

private:
	T Value;
};

template <typename T>
TSharedPtr<const ICefDispatchMetadata> MakeCefDispatchMetadata(T&& InValue)
{
	using FMetadataType = std::decay_t<T>;
	return MakeShared<TCefDispatchMetadata<FMetadataType>>(Forward<T>(InValue));
}

template <typename T>
const T* CefDispatchTryGetMetadata(const ICefDispatchMetadata* InMetadata)
{
	if (!InMetadata || InMetadata->GetTypeId() != FCefDispatchTypeId::Of<T>())
	{
		return nullptr;
	}

	return &static_cast<const TCefDispatchMetadata<T>*>(InMetadata)->GetValue();
}
