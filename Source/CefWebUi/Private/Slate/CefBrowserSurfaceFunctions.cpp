#include "Slate/CefBrowserSurfaceFunctions.h"

#include "Slate/CefBrowserSurfaceConstants.h"
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
	FIntRect r = InA;
	r.Clip(InB);
	return r;
}

void AddCefSlateBlitPass(
	FRDGBuilder& OutGraphBuilder,
	FRDGTextureRef InputTexture,
	FRDGTextureRef OutputTexture,
	const FIntRect& InDestRect,
	const FIntRect& InSrcRect,
	const FIntPoint& InSrcExtent)
{
	if (!InputTexture || !OutputTexture || InDestRect.Width() <= 0 || InDestRect.Height() <= 0 || InSrcRect.Width() <= 0 || InSrcRect.Height() <= 0)
	{
		return;
	}

	TShaderMapRef<FCefSlateBlitPS> pixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	FCefSlateBlitPS::FParameters* PassParameters = OutGraphBuilder.AllocParameters<FCefSlateBlitPS::FParameters>();
	PassParameters->InputTexture = OutGraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(InputTexture));
	PassParameters->InputSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
	PassParameters->DestMin = FVector2f(static_cast<float>(InDestRect.Min.X), static_cast<float>(InDestRect.Min.Y));
	PassParameters->DestSize = FVector2f(static_cast<float>(InDestRect.Width()), static_cast<float>(InDestRect.Height()));
	PassParameters->SrcMin = FVector2f(static_cast<float>(InSrcRect.Min.X), static_cast<float>(InSrcRect.Min.Y));
	PassParameters->SrcSize = FVector2f(static_cast<float>(InSrcRect.Width()), static_cast<float>(InSrcRect.Height()));
	PassParameters->SrcTexSize = FVector2f(static_cast<float>(FMath::Max(1, InSrcExtent.X)), static_cast<float>(FMath::Max(1, InSrcExtent.Y)));
	PassParameters->RenderTargets[0] = FRenderTargetBinding(OutputTexture, ERenderTargetLoadAction::ELoad);

	FPixelShaderUtils::AddFullscreenPass(
		OutGraphBuilder,
		GetGlobalShaderMap(GMaxRHIFeatureLevel),
		RDG_EVENT_NAME("CefSlateBlit"),
		pixelShader,
		PassParameters,
		InDestRect);
}
}
