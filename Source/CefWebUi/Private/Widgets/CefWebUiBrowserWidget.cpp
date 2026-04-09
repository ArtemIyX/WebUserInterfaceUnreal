// UCefBrowserWidget.cpp

#include "Widgets/CefWebUiBrowserWidget.h"

#include "CefWebUi.h"
#include "Services/CefFrameReader.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "Services/CefInputWriter.h"
#include "InputCoreTypes.h"

DECLARE_CYCLE_STAT(TEXT("CefWidget: PollFrame"), STAT_CefWidget_PollFrame, STATGROUP_Game);
DECLARE_CYCLE_STAT(TEXT("CefWidget: RHI Upload"), STAT_CefWidget_RHIUpload, STATGROUP_Game);

UCefWebUiBrowserWidget::UCefWebUiBrowserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BrowserWidth = 1920;

	BrowserHeight = 1080;
}


void UCefWebUiBrowserWidget::OnLoadStateChanged(uint8 InState)
{
	UE_LOG(LogTemp, Log, TEXT("Cef loading state changed: %d"), InState);
}

void UCefWebUiBrowserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ensureMsgf(FCefWebUiModule::IsAvailable(), TEXT("FCefWebUiModule is not available")))
	{
		return;
	}

	FCefWebUiModule& moduleRef = FCefWebUiModule::Get();
	FrameReader = moduleRef.GetFrameReaderPtr();
	InputWriter = moduleRef.GeInputWriterPtr();
	ControlWriter = moduleRef.GetControlWriterPtr();

	TSharedRef<FCefInputWriter> inputWriter = InputWriter.Pin().ToSharedRef();
	if (!inputWriter->IsOpen())
	{
		inputWriter->Open();
	}

	TSharedRef<FCefControlWriter> controlWriter = ControlWriter.Pin().ToSharedRef();
	if (!controlWriter->IsOpen())
	{
		controlWriter->Open();
	}

	FrameReader.Pin()->OnLoadStateChanged.AddUObject(this, &UCefWebUiBrowserWidget::OnLoadStateChanged);

	//controlWriter->SetURL("https://www.youtube.com/");
}

void UCefWebUiBrowserWidget::NativeDestruct()
{
	FrameReader.Reset();
	InputWriter.Reset();
	ControlWriter.Reset();
	Super::NativeDestruct();
}

void UCefWebUiBrowserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!FrameReader.IsValid())
	{
		return;
	}

	AccumulatedTime += InDeltaTime;
	if (AccumulatedTime < (1.0f / TargetFPS))
		return;

	AccumulatedTime = 0.0f;
	PollAndUpload();
}

FReply UCefWebUiBrowserWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	TSharedPtr<FCefInputWriter> writer = InputWriter.Pin();
	if (writer)
	{
		int32 x, y;
		GetBrowserCoords(InGeometry, InMouseEvent.GetScreenSpacePosition(), x, y);
		writer->WriteMouseMove(x, y);
	}
	return FReply::Handled();
}

FReply UCefWebUiBrowserWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	TSharedPtr<FCefInputWriter> writer = InputWriter.Pin();
	if (writer)
	{
		ECefMouseButton btn;
		if (SlateButtonToCef(InMouseEvent.GetEffectingButton(), btn))
		{
			int32 x, y;
			GetBrowserCoords(InGeometry, InMouseEvent.GetScreenSpacePosition(), x, y);
			writer->WriteMouseButton(x, y, btn, false);
		}
	}
	return FReply::Handled().CaptureMouse(GetCachedWidget().ToSharedRef());
}

FReply UCefWebUiBrowserWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	TSharedPtr<FCefInputWriter> writer = InputWriter.Pin();
	if (writer)
	{
		ECefMouseButton btn;
		if (SlateButtonToCef(InMouseEvent.GetEffectingButton(), btn))
		{
			int32 x, y;
			GetBrowserCoords(InGeometry, InMouseEvent.GetScreenSpacePosition(), x, y);
			writer->WriteMouseButton(x, y, btn, true);
		}
	}
	return FReply::Handled().ReleaseMouseCapture();
}

FReply UCefWebUiBrowserWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	TSharedPtr<FCefInputWriter> writer = InputWriter.Pin();
	if (writer)
	{
		int32 x, y;
		GetBrowserCoords(InGeometry, InMouseEvent.GetScreenSpacePosition(), x, y);
		writer->WriteMouseScroll(x, y, 0.f, InMouseEvent.GetWheelDelta() * 120.f);
	}
	return FReply::Handled();
}

