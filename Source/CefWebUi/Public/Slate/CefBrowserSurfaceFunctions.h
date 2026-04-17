#pragma once

#include "CoreMinimal.h"
#include "Input/Events.h"
#include "RenderGraphFwd.h"

namespace CefWebUi::BrowserSurface
{
uint32 BuildCefKeyModifiers(const FKeyEvent& KeyEvent);
FIntRect MakeIntersection(const FIntRect& A, const FIntRect& B);
void AddCefSlateBlitPass(
	FRDGBuilder& GraphBuilder,
	FRDGTextureRef InputTexture,
	FRDGTextureRef OutputTexture,
	const FIntRect& DestRect,
	const FIntRect& SrcRect,
	const FIntPoint& SrcExtent);
}
