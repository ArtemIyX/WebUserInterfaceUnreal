#include "Slate/CefBrowserSurface.h"

#include "InputCoreTypes.h"
#include "Sessions/CefWebUiBrowserSession.h"
#include "Services/CefInputWriter.h"

void SCefBrowserSurface::Construct(const FArguments& InArgs)
{
	BrowserSession = InArgs._BrowserSession;
	BrowserWidth = FMath::Max(1, InArgs._BrowserWidth);
	BrowserHeight = FMath::Max(1, InArgs._BrowserHeight);
}

void SCefBrowserSurface::SetBrowserSession(TWeakObjectPtr<UCefWebUiBrowserSession> InBrowserSession)
{
	BrowserSession = InBrowserSession;
}

void SCefBrowserSurface::SetBrowserSize(int32 InBrowserWidth, int32 InBrowserHeight)
{
	BrowserWidth = FMath::Max(1, InBrowserWidth);
	BrowserHeight = FMath::Max(1, InBrowserHeight);
}

int32 SCefBrowserSurface::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	// Step 1 only: input-capable surface. Rendering path comes next.
	return LayerId;
}

FVector2D SCefBrowserSurface::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(static_cast<float>(BrowserWidth), static_cast<float>(BrowserHeight));
}

FReply SCefBrowserSurface::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<FCefInputWriter> inputWriter;
	if (!TryGetInputWriter(inputWriter))
	{
		return FReply::Unhandled();
	}

	int32 x = 0;
	int32 y = 0;
	GetBrowserCoords(MyGeometry, MouseEvent.GetScreenSpacePosition(), x, y);
	inputWriter->WriteMouseMove(x, y);
	return FReply::Handled();
}

FReply SCefBrowserSurface::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<FCefInputWriter> inputWriter;
	if (!TryGetInputWriter(inputWriter))
	{
		return FReply::Unhandled();
	}

	ECefMouseButton button = ECefMouseButton::Left;
	if (!SlateButtonToCef(MouseEvent.GetEffectingButton(), button))
	{
		return FReply::Unhandled();
	}

	int32 x = 0;
	int32 y = 0;
	GetBrowserCoords(MyGeometry, MouseEvent.GetScreenSpacePosition(), x, y);
	inputWriter->WriteMouseButton(x, y, button, false);
	return FReply::Handled().CaptureMouse(AsShared());
}

FReply SCefBrowserSurface::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<FCefInputWriter> inputWriter;
	if (!TryGetInputWriter(inputWriter))
	{
		return FReply::Unhandled();
	}

	ECefMouseButton button = ECefMouseButton::Left;
	if (!SlateButtonToCef(MouseEvent.GetEffectingButton(), button))
	{
		return FReply::Unhandled();
	}

	int32 x = 0;
	int32 y = 0;
	GetBrowserCoords(MyGeometry, MouseEvent.GetScreenSpacePosition(), x, y);
	inputWriter->WriteMouseButton(x, y, button, true);
	return FReply::Handled().ReleaseMouseCapture();
}

FReply SCefBrowserSurface::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<FCefInputWriter> inputWriter;
	if (!TryGetInputWriter(inputWriter))
	{
		return FReply::Unhandled();
	}

	int32 x = 0;
	int32 y = 0;
	GetBrowserCoords(MyGeometry, MouseEvent.GetScreenSpacePosition(), x, y);
	inputWriter->WriteMouseScroll(x, y, 0.0f, MouseEvent.GetWheelDelta() * 120.0f);
	return FReply::Handled();
}

FReply SCefBrowserSurface::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	TSharedPtr<FCefInputWriter> inputWriter;
	if (!TryGetInputWriter(inputWriter))
	{
		return FReply::Unhandled();
	}

	const uint32 modifiers = KeyEvent.GetModifierKeys().IsLeftControlDown() ? 0x0002u : 0u;
	inputWriter->WriteKey(KeyEvent.GetKeyCode(), modifiers, true);
	return FReply::Handled();
}

FReply SCefBrowserSurface::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	TSharedPtr<FCefInputWriter> inputWriter;
	if (!TryGetInputWriter(inputWriter))
	{
		return FReply::Unhandled();
	}

	const uint32 modifiers = KeyEvent.GetModifierKeys().IsLeftControlDown() ? 0x0002u : 0u;
	inputWriter->WriteKey(KeyEvent.GetKeyCode(), modifiers, false);
	return FReply::Handled();
}

FReply SCefBrowserSurface::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& CharacterEvent)
{
	TSharedPtr<FCefInputWriter> inputWriter;
	if (!TryGetInputWriter(inputWriter))
	{
		return FReply::Unhandled();
	}

	inputWriter->WriteChar(static_cast<uint16>(CharacterEvent.GetCharacter()));
	return FReply::Handled();
}

bool SCefBrowserSurface::TryGetInputWriter(TSharedPtr<FCefInputWriter>& OutInputWriter) const
{
	const TWeakObjectPtr<UCefWebUiBrowserSession> session = BrowserSession;
	if (!session.IsValid())
	{
		return false;
	}

	OutInputWriter = session->GetInputWriterPtr().Pin();
	return OutInputWriter.IsValid();
}

void SCefBrowserSurface::GetBrowserCoords(const FGeometry& InGeometry, const FVector2D& InScreenPosition, int32& OutX, int32& OutY) const
{
	const FVector2D localPos = InGeometry.AbsoluteToLocal(InScreenPosition);
	const FVector2D localSize = InGeometry.GetLocalSize();
	const float width = FMath::Max(localSize.X, 1.0f);
	const float height = FMath::Max(localSize.Y, 1.0f);
	OutX = FMath::Clamp(FMath::RoundToInt(localPos.X / width * BrowserWidth), 0, BrowserWidth - 1);
	OutY = FMath::Clamp(FMath::RoundToInt(localPos.Y / height * BrowserHeight), 0, BrowserHeight - 1);
}

bool SCefBrowserSurface::SlateButtonToCef(const FKey& InKey, ECefMouseButton& OutButton)
{
	if (InKey == EKeys::LeftMouseButton)
	{
		OutButton = ECefMouseButton::Left;
		return true;
	}
	if (InKey == EKeys::MiddleMouseButton)
	{
		OutButton = ECefMouseButton::Middle;
		return true;
	}
	if (InKey == EKeys::RightMouseButton)
	{
		OutButton = ECefMouseButton::Right;
		return true;
	}
	return false;
}
