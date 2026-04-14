#include "Slate/CefBrowserSurface.h"

#include "InputCoreTypes.h"
#include "Sessions/CefWebUiBrowserSession.h"
#include "Engine/Texture2D.h"
#include "ID3D12DynamicRHI.h"
#include "RenderingThread.h"
#include "RHI.h"
#include "Services/CefInputWriter.h"
#include "TextureResource.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <d3d12.h>
#include "Windows/HideWindowsPlatformTypes.h"

SCefBrowserSurface::~SCefBrowserSurface()
{
	ReleaseResources();
}

void SCefBrowserSurface::Construct(const FArguments& InArgs)
{
	BrowserSession = InArgs._BrowserSession;
	BrowserWidth = FMath::Max(1, InArgs._BrowserWidth);
	BrowserHeight = FMath::Max(1, InArgs._BrowserHeight);
	RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SCefBrowserSurface::HandleActiveTimer));
}

void SCefBrowserSurface::SetBrowserSession(TWeakObjectPtr<UCefWebUiBrowserSession> InBrowserSession)
{
	BrowserSession = InBrowserSession;
	FrameReader.Reset();
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
	PollLatestFrame();
	if (!bHasFrame || !SlateMainTexture)
	{
		return LayerId;
	}

	constexpr ESlateDrawEffect drawEffects = ESlateDrawEffect::None;
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		&MainBrush,
		drawEffects,
		InWidgetStyle.GetColorAndOpacityTint());

	const bool bHasPopup = LastFrame.bPopupVisible &&
		((LastFrame.Flags & CefFrameFlag_PopupPlane) != 0) &&
		SharedPopupTextureRHI.IsValid() &&
		LastFrame.PopupRect.W > 0 &&
		LastFrame.PopupRect.H > 0;
	if (!bHasPopup)
	{
		return LayerId + 1;
	}

	const FVector2D localSize = AllottedGeometry.GetLocalSize();
	const float safeWidth = FMath::Max(1.0f, localSize.X);
	const float safeHeight = FMath::Max(1.0f, localSize.Y);
	const float sx = safeWidth / FMath::Max(1, static_cast<int32>(LastFrame.Width));
	const float sy = safeHeight / FMath::Max(1, static_cast<int32>(LastFrame.Height));
	const FVector2D popupPos(
		static_cast<float>(LastFrame.PopupRect.X) * sx,
		static_cast<float>(LastFrame.PopupRect.Y) * sy);
	const FVector2D popupSize(
		static_cast<float>(LastFrame.PopupRect.W) * sx,
		static_cast<float>(LastFrame.PopupRect.H) * sy);
	if (popupSize.X <= 0.0f || popupSize.Y <= 0.0f)
	{
		return LayerId + 1;
	}

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(popupSize, FSlateLayoutTransform(popupPos)),
		&PopupBrush,
		drawEffects,
		InWidgetStyle.GetColorAndOpacityTint());
	return LayerId + 2;
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

EActiveTimerReturnType SCefBrowserSurface::HandleActiveTimer(double CurrentTime, float DeltaTime)
{
	Invalidate(EInvalidateWidgetReason::Paint);
	return EActiveTimerReturnType::Continue;
}

