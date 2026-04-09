// UCefBrowserWidget.cpp

#include "Widgets/CefWebUiBrowserWidget.h"

#include "CefWebUi.h"
#include "Services/CefFrameReader.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "RHIResources.h"
#include "Services/CefInputWriter.h"
#include "InputCoreTypes.h"

#include "ID3D12DynamicRHI.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include "Windows/HideWindowsPlatformTypes.h"

DECLARE_CYCLE_STAT(TEXT("CefWidget: PollFrame"), STAT_CefWidget_PollFrame, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("CefWidget: BindShared"), STAT_CefWidget_BindShared, STATGROUP_Game);

UCefWebUiBrowserWidget::UCefWebUiBrowserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BrowserWidth = 1920;
	BrowserHeight = 1080;

	TextureFilter = TextureFilter::TF_Bilinear;
	bUseSRGB = false;
}

void UCefWebUiBrowserWidget::OnLoadStateChanged(uint8 InState)
{
	UE_LOG(LogCefWebUi, Log, TEXT("Cef loading state changed: %d"), InState);
}

void UCefWebUiBrowserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ensureMsgf(FCefWebUiModule::IsAvailable(), TEXT("FCefWebUiModule is not available")))
		return;

	FCefWebUiModule& ModuleRef = FCefWebUiModule::Get();
	FrameReader = ModuleRef.GetFrameReaderPtr();
	InputWriter = ModuleRef.GeInputWriterPtr();
	ControlWriter = ModuleRef.GetControlWriterPtr();

	TSharedRef<FCefInputWriter> InputWriterRef = InputWriter.Pin().ToSharedRef();
	if (!InputWriterRef->IsOpen())
		InputWriterRef->Open();

	TSharedRef<FCefControlWriter> ControlWriterRef = ControlWriter.Pin().ToSharedRef();
	if (!ControlWriterRef->IsOpen())
		ControlWriterRef->Open();

	FrameReader.Pin()->OnLoadStateChanged.AddUObject(this, &UCefWebUiBrowserWidget::OnLoadStateChanged);
}

void UCefWebUiBrowserWidget::NativeDestruct()
{
	// Release RHI texture on render thread before destroying
	FTextureRHIRef CapturedRHI = SharedTextureRHI;
	if (CapturedRHI.IsValid())
	{
		ENQUEUE_RENDER_COMMAND(CefReleaseSharedTex)(
			[CapturedRHI](FRHICommandListImmediate&) mutable
			{
				CapturedRHI.SafeRelease();
			});
	}
	SharedTextureRHI = nullptr;
	LastSharedHandle = nullptr;

	FrameReader.Reset();
	InputWriter.Reset();
	ControlWriter.Reset();
	Super::NativeDestruct();
}

void UCefWebUiBrowserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!FrameReader.IsValid())
		return;

	AccumulatedTime += InDeltaTime;
	if (AccumulatedTime < (1.0f / TargetFPS))
		return;

	AccumulatedTime = 0.0f;
	PollAndUpload();
}

