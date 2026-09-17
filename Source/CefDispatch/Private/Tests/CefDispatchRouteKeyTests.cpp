#include "Misc/AutomationTest.h"
#include "Dispatch/CefDispatchHandlerRegistry.h"
#include "Dispatch/CefDispatchRouteKey.h"

namespace
{
	struct FTestRoute
	{
		uint8 Group = 0;
		FName Name;

		bool operator==(const FTestRoute&) const = default;
	};

	uint32 GetTypeHash(const FTestRoute& InRoute)
	{
		return HashCombine(::GetTypeHash(InRoute.Group), ::GetTypeHash(InRoute.Name.GetComparisonIndex().ToUnstableInt()));
	}

	struct FOtherRoute
	{
		uint32 Value = 0;
		bool operator==(const FOtherRoute&) const = default;
	};

	uint32 GetTypeHash(const FOtherRoute& InRoute)
	{
		return ::GetTypeHash(InRoute.Value);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefDispatchRouteKeyPrimitiveTest, "CefDispatch.RouteKey.Primitive", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCefDispatchRouteKeyPrimitiveTest::RunTest(const FString& Parameters)
{
	const FCefDispatchRouteKey scalar = MakeCefDispatchRouteKey(7u);
	const FCefDispatchRouteKey sameScalar = MakeCefDispatchRouteKey(7u);
	const FCefDispatchRouteKey otherScalar = MakeCefDispatchRouteKey(8u);
	const FCefDispatchRouteKey route = MakeCefDispatchRouteKey(FTestRoute{2, TEXT("B_1_1")});
	const FCefDispatchRouteKey sameRoute = MakeCefDispatchRouteKey(FTestRoute{2, TEXT("B_1_1")});
	const FCefDispatchRouteKey differentRoute = MakeCefDispatchRouteKey(FTestRoute{2, TEXT("B_1_2")});
	const FCefDispatchRouteKey differentType = MakeCefDispatchRouteKey(FOtherRoute{7});

	TestTrue(TEXT("Scalar key is valid"), scalar.IsValid());
	TestTrue(TEXT("Equal scalar keys compare equal"), scalar == sameScalar);
	TestEqual(TEXT("Equal scalar keys have equal hashes"), GetTypeHash(scalar), GetTypeHash(sameScalar));
	TestFalse(TEXT("Different scalar values do not compare equal"), scalar == otherScalar);
	TestTrue(TEXT("Equal composite keys compare equal"), route == sameRoute);
	TestFalse(TEXT("Different composite fields do not compare equal"), route == differentRoute);
	TestFalse(TEXT("Different key types do not compare equal"), scalar == differentType);
	TestEqual(TEXT("Composite key can be inspected"), route.TryGet<FTestRoute>()->Name, FName(TEXT("B_1_1")));
	TestNull(TEXT("Wrong key type is rejected"), route.TryGet<FOtherRoute>());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefDispatchRouteKeyLifetimeTest, "CefDispatch.RouteKey.CopyLifetime", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCefDispatchRouteKeyLifetimeTest::RunTest(const FString& Parameters)
{
	FCefDispatchRouteKey copiedKey;
	{
		FTestRoute source{3, TEXT("temporary")};
		copiedKey = MakeCefDispatchRouteKey(source);
		source.Name = TEXT("changed");
	}

	const FTestRoute* retainedRoute = copiedKey.TryGet<FTestRoute>();
	TestNotNull(TEXT("Copied key retains owned value"), retainedRoute);
	if (retainedRoute)
	{
		TestEqual(TEXT("Copied key is independent of source"), retainedRoute->Name, FName(TEXT("temporary")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefDispatchRegistryRoutingTest, "CefDispatch.Registry.Routing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCefDispatchRegistryRoutingTest::RunTest(const FString& Parameters)
{
	FCefDispatchRegistry registry;
	const FTestRoute routeValue{1, TEXT("A_1")};
	int32 factoryCalls = 0;
	const bool registered = registry.RegisterFactory(routeValue,
		[&factoryCalls](const FCefDispatchRouteKey& InRouteKey, const TArray<uint8>& InPayload, FString& OutError)
		{
			++factoryCalls;
			return MakeCefDispatchValue(InRouteKey.TryGet<FTestRoute>()->Name);
		});

	TestTrue(TEXT("Composite factory registers"), registered);
	TestTrue(TEXT("Composite factory is found"), registry.HasFactory(FTestRoute{1, TEXT("A_1")}));
	TestFalse(TEXT("Duplicate factory is rejected"), registry.RegisterFactory(routeValue, FCefDispatchRegistry::FCefDispatchFactory()));

	TUniquePtr<ICefDispatchValue> value;
	FString error;
	TestEqual(TEXT("Factory decodes route"), registry.Decode(routeValue, {}, value, error), ECefDispatchFactoryResult::Ok);
	TestEqual(TEXT("Factory receives complete route"), factoryCalls, 1);
	TestEqual(TEXT("Factory returns expected value"), CefDispatchTryGetValue<FName>(*value)->GetValue(), FName(TEXT("A_1")));
	TestFalse(TEXT("Default key is not found"), registry.HasFactory(FCefDispatchRouteKey()));
	TestEqual(TEXT("Invalid key is rejected by decode"), registry.Decode(FCefDispatchRouteKey(), {}, value, error), ECefDispatchFactoryResult::InvalidRouteKey);
	TestTrue(TEXT("Factory unregisters"), registry.UnregisterFactory(routeValue));
	TestEqual(TEXT("Factory count is zero"), registry.GetFactoryCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefDispatchHandlerRoutingTest, "CefDispatch.Handler.Routing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCefDispatchHandlerRoutingTest::RunTest(const FString& Parameters)
{
	FCefDispatchHandlerRegistry handlers;
	const FTestRoute routeValue{4, TEXT("B_1_1")};
	bool reentered = false;
	TestTrue(TEXT("Typed handler registers"), handlers.RegisterTypedHandler<FString>(routeValue,
		[&handlers, &reentered](const FCefDispatchRouteKey& InRouteKey, const FString& InValue)
		{
			reentered = handlers.RegisterTypedHandler<FString>(FTestRoute{4, TEXT("reentry")}, [](const FString&) {});
			return InRouteKey.TryGet<FTestRoute>() && InValue == TEXT("payload");
		}));

	const TUniquePtr<ICefDispatchValue> value = MakeCefDispatchValue(FString(TEXT("payload")));
	FString error;
	TestEqual(TEXT("Handler receives route and value"), handlers.Handle(routeValue, *value, error), ECefDispatchHandlerResult::Ok);
	TestTrue(TEXT("Handler may reenter registry after lookup"), reentered);
	TestEqual(TEXT("Wrong payload reports type mismatch"), handlers.Handle(routeValue, *MakeCefDispatchValue(7u), error), ECefDispatchHandlerResult::HandlerTypeMismatch);
	TestEqual(TEXT("Invalid handler key is rejected"), handlers.Handle(FCefDispatchRouteKey(), *value, error), ECefDispatchHandlerResult::InvalidRouteKey);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefDispatchMetadataRoutingTest, "CefDispatch.Handler.Metadata", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCefDispatchMetadataRoutingTest::RunTest(const FString& Parameters)
{
	FCefDispatchHandlerRegistry handlers;
	const FTestRoute routeValue{5, TEXT("metadata")};
	const FTestRoute legacyRoute{5, TEXT("legacy")};
	FString receivedGuid;

	TestTrue(TEXT("Metadata handler registers"), handlers.RegisterTypedHandler<FString>(routeValue,
		[&receivedGuid](const FString& InValue, const ICefDispatchMetadata* InMetadata)
		{
			const FString* guid = CefDispatchTryGetMetadata<FString>(InMetadata);
			if (guid)
			{
				receivedGuid = *guid;
			}
			return guid && InValue == TEXT("payload");
		}));
	TestTrue(TEXT("Legacy handler registers"), handlers.RegisterTypedHandler<FString>(legacyRoute,
		[](const FString& InValue)
		{
			return InValue == TEXT("payload");
		}));

	const TUniquePtr<ICefDispatchValue> value = MakeCefDispatchValue(FString(TEXT("payload")));
	const TSharedPtr<const ICefDispatchMetadata> metadata = MakeCefDispatchMetadata(FString(TEXT("test-guid")));
	FString error;

	TestEqual(TEXT("Metadata handler receives metadata"), handlers.Handle(routeValue, *value, metadata, error), ECefDispatchHandlerResult::Ok);
	TestEqual(TEXT("Metadata value is preserved"), receivedGuid, FString(TEXT("test-guid")));
	TestEqual(TEXT("Legacy handler call without metadata succeeds"), handlers.Handle(legacyRoute, *value, error), ECefDispatchHandlerResult::Ok);
	return true;
}
