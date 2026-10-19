/**
 * @file CefWebUi\Public\Slate\CefBrowserSurfaceFunctions.h
 * @brief Declares CefBrowserSurfaceFunctions for module CefWebUi\Public\Slate\CefBrowserSurfaceFunctions.h.
 * @details Contains types and APIs used by the plugin runtime and gameplay-facing systems.
 */
#pragma once

#include "CoreMinimal.h"
#include "Input/Events.h"
#include "RenderGraphFwd.h"

struct FCefSharedFrame;

namespace CefWebUi::BrowserSurface
{
enum class ECefSlateBlitPlane : uint8
{
	Main,
	Popup,
};

/** @brief BuildCefKeyModifiers API. */
uint32 BuildCefKeyModifiers(const FKeyEvent& InKeyEvent);
/** @brief MakeIntersection API. */
FIntRect MakeIntersection(const FIntRect& InA, const FIntRect& InB);
FIntRect ClampSourceRect(const FIntRect& InSourceRect, const FIntPoint& InSourceExtent);
FIntRect MapSourceRectToDestRect(
	const FIntRect& InSourceRect,
	const FIntRect& InSourceExtent,
	const FIntRect& InDestRect);
FIntPoint ComputePhysicalTargetSize(const FVector2D& InRenderSize);
bool SanitizeSharedFrameMetadata(FCefSharedFrame& InOutFrame);
uint64 AdvanceResourceGeneration(uint64 InGeneration);
void AddCefSlateBlitPass(
	FRDGBuilder& OutGraphBuilder,
	FRDGTextureRef InputTexture,
	FRDGTextureRef OutputTexture,
	const FIntRect& InMappingDestRect,
	const FIntRect& InDrawRect,
	const FIntRect& InSrcRect,
	const FIntPoint& InSrcExtent,
	ECefSlateBlitPlane InPlane);
}

