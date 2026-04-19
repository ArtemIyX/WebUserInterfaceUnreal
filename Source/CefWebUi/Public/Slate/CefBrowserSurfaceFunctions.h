#pragma once

#include "CoreMinimal.h"
#include "Input/Events.h"
#include "RenderGraphFwd.h"

namespace CefWebUi::BrowserSurface
{
uint32 BuildCefKeyModifiers(const FKeyEvent& InKeyEvent);
FIntRect MakeIntersection(const FIntRect& InA, const FIntRect& InB);
void AddCefSlateBlitPass(
	FRDGBuilder& OutGraphBuilder,
	FRDGTextureRef InputTexture,
	FRDGTextureRef OutputTexture,
	const FIntRect& InDestRect,
	const FIntRect& InSrcRect,
	const FIntPoint& InSrcExtent);
}
