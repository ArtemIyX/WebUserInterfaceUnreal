# CefDispatch

`CefDispatch` is a format-agnostic `MessageType(uint32) -> Factory` module.

## Core
- `FCefDispatchRegistry`
- `ICefDispatchValue`
- `TCefDispatchValue<T>`
- `MakeCefDispatchValue(...)`

Factories return `TUniquePtr<ICefDispatchValue>`, so payload type can be anything:
- protobuf message
- custom struct
- `FString`
- raw bytes wrapper

## Manual Registration
```cpp
TSharedPtr<FCefDispatchRegistry> registry = FCefDispatchModule::Get().GetRegistry();
registry->RegisterFactory(1001,
	[](uint32 InMessageType, const TArray<uint8>& InPayload, FString& OutError) -> TUniquePtr<ICefDispatchValue>
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
	[](uint32 InMessageType, const TArray<uint8>& InPayload, FString& OutError) -> TUniquePtr<ICefDispatchValue>
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

## Typed Handler Registration
```cpp
CEF_DISPATCH_REGISTER_TYPED_HANDLER(2001, FString,
	[](const FString& InText)
	{
		UE_LOG(LogTemp, Log, TEXT("Handled text: %s"), *InText);
	});
```

```cpp
TSharedPtr<FCefDispatchHandlerRegistry> handlerRegistry = FCefDispatchModule::Get().GetHandlerRegistry();
FString error;
const ECefDispatchHandlerResult result = handlerRegistry->Dispatch(2001, bytes, error);
```

Typed handlers can use any of these signatures:
- `void(const T&)`
- `bool(const T&)`
- `void(uint32, const T&)`
- `bool(uint32, const T&)`
- `void(const T&, FString&)`
- `bool(const T&, FString&)`
- `void(uint32, const T&, FString&)`
- `bool(uint32, const T&, FString&)`

## Protobuf Note
For protobuf route factory:
1. parse payload into `MyProto::Message`
2. return `MakeCefDispatchValue(MoveTemp(message))`

No protobuf dependency is required in `CefDispatch` core.