bool SCefBrowserSurface::TryGetFrameReader(TSharedPtr<FCefFrameReader>& OutFrameReader) const
{
	if (TSharedPtr<FCefFrameReader> reader = FrameReader.Pin())
	{
		OutFrameReader = reader;
		return true;
	}

	const TWeakObjectPtr<UCefWebUiBrowserSession> session = BrowserSession;
	if (!session.IsValid())
	{
		return false;
	}

	FrameReader = session->GetFrameReaderPtr();
	OutFrameReader = FrameReader.Pin();
	return OutFrameReader.IsValid();
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

void SCefBrowserSurface::PollLatestFrame() const
{
	TSharedPtr<FCefFrameReader> frameReader;
	if (!TryGetFrameReader(frameReader))
	{
		return;
	}

	FCefSharedFrame frame;
	if (!frameReader->PollSharedTexture(frame))
	{
		return;
	}

	LastFrame = frame;
	bHasFrame = true;
	SharedSlotCount = FMath::Clamp(frame.SlotCount, 1u, MaxSharedSlots);
	EnsureSharedRhi();
	EnsureSlateTextures(frame.Width, frame.Height);
	UpdateTextureRefs(frame.WriteSlot % SharedSlotCount, (frame.Flags & CefFrameFlag_PopupPlane) != 0);
}

void SCefBrowserSurface::EnsureSharedRhi() const
{
	bool bReady = true;
	for (uint32 i = 0; i < SharedSlotCount; ++i)
	{
		if (!SharedTextureRHI[i].IsValid())
		{
			bReady = false;
			break;
		}
	}
	if (bReady)
	{
		return;
	}

	ENQUEUE_RENDER_COMMAND(CefSlateOpenSharedTextures)(
		[this](FRHICommandListImmediate&)
		{
			ID3D12DynamicRHI* d3d12Rhi = GetID3D12DynamicRHI();
			if (!d3d12Rhi)
			{
				return;
			}

			ID3D12Device* d3dDevice = d3d12Rhi->RHIGetDevice(0);
			if (!d3dDevice)
			{
				return;
			}

			for (uint32 i = 0; i < SharedSlotCount; ++i)
			{
				if (SharedTextureRHI[i].IsValid())
				{
					continue;
				}

				wchar_t name[64];
				swprintf(name, 64, L"Global\\CEFHost_SharedTex_%u", i);
				HANDLE sharedHandle = nullptr;
				HRESULT hr = d3dDevice->OpenSharedHandleByName(name, GENERIC_ALL, &sharedHandle);
				if (FAILED(hr) || !sharedHandle)
				{
					return;
				}

				ID3D12Resource* d3dResource = nullptr;
				hr = d3dDevice->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&d3dResource));
				CloseHandle(sharedHandle);
				if (FAILED(hr) || !d3dResource)
				{
					return;
				}

				SharedTextureRHI[i] = d3d12Rhi->RHICreateTexture2DFromResource(
					PF_B8G8R8A8,
					ETextureCreateFlags::External | ETextureCreateFlags::ShaderResource,
					FClearValueBinding::None,
					d3dResource);
				d3dResource->Release();
			}
		});
}

bool SCefBrowserSurface::EnsurePopupPlaneRhi() const
{
	if (SharedPopupTextureRHI.IsValid())
	{
		return true;
	}

	ENQUEUE_RENDER_COMMAND(CefSlateOpenPopupTexture)(
		[this](FRHICommandListImmediate&)
		{
			if (SharedPopupTextureRHI.IsValid())
			{
				return;
			}

			ID3D12DynamicRHI* d3d12Rhi = GetID3D12DynamicRHI();
			if (!d3d12Rhi)
			{
				return;
			}

			ID3D12Device* d3dDevice = d3d12Rhi->RHIGetDevice(0);
			if (!d3dDevice)
			{
				return;
			}

			HANDLE sharedHandle = nullptr;
			HRESULT hr = d3dDevice->OpenSharedHandleByName(L"Global\\CEFHost_SharedPopupTex", GENERIC_ALL, &sharedHandle);
			if (FAILED(hr) || !sharedHandle)
			{
				return;
			}

			ID3D12Resource* d3dResource = nullptr;
			hr = d3dDevice->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&d3dResource));
			CloseHandle(sharedHandle);
			if (FAILED(hr) || !d3dResource)
			{
				return;
			}

			SharedPopupTextureRHI = d3d12Rhi->RHICreateTexture2DFromResource(
				PF_B8G8R8A8,
				ETextureCreateFlags::External | ETextureCreateFlags::ShaderResource,
				FClearValueBinding::None,
				d3dResource);
			d3dResource->Release();
		});
	return SharedPopupTextureRHI.IsValid();
}

