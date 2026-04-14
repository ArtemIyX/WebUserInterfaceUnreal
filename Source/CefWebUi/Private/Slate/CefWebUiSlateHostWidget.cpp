#include "Slate/CefWebUiSlateHostWidget.h"

#include "Slate/CefBrowserSurface.h"
#include "Sessions/CefWebUiBrowserSession.h"

void UCefWebUiSlateHostWidget::SetBrowserSession(UCefWebUiBrowserSession* InBrowserSession)
{
	BrowserSession = InBrowserSession;
	if (BrowserSurfaceWidget.IsValid())
	{
		BrowserSurfaceWidget->SetBrowserSession(BrowserSession);
	}
}

void UCefWebUiSlateHostWidget::SetBrowserSize(int32 InBrowserWidth, int32 InBrowserHeight)
{
	BrowserWidth = FMath::Max(1, InBrowserWidth);
	BrowserHeight = FMath::Max(1, InBrowserHeight);
	if (BrowserSurfaceWidget.IsValid())
	{
		BrowserSurfaceWidget->SetBrowserSize(BrowserWidth, BrowserHeight);
	}
}

TSharedRef<SWidget> UCefWebUiSlateHostWidget::RebuildWidget()
{
	SAssignNew(BrowserSurfaceWidget, SCefBrowserSurface)
		.BrowserSession(BrowserSession)
		.BrowserWidth(BrowserWidth)
		.BrowserHeight(BrowserHeight);

	return BrowserSurfaceWidget.ToSharedRef();
}

void UCefWebUiSlateHostWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	BrowserSurfaceWidget.Reset();
}

void UCefWebUiSlateHostWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (BrowserSurfaceWidget.IsValid())
	{
		BrowserSurfaceWidget->SetBrowserSession(BrowserSession);
		BrowserSurfaceWidget->SetBrowserSize(BrowserWidth, BrowserHeight);
	}
}
