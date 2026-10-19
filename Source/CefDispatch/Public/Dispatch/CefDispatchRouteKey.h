/**
 * @file CefDispatch\Public\Dispatch\CefDispatchRouteKey.h
 * @brief Type-erased immutable keys used by CefDispatch route registries.
 */
#pragma once

#include "CoreMinimal.h"
#include <concepts>
#include <type_traits>

class FCefDispatchRouteKey;
template <typename TKey>
const std::decay_t<TKey>* CefDispatchTryGetRouteKey(const FCefDispatchRouteKey& InRouteKey);

template <typename TKey>
struct TCefDispatchRouteKeyTraits
{
	using FKeyType = std::decay_t<TKey>;

	static uint32 GetHash(const FKeyType& InKey)
	{
		return GetTypeHash(InKey);
	}

	static bool Equals(const FKeyType& InLeft, const FKeyType& InRight)
	{
		return InLeft == InRight;
	}
};

namespace CefDispatchRouteKeyPrivate
{
	template <typename TKey>
	const void* GetTypeToken()
	{
		static const uint8 Token = 0;
		return &Token;
	}

	class IStorage
	{
	public:
		virtual ~IStorage() = default;
		virtual const void* GetTypeToken() const = 0;
		virtual uint32 GetValueHash() const = 0;
		virtual bool Equals(const IStorage& InOther) const = 0;
		virtual FString GetDescription() const = 0;
	};

	template <typename TKey>
	class TStorage final : public IStorage
	{
	public:
		using FKeyType = std::decay_t<TKey>;

		explicit TStorage(FKeyType InKey)
			: Key(MoveTemp(InKey))
		{
			ValueHash = TCefDispatchRouteKeyTraits<FKeyType>::GetHash(Key);
		}

		virtual const void* GetTypeToken() const override
		{
			return CefDispatchRouteKeyPrivate::GetTypeToken<FKeyType>();
		}

		virtual uint32 GetValueHash() const override
		{
			return ValueHash;
		}

		virtual bool Equals(const IStorage& InOther) const override
		{
			if (InOther.GetTypeToken() != GetTypeToken())
			{
				return false;
			}

			const TStorage& other = static_cast<const TStorage&>(InOther);
			return TCefDispatchRouteKeyTraits<FKeyType>::Equals(Key, other.Key);
		}

		virtual FString GetDescription() const override
		{
			if constexpr (requires(const FKeyType& InValue) { TCefDispatchRouteKeyTraits<FKeyType>::Describe(InValue); })
			{
				return TCefDispatchRouteKeyTraits<FKeyType>::Describe(Key).Left(128);
			}
			return FString();
		}

		const FKeyType& GetKey() const
		{
			return Key;
		}

	private:
		const FKeyType Key;
		uint32 ValueHash = 0;
	};
}

class CEFDISPATCH_API FCefDispatchRouteKey
{
public:
	FCefDispatchRouteKey() = default;

	explicit FCefDispatchRouteKey(TSharedPtr<const CefDispatchRouteKeyPrivate::IStorage> InStorage)
		: Storage(MoveTemp(InStorage))
	{
	}

	bool IsValid() const
	{
		return Storage.IsValid();
	}

	uint32 GetValueHash() const
	{
		return Storage.IsValid() ? Storage->GetValueHash() : 0;
	}

	const void* GetTypeIdentity() const
	{
		return Storage.IsValid() ? Storage->GetTypeToken() : nullptr;
	}

	FString GetDiagnosticDescription() const
	{
		return Storage.IsValid() ? Storage->GetDescription() : FString();
	}

	FString GetDiagnosticText() const
	{
		if (!IsValid())
		{
			return TEXT("invalid");
		}
		const FString description = GetDiagnosticDescription();
		const uint32 routeHash = HashCombine(GetValueHash(), PointerHash(GetTypeIdentity()));
		return description.IsEmpty()
			? FString::Printf(TEXT("hash=%u"), routeHash)
			: FString::Printf(TEXT("hash=%u description=%s"), routeHash, *description);
	}

	template <typename TKey>
	const std::decay_t<TKey>* TryGet() const;

	bool operator==(const FCefDispatchRouteKey& InOther) const
	{
		return Storage.IsValid() && InOther.Storage.IsValid() &&
			Storage->GetTypeToken() == InOther.Storage->GetTypeToken() && Storage->Equals(*InOther.Storage);
	}

private:
	template <typename TKey>
	friend FCefDispatchRouteKey MakeCefDispatchRouteKey(TKey&& InKey);
	template <typename TKey>
	friend const std::decay_t<TKey>* CefDispatchTryGetRouteKey(const FCefDispatchRouteKey& InRouteKey);

	TSharedPtr<const CefDispatchRouteKeyPrivate::IStorage> Storage;
};

inline uint32 GetTypeHash(const FCefDispatchRouteKey& InRouteKey)
{
	return HashCombine(InRouteKey.GetValueHash(), PointerHash(InRouteKey.GetTypeIdentity()));
}

template <typename TKey>
requires requires(const std::decay_t<TKey>& InKey)
{
	TCefDispatchRouteKeyTraits<std::decay_t<TKey>>::GetHash(InKey);
	{ TCefDispatchRouteKeyTraits<std::decay_t<TKey>>::Equals(InKey, InKey) } -> std::convertible_to<bool>;
}
FCefDispatchRouteKey MakeCefDispatchRouteKey(TKey&& InKey)
{
	using FKeyType = std::decay_t<TKey>;
	return FCefDispatchRouteKey(MakeShared<const CefDispatchRouteKeyPrivate::TStorage<FKeyType>>(Forward<TKey>(InKey)));
}

template <typename TKey>
const std::decay_t<TKey>* CefDispatchTryGetRouteKey(const FCefDispatchRouteKey& InRouteKey)
{
	using FKeyType = std::decay_t<TKey>;
	if (!InRouteKey.IsValid() || InRouteKey.GetTypeIdentity() != CefDispatchRouteKeyPrivate::GetTypeToken<FKeyType>())
	{
		return nullptr;
	}
	return &static_cast<const CefDispatchRouteKeyPrivate::TStorage<FKeyType>*>(InRouteKey.Storage.Get())->GetKey();
}

template <typename TKey>
const std::decay_t<TKey>* FCefDispatchRouteKey::TryGet() const
{
	return CefDispatchTryGetRouteKey<TKey>(*this);
}