void UCefWebUiBrowserWidget::EnsureExternalTexture(void* InNTHandle, uint32 InWidth, uint32 InHeight, uint32 InCefPid)
{
	if (InNTHandle == LastSharedHandle && SharedTextureRHI.IsValid())
		return;

	LastSharedHandle = InNTHandle;

	FTextureRHIRef OldRHI = SharedTextureRHI;
	SharedTextureRHI = nullptr;

	// Duplicate the CEF process handle into this process
	HANDLE CefProcess = OpenProcess(PROCESS_DUP_HANDLE, Windows::FALSE, InCefPid);
	if (!CefProcess)
	{
		UE_LOG(LogCefWebUi, Error, TEXT("CefWidget: OpenProcess failed: %d"), GetLastError());
		return;
	}

	HANDLE LocalHandle = nullptr;
	BOOL bOk = DuplicateHandle(
		CefProcess,
		reinterpret_cast<HANDLE>(InNTHandle),
		GetCurrentProcess(),
		&LocalHandle,
		0,
		Windows::FALSE,
		DUPLICATE_SAME_ACCESS
	);
	CloseHandle(CefProcess);

	if (!bOk || !LocalHandle)
	{
		UE_LOG(LogCefWebUi, Error, TEXT("CefWidget: DuplicateHandle failed: %d"), GetLastError());
		return;
	}

	ENQUEUE_RENDER_COMMAND(CefOpenSharedTexture)(
		[this, LocalHandle, InWidth, InHeight, OldRHI](FRHICommandListImmediate& RHICmdList) mutable
		{
			OldRHI.SafeRelease();

			ID3D12DynamicRHI* D3D12RHI = GetID3D12DynamicRHI();
			if (!D3D12RHI)
			{
				UE_LOG(LogCefWebUi, Error, TEXT("CefWidget: Failed to get ID3D12DynamicRHI"));
				CloseHandle(LocalHandle);
				return;
			}

			ID3D12Device* D3DDevice = D3D12RHI->RHIGetDevice(0);
			if (!D3DDevice)
			{
				UE_LOG(LogCefWebUi, Error, TEXT("CefWidget: Failed to get ID3D12Device"));
				CloseHandle(LocalHandle);
				return;
			}

			ID3D12Resource* D3DResource = nullptr;
			HRESULT hr = D3DDevice->OpenSharedHandle(LocalHandle, IID_PPV_ARGS(&D3DResource));
			CloseHandle(LocalHandle); // always close after OpenSharedHandle attempt

			if (FAILED(hr) || !D3DResource)
			{
				UE_LOG(LogCefWebUi, Error, TEXT("CefWidget: OpenSharedHandle failed: 0x%08X"), hr);
				return;
			}

			SharedTextureRHI = D3D12RHI->RHICreateTexture2DFromResource(
				PF_B8G8R8A8,
				ETextureCreateFlags::External | ETextureCreateFlags::ShaderResource,
				FClearValueBinding::None,
				D3DResource);

			D3DResource->Release();

			if (!SharedTextureRHI.IsValid())
			{
				UE_LOG(LogCefWebUi, Error, TEXT("CefWidget: RHICreateTexture2DFromResource failed"));
				return;
			}

			UE_LOG(LogCefWebUi, Log, TEXT("CefWidget: Shared texture bound %ux%u"), InWidth, InHeight);

			if (!SharedTextureRHI.IsValid())
			{
				UE_LOG(LogCefWebUi, Error, TEXT("CefWidget: RHICreateTexture2DFromResource failed"));
				return;
			}

			UE_LOG(LogCefWebUi, Log, TEXT("CefWidget: SharedTextureRHI is valid"));

			AsyncTask(ENamedThreads::GameThread, [this]()
			{
				UE_LOG(LogCefWebUi, Log, TEXT("CefWidget: GameThread bind, DisplayImage=%s DisplayTexture=%s"),
				       DisplayImage ? TEXT("valid") : TEXT("null"),
				       DisplayTexture ? TEXT("valid") : TEXT("null"));

				// ...
			});

			AsyncTask(ENamedThreads::GameThread, [this, InWidth, InHeight]()
			{
				// Create a proper transient texture as the carrier
				if (!DisplayTexture || TextureWidth != InWidth || TextureHeight != InHeight)
				{
					TextureWidth = InWidth;
					TextureHeight = InHeight;
					DisplayTexture = UTexture2D::CreateTransient(InWidth, InHeight, PF_B8G8R8A8);
					DisplayTexture->Filter = TextureFilter;
					DisplayTexture->SRGB = bUseSRGB;
					DisplayTexture->UpdateResource();
				}

				// Replace its RHI with our shared texture
				FTextureRHIRef texRhi = SharedTextureRHI;
				FTextureResource* resource = DisplayTexture->GetResource();
				if (resource)
				{
					ENQUEUE_RENDER_COMMAND(CefSetRHI)(
						[resource, texRhi](FRHICommandListImmediate&)
						{
							resource->TextureRHI = texRhi;
						});
				}

				if (DisplayImage)
					DisplayImage->SetBrushFromTexture(DisplayTexture);
			});
		}
	);
}

void UCefWebUiBrowserWidget::PollAndUpload()
{
	TSharedPtr<FCefFrameReader> Reader = FrameReader.Pin();
	if (!Reader)
		return;

	FCefSharedFrame Frame;

	{
		SCOPE_CYCLE_COUNTER(STAT_CefWidget_PollFrame);
		if (!Reader->PollSharedTexture(Frame))
			return;
	}

	if (Frame.SharedTextureHandle == nullptr)
		return;

	/*UE_LOG(LogCefWebUi, Log, TEXT("CefWidget: Frame %dx%d has been pooled!"),
	       Frame.Width, Frame.Height);*/
	SetCursor(FCefFrameReader::MapCefCursor(Frame.CursorType));

	{
		SCOPE_CYCLE_COUNTER(STAT_CefWidget_BindShared);
		EnsureExternalTexture(Frame.SharedTextureHandle, Frame.Width, Frame.Height, Frame.CefPid);
	}
}

