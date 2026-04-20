/**
 * @file CefWebUi\Public\Slate\CefBrowserSurfaceFunctions.h
 * @brief Declares CefBrowserSurfaceFunctions for module CefWebUi\Public\Slate\CefBrowserSurfaceFunctions.h.
 * @details Contains types and APIs used by the plugin runtime and gameplay-facing systems.
 */
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

