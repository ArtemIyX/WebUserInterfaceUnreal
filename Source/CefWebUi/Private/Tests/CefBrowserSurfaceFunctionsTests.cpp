#include "Misc/AutomationTest.h"

#include "Services/CefFrameReader.h"
#include "Slate/CefBrowserSurfaceFunctions.h"

namespace
{
	void TestRect(FAutomationTestBase& InTest, const TCHAR* InName, const FIntRect& InActual, const FIntRect& InExpected)
	{
		InTest.TestEqual(FString::Printf(TEXT("%s min x"), InName), InActual.Min.X, InExpected.Min.X);
		InTest.TestEqual(FString::Printf(TEXT("%s min y"), InName), InActual.Min.Y, InExpected.Min.Y);
		InTest.TestEqual(FString::Printf(TEXT("%s max x"), InName), InActual.Max.X, InExpected.Max.X);
		InTest.TestEqual(FString::Printf(TEXT("%s max y"), InName), InActual.Max.Y, InExpected.Max.Y);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCefBrowserSurfaceRectangleTest,
	"CefWebUi.Unit.BrowserSurface.Rectangles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCefBrowserSurfaceRectangleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace CefWebUi::BrowserSurface;

	TestRect(*this, TEXT("intersection"), MakeIntersection(FIntRect(0, 0, 100, 100), FIntRect(40, 20, 140, 80)), FIntRect(40, 20, 100, 80));
	TestRect(*this, TEXT("clamp"), ClampSourceRect(FIntRect(-10, 20, 120, 80), FIntPoint(100, 64)), FIntRect(0, 20, 100, 64));
	TestRect(
		*this,
		TEXT("mapping"),
		MapSourceRectToDestRect(FIntRect(10, 20, 30, 40), FIntRect(0, 0, 100, 100), FIntRect(100, 200, 500, 600)),
		FIntRect(140, 280, 220, 360));

	const FIntPoint physicalSize = ComputePhysicalTargetSize(FVector2D(127.4f, 63.6f));
	TestEqual(TEXT("physical width"), physicalSize.X, 127);
	TestEqual(TEXT("physical height"), physicalSize.Y, 64);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCefBrowserSurfaceFrameMetadataTest,
	"CefWebUi.Unit.BrowserSurface.FrameMetadata",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCefBrowserSurfaceFrameMetadataTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace CefWebUi::BrowserSurface;

	FCefSharedFrame frame;
	frame.Width = 128;
	frame.Height = 64;
	frame.SlotCount = 2;
	frame.WriteSlot = 1;
	frame.PopupRect = { -8, -4, 40, 24 };
	frame.DirtyCount = 1;
	frame.DirtyRects[0] = { 100, 50, 64, 32 };
	frame.CursorType = static_cast<ECefCustomCursorType>(255);
	frame.LoadState = static_cast<ECefLoadState>(255);

	TestTrue(TEXT("valid metadata is accepted"), SanitizeSharedFrameMetadata(frame));
	TestEqual(TEXT("popup x is clamped"), frame.PopupRect.X, 0);
	TestEqual(TEXT("popup width is clamped"), frame.PopupRect.W, 32);
	TestEqual(TEXT("dirty x is clamped"), frame.DirtyRects[0].X, 100);
	TestEqual(TEXT("dirty width is clamped"), frame.DirtyRects[0].W, 28);
	TestEqual(TEXT("invalid cursor maps to none"), frame.CursorType, ECefCustomCursorType::CT_NONE);
	TestEqual(TEXT("invalid load state maps to idle"), frame.LoadState, ECefLoadState::Idle);

	FCefSharedFrame widescreenFrame = frame;
	widescreenFrame.Width = 5120;
	widescreenFrame.Height = 1440;
	TestTrue(TEXT("5120x1440 metadata is accepted"), SanitizeSharedFrameMetadata(widescreenFrame));

	FCefSharedFrame invalidFrame = frame;
	invalidFrame.SlotCount = 4;
	TestFalse(TEXT("unsupported slot count is rejected"), SanitizeSharedFrameMetadata(invalidFrame));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCefBrowserSurfaceGenerationTest,
	"CefWebUi.Unit.BrowserSurface.ResourceGeneration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCefBrowserSurfaceGenerationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace CefWebUi::BrowserSurface;

	TestEqual(TEXT("generation increments"), AdvanceResourceGeneration(7), uint64(8));
	TestEqual(TEXT("generation skips zero"), AdvanceResourceGeneration(MAX_uint64), uint64(1));
	return true;
}