// ---- EnsureTexture kept for fallback / legacy ---------------------------------

void UCefWebUiBrowserWidget::EnsureTexture(uint32 InWidth, uint32 InHeight)
{
	if (DisplayTexture && TextureWidth == InWidth && TextureHeight == InHeight)
		return;

	TextureWidth = InWidth;
	TextureHeight = InHeight;

	DisplayTexture = UTexture2D::CreateTransient(InWidth, InHeight, PF_B8G8R8A8);
	DisplayTexture->Filter = TF_Bilinear;
	DisplayTexture->UpdateResource();

	if (DisplayImage)
		DisplayImage->SetBrushFromTexture(DisplayTexture);
}

// ---- Input -------------------------------------------------------------------

FReply UCefWebUiBrowserWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	TSharedPtr<FCefInputWriter> Writer = InputWriter.Pin();
	if (Writer)
	{
		int32 X, Y;
		GetBrowserCoords(InGeometry, InMouseEvent.GetScreenSpacePosition(), X, Y);
		Writer->WriteMouseMove(X, Y);
	}
	return FReply::Handled();
}

FReply UCefWebUiBrowserWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	TSharedPtr<FCefInputWriter> Writer = InputWriter.Pin();
	if (Writer)
	{
		ECefMouseButton Btn;
		if (SlateButtonToCef(InMouseEvent.GetEffectingButton(), Btn))
		{
			int32 X, Y;
			GetBrowserCoords(InGeometry, InMouseEvent.GetScreenSpacePosition(), X, Y);
			Writer->WriteMouseButton(X, Y, Btn, false);
		}
	}
	return FReply::Handled().CaptureMouse(GetCachedWidget().ToSharedRef());
}

FReply UCefWebUiBrowserWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	TSharedPtr<FCefInputWriter> Writer = InputWriter.Pin();
	if (Writer)
	{
		ECefMouseButton Btn;
		if (SlateButtonToCef(InMouseEvent.GetEffectingButton(), Btn))
		{
			int32 X, Y;
			GetBrowserCoords(InGeometry, InMouseEvent.GetScreenSpacePosition(), X, Y);
			Writer->WriteMouseButton(X, Y, Btn, true);
		}
	}
	return FReply::Handled().ReleaseMouseCapture();
}

FReply UCefWebUiBrowserWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	TSharedPtr<FCefInputWriter> Writer = InputWriter.Pin();
	if (Writer)
	{
		int32 X, Y;
		GetBrowserCoords(InGeometry, InMouseEvent.GetScreenSpacePosition(), X, Y);
		Writer->WriteMouseScroll(X, Y, 0.f, InMouseEvent.GetWheelDelta() * 120.f);
	}
	return FReply::Handled();
}

FReply UCefWebUiBrowserWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	TSharedPtr<FCefInputWriter> Writer = InputWriter.Pin();
	if (Writer)
		Writer->WriteKey(InKeyEvent.GetKeyCode(),
		                 InKeyEvent.GetModifierKeys().IsLeftControlDown() ? 0x0002 : 0, true);
	return FReply::Handled();
}

FReply UCefWebUiBrowserWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	TSharedPtr<FCefInputWriter> Writer = InputWriter.Pin();
	if (Writer)
		Writer->WriteKey(InKeyEvent.GetKeyCode(),
		                 InKeyEvent.GetModifierKeys().IsLeftControlDown() ? 0x0002 : 0, false);
	return FReply::Handled();
}

FReply UCefWebUiBrowserWidget::NativeOnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent)
{
	TSharedPtr<FCefInputWriter> Writer = InputWriter.Pin();
	if (Writer)
		Writer->WriteChar(static_cast<uint16>(InCharacterEvent.GetCharacter()));
	return FReply::Handled();
}

// ---- Browser control --------------------------------------------------------

#define CEF_CTRL(expr) if (GetControlWriter()->IsOpen()) { GetControlWriter()->expr; }

