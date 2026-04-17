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
uint32 BuildCefKeyModifiers(const FKeyEvent& KeyEvent)
{
	const FModifierKeysState& Mk = KeyEvent.GetModifierKeys();
	uint32 Flags = 0;
	if (Mk.AreCapsLocked())
	{
		Flags |= CefEventFlagCapsLockOn;
	}
	if (Mk.IsShiftDown())
	{
		Flags |= CefEventFlagShiftDown;
	}
	if (Mk.IsControlDown())
	{
		Flags |= CefEventFlagControlDown;
	}
	if (Mk.IsAltDown())
	{
		Flags |= CefEventFlagAltDown;
	}
	if (Mk.IsCommandDown())
	{
		Flags |= CefEventFlagCommandDown;
	}

	const FKey Key = KeyEvent.GetKey();
	if (Key.IsModifierKey())
	{
		if (Key == EKeys::LeftShift || Key == EKeys::LeftControl || Key == EKeys::LeftAlt || Key == EKeys::LeftCommand)
		{
			Flags |= CefEventFlagIsLeft;
		}
		else if (Key == EKeys::RightShift || Key == EKeys::RightControl || Key == EKeys::RightAlt || Key == EKeys::RightCommand)
		{
			Flags |= CefEventFlagIsRight;
		}
	}

	if (KeyEvent.IsRepeat())
	{
		Flags |= CefEventFlagIsRepeat;
	}

	if (Key.IsGamepadKey())
	{
		return Flags;
	}

	if (Key == EKeys::NumPadZero || Key == EKeys::NumPadOne || Key == EKeys::NumPadTwo ||
		Key == EKeys::NumPadThree || Key == EKeys::NumPadFour || Key == EKeys::NumPadFive ||
		Key == EKeys::NumPadSix || Key == EKeys::NumPadSeven || Key == EKeys::NumPadEight ||
		Key == EKeys::NumPadNine || Key == EKeys::Multiply || Key == EKeys::Add ||
		Key == EKeys::Subtract || Key == EKeys::Decimal || Key == EKeys::Divide)
	{
		Flags |= CefEventFlagIsKeyPad;
		Flags |= CefEventFlagNumLockOn;
	}

	return Flags;
}

FIntRect MakeIntersection(const FIntRect& A, const FIntRect& B)
{
	FIntRect R = A;
	R.Clip(B);
	return R;
}

void AddCefSlateBlitPass(
	FRDGBuilder& GraphBuilder,
	FRDGTextureRef InputTexture,
	FRDGTextureRef OutputTexture,
	const FIntRect& DestRect,
	const FIntRect& SrcRect,
	const FIntPoint& SrcExtent)
{
	if (!InputTexture || !OutputTexture || DestRect.Width() <= 0 || DestRect.Height() <= 0 || SrcRect.Width() <= 0 || SrcRect.Height() <= 0)
	{
		return;
	}

	TShaderMapRef<FCefSlateBlitPS> PixelShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	FCefSlateBlitPS::FParameters* PassParameters = GraphBuilder.AllocParameters<FCefSlateBlitPS::FParameters>();
	PassParameters->InputTexture = GraphBuilder.CreateSRV(FRDGTextureSRVDesc::Create(InputTexture));
	PassParameters->InputSampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
	PassParameters->DestMin = FVector2f(static_cast<float>(DestRect.Min.X), static_cast<float>(DestRect.Min.Y));
	PassParameters->DestSize = FVector2f(static_cast<float>(DestRect.Width()), static_cast<float>(DestRect.Height()));
	PassParameters->SrcMin = FVector2f(static_cast<float>(SrcRect.Min.X), static_cast<float>(SrcRect.Min.Y));
	PassParameters->SrcSize = FVector2f(static_cast<float>(SrcRect.Width()), static_cast<float>(SrcRect.Height()));
	PassParameters->SrcTexSize = FVector2f(static_cast<float>(FMath::Max(1, SrcExtent.X)), static_cast<float>(FMath::Max(1, SrcExtent.Y)));
	PassParameters->RenderTargets[0] = FRenderTargetBinding(OutputTexture, ERenderTargetLoadAction::ELoad);

	FPixelShaderUtils::AddFullscreenPass(
		GraphBuilder,
		GetGlobalShaderMap(GMaxRHIFeatureLevel),
		RDG_EVENT_NAME("CefSlateBlit"),
		PixelShader,
		PassParameters,
		DestRect);
}
}
