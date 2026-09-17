# CefDispatch

`CefDispatch` provides format-agnostic exact route-key -> factory and typed handler routing.

## Core
- `FCefDispatchRegistry`
- `ICefDispatchValue`
- `TCefDispatchValue<T>`
- `MakeCefDispatchValue(...)`

`FCefDispatchRegistry` is shared by the module and stores message factories. Handler registries are not global. Each server object must create its own `FCefDispatchHandlerRegistry`, pass the shared decode registry to it, and register its handlers during server initialization.

Factories return `TUniquePtr<ICefDispatchValue>`, so payload type can be anything:
- protobuf message
- custom struct
- `FString`
- raw bytes wrapper

## Manual Registration
```cpp
TSharedPtr<FCefDispatchRegistry> registry = FCefDispatchModule::Get().GetDispatchRegistry();
registry->RegisterFactory(1001,
	[](const FCefDispatchRouteKey& InRouteKey, const TArray<uint8>& InPayload, FString& OutError) -> TUniquePtr<ICefDispatchValue>
	{
		FString text;
		FUTF8ToTCHAR converter(reinterpret_cast<const ANSICHAR*>(InPayload.GetData()), InPayload.Num());
		text = FString(converter.Length(), converter.Get());
		return MakeCefDispatchValue(MoveTemp(text));
	});
```

## Semi-Auto Registration
```cpp
CEF_DISPATCH_REGISTER_FACTORY(2001,
	[](const FCefDispatchRouteKey& InRouteKey, const TArray<uint8>& InPayload, FString& OutError) -> TUniquePtr<ICefDispatchValue>
	{
		struct FMyPayload
		{
			int32 Id = 0;
			FString Name;
		};

		FMyPayload payload;
		payload.Id = InPayload.Num();
		payload.Name = TEXT("demo");
		return MakeCefDispatchValue(MoveTemp(payload));
	});
```

## Decode + Typed Read
```cpp
TUniquePtr<ICefDispatchValue> value;
FString error;
const ECefDispatchFactoryResult result = registry->Decode(2001, bytes, value, error);
if (result == ECefDispatchFactoryResult::Ok && value.IsValid())
{
	if (const TCefDispatchValue<FString>* textValue = CefDispatchTryGetValue<FString>(*value))
	{
		UE_LOG(LogTemp, Log, TEXT("Text: %s"), *textValue->GetValue());
	}
}
```

## Server-Owned Handler Registration
```cpp
// Server initialization.
HandlerRegistry = MakeShared<FCefDispatchHandlerRegistry>(GetDispatchRegistry());
RegisterHandlers();

void RegisterHandlers()
{
	HandlerRegistry->RegisterTypedHandler<FString>(2001,
		[](const FString& InText)
		{
			// Handle the message for this server instance.
		});
}
```

Dispatch messages through that same server-owned registry:

```cpp
FString error;
const ECefDispatchHandlerResult result = HandlerRegistry->Dispatch(2001, bytes, error);
```

Typed handlers can use any of these signatures:
- `void(const T&)`
- `bool(const T&)`
- `void(const FCefDispatchRouteKey&, const T&)`
- `bool(const FCefDispatchRouteKey&, const T&)`
- `void(const T&, FString&)`
- `bool(const T&, FString&)`
- `void(const FCefDispatchRouteKey&, const T&, FString&)`
- `bool(const FCefDispatchRouteKey&, const T&, FString&)`

## Protobuf Note
For protobuf route factory:
1. parse payload into `MyProto::Message`
2. return `MakeCefDispatchValue(MoveTemp(message))`

No protobuf dependency is required in `CefDispatch` core.

## Route keys

Pass a scalar such as `1001` to preserve existing routes, or pass any copyable value
type with `GetTypeHash` and `operator==`:

```cpp
struct FMessageRoute { EMessageKind Kind; FName Subtype; bool operator==(const FMessageRoute&) const = default; };
uint32 GetTypeHash(const FMessageRoute& InRoute) { return HashCombine(GetTypeHash(InRoute.Kind), GetTypeHash(InRoute.Subtype)); }
registry->RegisterFactory(FMessageRoute{EMessageKind::B, TEXT("B_1_1")}, Factory);
```

`FGameplayTag` can be used by a consumer module that declares its own `GameplayTags`
dependency. Keys are immutable value identities. Do not use raw UObject pointers,
transient addresses, or mutable external state. Matching is exact and has no implicit
parent or wildcard fallback.

The transport codec parses its envelope, constructs the agreed C++ route key, then
calls `Dispatch`. `FCefDispatchRouteKey` intentionally has no serialization API.