FReply UCefWebUiBrowserWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	TSharedPtr<FCefInputWriter> writer = InputWriter.Pin();
	if (writer)
		writer->WriteKey(InKeyEvent.GetKeyCode(), InKeyEvent.GetModifierKeys().IsLeftControlDown() ? 0x0002 : 0, true);

	return FReply::Handled();
}

FReply UCefWebUiBrowserWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	TSharedPtr<FCefInputWriter> writer = InputWriter.Pin();
	if (writer)
		writer->WriteKey(InKeyEvent.GetKeyCode(), InKeyEvent.GetModifierKeys().IsLeftControlDown() ? 0x0002 : 0, false);

	return FReply::Handled();
}

FReply UCefWebUiBrowserWidget::NativeOnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent)
{
	TSharedPtr<FCefInputWriter> writer = InputWriter.Pin();
	if (writer)
		writer->WriteChar(static_cast<uint16>(InCharacterEvent.GetCharacter()));

	return FReply::Handled();
}

#define CEF_CTRL(expr) \
if (GetControlWriter()->IsOpen()) { GetControlWriter()->expr; }

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

void UCefWebUiBrowserWidget::BP_Resize(int32 InWidth, int32 InHeight)
{
	CEF_CTRL(Resize(static_cast<uint32>(InWidth), static_cast<uint32>(InHeight)))
}

void UCefWebUiBrowserWidget::BP_SetMuted(bool bInMuted) { CEF_CTRL(SetMuted(bInMuted)) }
void UCefWebUiBrowserWidget::BP_OpenDevTools() { CEF_CTRL(OpenDevTools()) }
void UCefWebUiBrowserWidget::BP_CloseDevTools() { CEF_CTRL(CloseDevTools()) }
void UCefWebUiBrowserWidget::BP_SetInputEnabled(bool bInEnabled) { CEF_CTRL(SetInputEnabled(bInEnabled)) }
void UCefWebUiBrowserWidget::BP_ExecuteJS(const FString& InScript) { CEF_CTRL(ExecuteJS(InScript)) }
void UCefWebUiBrowserWidget::BP_ClearCookies() { CEF_CTRL(ClearCookies()) }

#undef CEF_CTRL

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

void UCefWebUiBrowserWidget::GetBrowserCoords(const FGeometry& InGeometry, const FVector2D& InScreenPosition,
                                              int32& OutX, int32& OutY) const
{
	const FVector2D localPos = InGeometry.AbsoluteToLocal(InScreenPosition);
	const FVector2D localSize = InGeometry.GetLocalSize();

	OutX = FMath::Clamp(FMath::RoundToInt(localPos.X / localSize.X * BrowserWidth), 0, BrowserWidth - 1);
	OutY = FMath::Clamp(FMath::RoundToInt(localPos.Y / localSize.Y * BrowserHeight), 0, BrowserHeight - 1);
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

void UCefWebUiBrowserWidget::PollAndUpload()
{
	TSharedPtr<FCefFrameReader> reader = FrameReader.Pin();
	if (!reader)
		return;

	FCefFrame frame;

	{
		SCOPE_CYCLE_COUNTER(STAT_CefWidget_PollFrame);
		if (!reader->PollFrame(frame))
			return;
	}

	{
		EMouseCursor::Type cursorType = FCefFrameReader::MapCefCursor(frame.CursorType);
		SetCursor(cursorType);
	}

	EnsureTexture(frame.Width, frame.Height);

	if (!DisplayTexture || !DisplayTexture->GetResource())
		return;

	FTextureResource* resource = DisplayTexture->GetResource();

	ENQUEUE_RENDER_COMMAND(CefUploadFrame)(
		[resource, pixels = MoveTemp(frame.Pixels)](FRHICommandListImmediate& RHICmdList)
		{
			SCOPE_CYCLE_COUNTER(STAT_CefWidget_RHIUpload);

			FRHITexture* rhi = resource->GetTextureRHI();
			if (!rhi)
				return;

			uint32 stride = 0;
			void* data = RHICmdList.LockTexture2D(rhi, 0, RLM_WriteOnly, stride, false);
			if (data)
				FMemory::Memcpy(data, pixels.GetData(), pixels.Num());
			RHICmdList.UnlockTexture2D(rhi, 0, false);
		}
	);
}
