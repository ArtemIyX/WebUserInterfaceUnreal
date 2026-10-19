#include "Slate/CefBrowserSurfaceFunctions.h"

#include "Slate/CefBrowserSurfaceConstants.h"
#include "Services/CefFrameReader.h"
#include "GlobalShader.h"
#include "InputCoreTypes.h"
#include "PixelShaderUtils.h"
#include "RenderGraphUtils.h"

class FCefSlateBlitPS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCefSlateBlitPS);
	SHADER_USE_PARAMETER_STRUCT(FCefSlateBlitPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, InputTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, InputSampler)
		SHADER_PARAMETER(FVector2f, DestMin)
		SHADER_PARAMETER(FVector2f, DestSize)
		SHADER_PARAMETER(FVector2f, SrcMin)
		SHADER_PARAMETER(FVector2f, SrcSize)
		SHADER_PARAMETER(FVector2f, SrcTexSize)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(FCefSlateBlitPS, "/Plugin/CefWebUi/Private/CefSlateBlit.usf", "MainPS", SF_Pixel);

namespace CefWebUi::BrowserSurface
{
uint32 BuildCefKeyModifiers(const FKeyEvent& InKeyEvent)
{
	const FModifierKeysState& Mk = InKeyEvent.GetModifierKeys();
	uint32 flags = 0;
	if (Mk.AreCapsLocked())
	{
		flags |= CefEventFlagCapsLockOn;
	}
	if (Mk.IsShiftDown())
	{
		flags |= CefEventFlagShiftDown;
	}
	if (Mk.IsControlDown())
	{
		flags |= CefEventFlagControlDown;
	}
	if (Mk.IsAltDown())
	{
		flags |= CefEventFlagAltDown;
	}
	if (Mk.IsCommandDown())
	{
		flags |= CefEventFlagCommandDown;
	}

	const FKey key = InKeyEvent.GetKey();
	if (key.IsModifierKey())
	{
		if (key == EKeys::LeftShift || key == EKeys::LeftControl || key == EKeys::LeftAlt || key == EKeys::LeftCommand)
		{
			flags |= CefEventFlagIsLeft;
		}
		else if (key == EKeys::RightShift || key == EKeys::RightControl || key == EKeys::RightAlt || key == EKeys::RightCommand)
		{
			flags |= CefEventFlagIsRight;
		}
	}

	if (InKeyEvent.IsRepeat())
	{
		flags |= CefEventFlagIsRepeat;
	}

	if (key.IsGamepadKey())
	{
		return flags;
	}

	if (key == EKeys::NumPadZero || key == EKeys::NumPadOne || key == EKeys::NumPadTwo ||
		key == EKeys::NumPadThree || key == EKeys::NumPadFour || key == EKeys::NumPadFive ||
		key == EKeys::NumPadSix || key == EKeys::NumPadSeven || key == EKeys::NumPadEight ||
		key == EKeys::NumPadNine || key == EKeys::Multiply || key == EKeys::Add ||
		key == EKeys::Subtract || key == EKeys::Decimal || key == EKeys::Divide)
	{
		flags |= CefEventFlagIsKeyPad;
		flags |= CefEventFlagNumLockOn;
	}

	return flags;
}

FIntRect MakeIntersection(const FIntRect& InA, const FIntRect& InB)
{
	FIntRect Result = InA;
	Result.Clip(InB);
	return Result;
}

FIntRect ClampSourceRect(const FIntRect& InSourceRect, const FIntPoint& InSourceExtent)
{
	const int32 ExtentX = FMath::Max(0, InSourceExtent.X);
	const int32 ExtentY = FMath::Max(0, InSourceExtent.Y);
	const int32 MinX = FMath::Clamp(InSourceRect.Min.X, 0, ExtentX);
	const int32 MinY = FMath::Clamp(InSourceRect.Min.Y, 0, ExtentY);
	const int32 MaxX = FMath::Clamp(InSourceRect.Max.X, 0, ExtentX);
	const int32 MaxY = FMath::Clamp(InSourceRect.Max.Y, 0, ExtentY);
	return FIntRect(MinX, MinY, FMath::Max(MinX, MaxX), FMath::Max(MinY, MaxY));
}

FIntRect MapSourceRectToDestRect(
	const FIntRect& InSourceRect,
	const FIntRect& InSourceExtent,
	const FIntRect& InDestRect)
{
	const int32 SourceWidth = InSourceExtent.Width();
	const int32 SourceHeight = InSourceExtent.Height();
	if (SourceWidth <= 0 || SourceHeight <= 0 || InDestRect.Width() <= 0 || InDestRect.Height() <= 0)
	{
		return FIntRect(0, 0, 0, 0);
	}

	const float ScaleX = static_cast<float>(InDestRect.Width()) / static_cast<float>(SourceWidth);
	const float ScaleY = static_cast<float>(InDestRect.Height()) / static_cast<float>(SourceHeight);
	return FIntRect(
		InDestRect.Min.X + FMath::RoundToInt(static_cast<float>(InSourceRect.Min.X - InSourceExtent.Min.X) * ScaleX),
		InDestRect.Min.Y + FMath::RoundToInt(static_cast<float>(InSourceRect.Min.Y - InSourceExtent.Min.Y) * ScaleY),
		InDestRect.Min.X + FMath::RoundToInt(static_cast<float>(InSourceRect.Max.X - InSourceExtent.Min.X) * ScaleX),
		InDestRect.Min.Y + FMath::RoundToInt(static_cast<float>(InSourceRect.Max.Y - InSourceExtent.Min.Y) * ScaleY));
}

FIntPoint ComputePhysicalTargetSize(const FVector2D& InRenderSize)
{
	return FIntPoint(
		FMath::Max(1, FMath::RoundToInt(FMath::Max(0.0f, InRenderSize.X))),
		FMath::Max(1, FMath::RoundToInt(FMath::Max(0.0f, InRenderSize.Y))));
}

bool SanitizeSharedFrameMetadata(FCefSharedFrame& InOutFrame)
{
	if (InOutFrame.Width == 0 || InOutFrame.Height == 0 ||
		InOutFrame.Width > static_cast<uint32>(MAX_int32) ||
		InOutFrame.Height > static_cast<uint32>(MAX_int32) ||
		InOutFrame.SlotCount == 0 || InOutFrame.SlotCount > CEF_SHM_MAX_SLOTS ||
		InOutFrame.WriteSlot >= InOutFrame.SlotCount)
	{
		return false;
	}

	const FIntPoint Extent(static_cast<int32>(InOutFrame.Width), static_cast<int32>(InOutFrame.Height));
	auto SanitizeRect = [&Extent](FCefDirtyRect& InOutRect)
	{
		if (InOutRect.W < 0 || InOutRect.H < 0)
		{
			return false;
		}

		const int64 EndX = static_cast<int64>(InOutRect.X) + static_cast<int64>(InOutRect.W);
		const int64 EndY = static_cast<int64>(InOutRect.Y) + static_cast<int64>(InOutRect.H);
		const int32 MinX = FMath::Clamp(InOutRect.X, 0, Extent.X);
		const int32 MinY = FMath::Clamp(InOutRect.Y, 0, Extent.Y);
		const int32 MaxX = FMath::Clamp(EndX, int64(0), static_cast<int64>(Extent.X));
		const int32 MaxY = FMath::Clamp(EndY, int64(0), static_cast<int64>(Extent.Y));
		InOutRect.X = MinX;
		InOutRect.Y = MinY;
		InOutRect.W = FMath::Max(0, MaxX - MinX);
		InOutRect.H = FMath::Max(0, MaxY - MinY);
		return true;
	};

	if (!SanitizeRect(InOutFrame.PopupRect))
	{
		return false;
	}
	InOutFrame.DirtyCount = FMath::Min<uint8>(InOutFrame.DirtyCount, static_cast<uint8>(MAX_CEF_DIRTY_RECTS));
	for (uint8 Index = 0; Index < InOutFrame.DirtyCount; ++Index)
	{
		if (!SanitizeRect(InOutFrame.DirtyRects[Index]))
		{
			return false;
		}
	}

	if (static_cast<uint8>(InOutFrame.CursorType) >= static_cast<uint8>(ECefCustomCursorType::CT_NUM_VALUES))
	{
		InOutFrame.CursorType = ECefCustomCursorType::CT_NONE;
	}
	if (static_cast<uint8>(InOutFrame.LoadState) > static_cast<uint8>(ECefLoadState::Error))
	{
		InOutFrame.LoadState = ECefLoadState::Idle;
	}
	return true;
}

uint64 AdvanceResourceGeneration(uint64 InGeneration)
{
	return (InGeneration == MAX_uint64) ? 1 : InGeneration + 1;
}

void AddCefSlateBlitPass(
	FRDGBuilder& OutGraphBuilder,
	FRDGTextureRef InputTexture,
	FRDGTextureRef OutputTexture,
	const FIntRect& InMappingDestRect,
	const FIntRect& InDrawRect,
	const FIntRect& InSrcRect,
	const FIntPoint& InSrcExtent,
	ECefSlateBlitPlane InPlane)
{
	const FIntRect SourceRect = ClampSourceRect(InSrcRect, InSrcExtent);
	if (!InputTexture || !OutputTexture ||
		InMappingDestRect.Width() <= 0 || InMappingDestRect.Height() <= 0 ||
		InDrawRect.Width() <= 0 || InDrawRect.Height() <= 0 ||
		SourceRect.Width() <= 0 || SourceRect.Height() <= 0)
	{
		return;
	}

	TShaderMapRef<FCefSlateBlitPS> pixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	FCefSlateBlitPS::FParameters* PassParameters = OutGraphBuilder.AllocParameters<FCefSlateBlitPS::FParameters>();
	PassParameters->InputTexture = OutGraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(InputTexture));
	PassParameters->InputSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
	PassParameters->DestMin = FVector2f(static_cast<float>(InMappingDestRect.Min.X), static_cast<float>(InMappingDestRect.Min.Y));
	PassParameters->DestSize = FVector2f(static_cast<float>(InMappingDestRect.Width()), static_cast<float>(InMappingDestRect.Height()));
	PassParameters->SrcMin = FVector2f(static_cast<float>(SourceRect.Min.X), static_cast<float>(SourceRect.Min.Y));
	PassParameters->SrcSize = FVector2f(static_cast<float>(SourceRect.Width()), static_cast<float>(SourceRect.Height()));
	PassParameters->SrcTexSize = FVector2f(static_cast<float>(FMath::Max(1, InSrcExtent.X)), static_cast<float>(FMath::Max(1, InSrcExtent.Y)));
	PassParameters->RenderTargets[0] = FRenderTargetBinding(OutputTexture, ERenderTargetLoadAction::ELoad);
	const TCHAR* PassName = (InPlane == ECefSlateBlitPlane::Main) ? TEXT("CefSlateBlit Main") : TEXT("CefSlateBlit Popup");

	FPixelShaderUtils::AddFullscreenPass(
		OutGraphBuilder,
		GetGlobalShaderMap(GMaxRHIFeatureLevel),
		RDG_EVENT_NAME("%s", PassName),
		pixelShader,
		PassParameters,
		InDrawRect);
}
}