void UCefWebUiBrowserWidget::BP_GoBack() { CEF_CTRL(GoBack()) }
void UCefWebUiBrowserWidget::BP_GoForward() { CEF_CTRL(GoForward()) }
void UCefWebUiBrowserWidget::BP_StopLoad() { CEF_CTRL(StopLoad()) }
void UCefWebUiBrowserWidget::BP_Reload() { CEF_CTRL(Reload()) }
void UCefWebUiBrowserWidget::BP_SetURL(const FString& InURL) { CEF_CTRL(SetURL(InURL)) }
void UCefWebUiBrowserWidget::BP_SetPaused(bool bInPaused) { CEF_CTRL(SetPaused(bInPaused)) }
void UCefWebUiBrowserWidget::BP_SetHidden(bool bInHidden) { CEF_CTRL(SetHidden(bInHidden)) }
void UCefWebUiBrowserWidget::BP_SetFocus(bool bInFocus) { CEF_CTRL(SetFocus(bInFocus)) }
void UCefWebUiBrowserWidget::BP_SetZoomLevel(float InLevel) { CEF_CTRL(SetZoomLevel(InLevel)) }
void UCefWebUiBrowserWidget::BP_SetFrameRate(int32 InRate) { CEF_CTRL(SetFrameRate(static_cast<uint32>(InRate))) }
void UCefWebUiBrowserWidget::BP_ScrollTo(int32 InX, int32 InY) { CEF_CTRL(ScrollTo(InX, InY)) }
void UCefWebUiBrowserWidget::BP_SetMuted(bool bInMuted) { CEF_CTRL(SetMuted(bInMuted)) }
void UCefWebUiBrowserWidget::BP_OpenDevTools() { CEF_CTRL(OpenDevTools()) }
void UCefWebUiBrowserWidget::BP_CloseDevTools() { CEF_CTRL(CloseDevTools()) }
void UCefWebUiBrowserWidget::BP_SetInputEnabled(bool bInEnabled) { CEF_CTRL(SetInputEnabled(bInEnabled)) }
void UCefWebUiBrowserWidget::BP_ExecuteJS(const FString& InScript) { CEF_CTRL(ExecuteJS(InScript)) }
void UCefWebUiBrowserWidget::BP_ClearCookies() { CEF_CTRL(ClearCookies()) }

void UCefWebUiBrowserWidget::BP_Resize(int32 InWidth, int32 InHeight)
{
	CEF_CTRL(Resize(static_cast<uint32>(InWidth), static_cast<uint32>(InHeight)))
}

#undef CEF_CTRL

// ---- Helpers ----------------------------------------------------------------

void UCefWebUiBrowserWidget::GetBrowserCoords(const FGeometry& InGeometry,
                                              const FVector2D& InScreenPosition, int32& OutX, int32& OutY) const
{
	const FVector2D LocalPos = InGeometry.AbsoluteToLocal(InScreenPosition);
	const FVector2D LocalSize = InGeometry.GetLocalSize();
	OutX = FMath::Clamp(FMath::RoundToInt(LocalPos.X / LocalSize.X * BrowserWidth), 0, BrowserWidth - 1);
	OutY = FMath::Clamp(FMath::RoundToInt(LocalPos.Y / LocalSize.Y * BrowserHeight), 0, BrowserHeight - 1);
}

bool UCefWebUiBrowserWidget::SlateButtonToCef(const FKey& InKey, ECefMouseButton& OutButton)
{
	if (InKey == EKeys::LeftMouseButton)
	{
		OutButton = ECefMouseButton::Left;
		return true;
	}
	else if (InKey == EKeys::RightMouseButton)
	{
		OutButton = ECefMouseButton::Right;
		return true;
	}
	else if (InKey == EKeys::MiddleMouseButton)
	{
		OutButton = ECefMouseButton::Middle;
		return true;
	}
	return false;
}

// ---- Getters ----------------------------------------------------------------

TSharedRef<FCefControlWriter> UCefWebUiBrowserWidget::GetControlWriter() const
{
	return ControlWriter.Pin().ToSharedRef();
}

TSharedRef<FCefFrameReader> UCefWebUiBrowserWidget::GetFrameReader() const
{
	return FrameReader.Pin().ToSharedRef();
}

TSharedRef<FCefInputWriter> UCefWebUiBrowserWidget::GetInputWriter() const
{
	return InputWriter.Pin().ToSharedRef();
}
