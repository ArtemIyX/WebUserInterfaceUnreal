#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "Rendering/RenderingCommon.h"
#include "Widgets/SLeafWidget.h"
#include "Services/CefFrameReader.h"
#include "Services/CefInputWriter.h"

class FCefInputWriter;
class FCefFrameReader;
class UCefWebUiBrowserSession;
class FCefBrowserSurfaceDrawer;

class CEFWEBUI_API SCefBrowserSurface : public SLeafWidget
{
public:
	virtual ~SCefBrowserSurface() override;

	SLATE_BEGIN_ARGS(SCefBrowserSurface)
		: _BrowserWidth(1920)
		, _BrowserHeight(1080)
	{}
		SLATE_ARGUMENT(TWeakObjectPtr<UCefWebUiBrowserSession>, BrowserSession)
		SLATE_ARGUMENT(int32, BrowserWidth)
		SLATE_ARGUMENT(int32, BrowserHeight)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetBrowserSession(TWeakObjectPtr<UCefWebUiBrowserSession> InBrowserSession);
	void SetBrowserSize(int32 InBrowserWidth, int32 InBrowserHeight);

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;
	virtual FReply OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& CharacterEvent) override;

private:
	EActiveTimerReturnType HandleActiveTimer(double CurrentTime, float DeltaTime);
	bool TryGetFrameReader(TSharedPtr<FCefFrameReader>& OutFrameReader) const;
	bool TryGetInputWriter(TSharedPtr<FCefInputWriter>& OutInputWriter) const;
	void PollLatestFrame() const;
	void EnsureSharedRhi() const;
	bool EnsurePopupPlaneRhi() const;
	void ReleaseResources();
	void GetBrowserCoords(const FGeometry& InGeometry, const FVector2D& InScreenPosition, int32& OutX, int32& OutY) const;
	static bool SlateButtonToCef(const FKey& InKey, ECefMouseButton& OutButton);

private:
	static constexpr uint32 MaxSharedSlots = 3;

	TWeakObjectPtr<UCefWebUiBrowserSession> BrowserSession;
	int32 BrowserWidth = 1920;
	int32 BrowserHeight = 1080;

	mutable TWeakPtr<FCefFrameReader> FrameReader;
	mutable FCefSharedFrame LastFrame;
	mutable bool bHasFrame = false;
	mutable uint32 SharedSlotCount = 2;
	mutable FTextureRHIRef SharedTextureRHI[MaxSharedSlots];
	mutable FTextureRHIRef SharedPopupTextureRHI;
	mutable TSharedPtr<FCefBrowserSurfaceDrawer, ESPMode::ThreadSafe> CustomDrawer;
};