void SCefBrowserSurface::EnsureSlateTextures(uint32 InWidth, uint32 InHeight) const
{
	if (!SlateMainTexture || SlateMainTexture->GetSizeX() != static_cast<int32>(InWidth) || SlateMainTexture->GetSizeY() != static_cast<int32>(InHeight))
	{
		if (SlateMainTexture)
		{
			SlateMainTexture->RemoveFromRoot();
		}
		SlateMainTexture = UTexture2D::CreateTransient(static_cast<int32>(InWidth), static_cast<int32>(InHeight), PF_B8G8R8A8);
		if (SlateMainTexture)
		{
			SlateMainTexture->SRGB = false;
			SlateMainTexture->Filter = TF_Bilinear;
			SlateMainTexture->AddToRoot();
			SlateMainTexture->UpdateResource();
			MainBrush.SetResourceObject(SlateMainTexture);
			MainBrush.ImageSize = FVector2D(static_cast<float>(InWidth), static_cast<float>(InHeight));
			MainBrush.DrawAs = ESlateBrushDrawType::Image;
		}
	}

	if (!SlatePopupTexture || SlatePopupTexture->GetSizeX() != static_cast<int32>(InWidth) || SlatePopupTexture->GetSizeY() != static_cast<int32>(InHeight))
	{
		if (SlatePopupTexture)
		{
			SlatePopupTexture->RemoveFromRoot();
		}
		SlatePopupTexture = UTexture2D::CreateTransient(static_cast<int32>(InWidth), static_cast<int32>(InHeight), PF_B8G8R8A8);
		if (SlatePopupTexture)
		{
			SlatePopupTexture->SRGB = false;
			SlatePopupTexture->Filter = TF_Bilinear;
			SlatePopupTexture->AddToRoot();
			SlatePopupTexture->UpdateResource();
			PopupBrush.SetResourceObject(SlatePopupTexture);
			PopupBrush.ImageSize = FVector2D(static_cast<float>(InWidth), static_cast<float>(InHeight));
			PopupBrush.DrawAs = ESlateBrushDrawType::Image;
		}
	}
}

void SCefBrowserSurface::UpdateTextureRefs(uint32 InSlot, bool bUsePopupPlane) const
{
	if (!SlateMainTexture || !SharedTextureRHI[InSlot].IsValid() || !SlateMainTexture->GetResource())
	{
		return;
	}

	FTextureRHIRef srcMain = SharedTextureRHI[InSlot];
	FTextureReferenceRHIRef mainTextureRef = SlateMainTexture->TextureReference.TextureReferenceRHI;
	ENQUEUE_RENDER_COMMAND(CefSlateUpdateMainTextureRef)(
		[srcMain, mainTextureRef](FRHICommandListImmediate& RHICmdList)
		{
			if (mainTextureRef.IsValid())
			{
				RHICmdList.UpdateTextureReference(mainTextureRef.GetReference(), srcMain.GetReference());
			}
		});

	if (!bUsePopupPlane || !SlatePopupTexture || !SlatePopupTexture->GetResource())
	{
		return;
	}
	if (!EnsurePopupPlaneRhi() || !SharedPopupTextureRHI.IsValid())
	{
		return;
	}

	FTextureRHIRef srcPopup = SharedPopupTextureRHI;
	FTextureReferenceRHIRef popupTextureRef = SlatePopupTexture->TextureReference.TextureReferenceRHI;
	ENQUEUE_RENDER_COMMAND(CefSlateUpdatePopupTextureRef)(
		[srcPopup, popupTextureRef](FRHICommandListImmediate& RHICmdList)
		{
			if (popupTextureRef.IsValid())
			{
				RHICmdList.UpdateTextureReference(popupTextureRef.GetReference(), srcPopup.GetReference());
			}
		});
}

void SCefBrowserSurface::ReleaseResources()
{
	if (SlateMainTexture)
	{
		SlateMainTexture->RemoveFromRoot();
		SlateMainTexture = nullptr;
	}
	if (SlatePopupTexture)
	{
		SlatePopupTexture->RemoveFromRoot();
		SlatePopupTexture = nullptr;
	}

	FTextureRHIRef oldMain0 = SharedTextureRHI[0];
	FTextureRHIRef oldMain1 = SharedTextureRHI[1];
	FTextureRHIRef oldMain2 = SharedTextureRHI[2];
	FTextureRHIRef oldPopup = SharedPopupTextureRHI;
	ENQUEUE_RENDER_COMMAND(CefSlateReleaseSharedTextures)(
		[oldMain0, oldMain1, oldMain2, oldPopup](FRHICommandListImmediate&) mutable
		{
			oldMain0.SafeRelease();
			oldMain1.SafeRelease();
			oldMain2.SafeRelease();
			oldPopup.SafeRelease();
		});

	for (uint32 i = 0; i < MaxSharedSlots; ++i)
	{
		SharedTextureRHI[i] = nullptr;
	}
	SharedPopupTextureRHI = nullptr;
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
