// UCefBrowserWidget.cpp

#include "Widgets/CefWebUiBrowserWidget.h"

#include "CefWebUi.h"
#include "Services/CefFrameReader.h"
#include "Components/Image.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TextureResource.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "RHIResources.h"
#include "Services/CefInputWriter.h"
#include "InputCoreTypes.h"
#include "HAL/IConsoleManager.h"

#include "ID3D12DynamicRHI.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <d3d12.h>
#include <dxgi1_4.h>

#include "tiffiop.h"
#include "Windows/HideWindowsPlatformTypes.h"

namespace
{
constexpr int32 kMaxDirtyRectInflatePx = 8;
constexpr int32 kMaxDirtyRectGapPx = 16;
}

DECLARE_STATS_GROUP(TEXT("CefWebUiTelemetry"), STATGROUP_CefWebUiTelemetry, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("CefWidget: PollFrame"), STAT_CefWidget_PollFrame, STATGROUP_CefWebUiTelemetry);
DECLARE_CYCLE_STAT(TEXT("CefWidget: GPUBlit"), STAT_CefWidget_GPUBlit, STATGROUP_CefWebUiTelemetry);
DECLARE_CYCLE_STAT(TEXT("CefWidget: GPUBlit Full"), STAT_CefWidget_GPUBlitFull, STATGROUP_CefWebUiTelemetry);
DECLARE_CYCLE_STAT(TEXT("CefWidget: GPUBlit Dirty"), STAT_CefWidget_GPUBlitDirty, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Consumed Frames"), STAT_CefTel_ConsumedFrames, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Frame Gaps"), STAT_CefTel_FrameGaps, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Forced Full"), STAT_CefTel_ForcedFull, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Full Copy"), STAT_CefTel_FullCopy, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Dirty Copy"), STAT_CefTel_DirtyCopy, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Dirty Rect Count Sum"), STAT_CefTel_DirtyRectCountSum, STATGROUP_CefWebUiTelemetry);
DECLARE_DWORD_COUNTER_STAT(TEXT("Dirty Rect Area Sum"), STAT_CefTel_DirtyRectAreaSum, STATGROUP_CefWebUiTelemetry);

static TAutoConsoleVariable<int32> CVarCefWebUiUseDirtyRects(
	TEXT("CefWebUi.UseDirtyRects"),
	0,
	TEXT("Enable dirty-rect copy path (0=full copy default, 1=dirty rect copy)."));

static TAutoConsoleVariable<int32> CVarCefWebUiCadenceFeedback(
	TEXT("CefWebUi.CadenceFeedback"),
	1,
	TEXT("Enable consumer cadence feedback to host pacing (0/1)."));

static TAutoConsoleVariable<float> CVarCefWebUiCadenceFeedbackPeriodSec(
	TEXT("CefWebUi.CadenceFeedbackPeriodSec"),
	0.10f,
	TEXT("Cadence feedback send period in seconds."));

static TAutoConsoleVariable<float> CVarCefWebUiIdleDirtyRecoverySec(
	TEXT("CefWebUi.IdleDirtyRecoverySec"),
	0.15f,
	TEXT("After dirty upload and no new frame for this many seconds, force one full copy."));

static TAutoConsoleVariable<float> CVarCefWebUiDirtySafetyFullCopySec(
	TEXT("CefWebUi.DirtySafetyFullCopySec"),
	0.25f,
	TEXT("While dirty mode is enabled, force periodic full copy every N seconds (0 disables)."));

static TAutoConsoleVariable<int32> CVarCefWebUiDirtyRectInflatePx(
	TEXT("CefWebUi.DirtyRectInflatePx"),
	1,
	TEXT("Inflate dirty rects by N pixels before copy."));

static TAutoConsoleVariable<int32> CVarCefWebUiDirtyRectCoalesceGapPx(
	TEXT("CefWebUi.DirtyRectCoalesceGapPx"),
	2,
	TEXT("Merge dirty rects that overlap or are within N pixels."));

static TAutoConsoleVariable<float> CVarCefWebUiDirtyAreaFullCopyThreshold(
	TEXT("CefWebUi.DirtyAreaFullCopyThreshold"),
	0.45f,
	TEXT("Fallback to full copy when merged dirty area ratio exceeds this [0..1]."));

static TAutoConsoleVariable<float> CVarCefWebUiTelemetryLogPeriodSec(
	TEXT("CefWebUi.TelemetryLogPeriodSec"),
	2.0f,
	TEXT("Telemetry log period in seconds."));

static TAutoConsoleVariable<int32> CVarCefWebUiMaxDirtyRectsPerFrame(
	TEXT("CefWebUi.MaxDirtyRectsPerFrame"),
	12,
	TEXT("Max dirty rects processed per frame; above this uses full copy."));

static TAutoConsoleVariable<float> CVarCefWebUiDirtyDisableAfterInputMs(
	TEXT("CefWebUi.DirtyDisableAfterInputMs"),
	200.0f,
	TEXT("Disable dirty rects for this many ms after input (0 disables)."));

static TAutoConsoleVariable<int32> CVarCefWebUiUploadSkipWhenHidden(
	TEXT("CefWebUi.UploadSkipWhenHidden"),
	1,
	TEXT("Skip PollAndUpload when widget is hidden (0/1)."));

static TAutoConsoleVariable<int32> CVarCefWebUiForceFullOnFrameGap(
	TEXT("CefWebUi.ForceFullOnFrameGap"),
	1,
	TEXT("After frame gap, force full copies for N subsequent frames (0 disables)."));

static TAutoConsoleVariable<float> CVarCefWebUiCursorUpdateRateHz(
	TEXT("CefWebUi.CursorUpdateRateHz"),
	120.0f,
	TEXT("Max cursor updates per second (0 disables limit)."));

static TAutoConsoleVariable<int32> CVarCefWebUiGpuFenceSync(
	TEXT("CefWebUi.GpuFenceSync"),
	0,
	TEXT("Wait on host-published shared GPU fence before copying shared texture (0/1)."));

static TAutoConsoleVariable<int32> CVarCefWebUiGpuFenceWaitTimeoutMs(
	TEXT("CefWebUi.GpuFenceWaitTimeoutMs"),
	8,
	TEXT("Initial timeout in ms for GPU fence wait before falling back to blocking wait."));

static TAutoConsoleVariable<int32> CVarCefWebUiGpuFenceWaitMode(
	TEXT("CefWebUi.GpuFenceWaitMode"),
	1,
	TEXT("0=auto, 1=timeout-no-block, 2=block-only, 3=no-wait"));

static TAutoConsoleVariable<int32> CVarCefWebUiUsePopupDedicatedPlane(
	TEXT("CefWebUi.UsePopupDedicatedPlane"),
	1,
	TEXT("Use dedicated popup shared plane when host provides it (0/1)."));

static TAutoConsoleVariable<float> CVarCefWebUiHostTuningPushPeriodSec(
	TEXT("CefWebUi.HostTuningPushPeriodSec"),
	0.5f,
	TEXT("How often to push host tuning CVars via control channel."));

static TAutoConsoleVariable<int32> CVarCefWebUiHostMaxInFlightBeginFrames(
	TEXT("CefWebUi.HostMaxInFlightBeginFrames"),
	0,
	TEXT("Host tuning: max in-flight begin frames (0 disables cap)."));

static TAutoConsoleVariable<int32> CVarCefWebUiHostFlushIntervalFrames(
	TEXT("CefWebUi.HostFlushIntervalFrames"),
	4,
	TEXT("Host tuning: flush every N frames (0=only special cases)."));

static TAutoConsoleVariable<int32> CVarCefWebUiHostKeyframeIntervalUs(
	TEXT("CefWebUi.HostKeyframeIntervalUs"),
	300000,
	TEXT("Host tuning: time-based keyframe full refresh interval in microseconds."));

UCefWebUiBrowserWidget::UCefWebUiBrowserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BrowserWidth = 1920;
	BrowserHeight = 1080;
	bAutoGenerateMips = false;
	RenderFilter = TextureFilter::TF_Bilinear;
	bSRGB = false;
	RenderCompression = TextureCompressionSettings::TC_Default;
	RenderMipLoadOptions = ETextureMipLoadOptions::OnlyFirstMip;
}

void UCefWebUiBrowserWidget::ResetRuntimeState()
{
	LastConsumerFrameTimeSec = 0.0;
	LastCadenceSentTimeSec = 0.0;
	LastUploadTimeSec = 0.0;
	LastIdleRecoveryCopyTimeSec = 0.0;
	LastSafetyFullCopyTimeSec = 0.0;
	LastInputEventTimeSec = 0.0;
	LastCursorUpdateTimeSec = 0.0;
	GpuFenceAutoBlockUntilSec = 0.0;
	LastHostTuningPushTimeSec = 0.0;
	LastTelemetryLogTimeSec = 0.0;
	SmoothedCadenceUs = 0;
	LastSeenFrameId = 0;
	PendingForceFullFrames = 0;
	GpuFenceTimeoutBurst = 0;
	LastCursorType = ECefCustomCursorType::CT_NONE;
	bLastUploadUsedDirty = false;
	bHasLastUploadedFrame = false;
	LastUploadedFrame = FCefSharedFrame{};
	TelemetryConsumedFrames = 0;
	TelemetryFrameGapCount = 0;
	TelemetryForcedFullCount = 0;
	TelemetryFullCopyCount = 0;
	TelemetryDirtyCopyCount = 0;
	TelemetryDirtyRectCountSum = 0;
	TelemetryDirtyRectAreaSum = 0;
	SET_DWORD_STAT(STAT_CefTel_ConsumedFrames, 0);
	SET_DWORD_STAT(STAT_CefTel_FrameGaps, 0);
	SET_DWORD_STAT(STAT_CefTel_ForcedFull, 0);
	SET_DWORD_STAT(STAT_CefTel_FullCopy, 0);
	SET_DWORD_STAT(STAT_CefTel_DirtyCopy, 0);
	SET_DWORD_STAT(STAT_CefTel_DirtyRectCountSum, 0);
	SET_DWORD_STAT(STAT_CefTel_DirtyRectAreaSum, 0);
}

void UCefWebUiBrowserWidget::EnsureRenderTarget(uint32 InWidth, uint32 InHeight)
{
	if (RenderTarget && TextureWidth == InWidth && TextureHeight == InHeight)
		return;

	TextureWidth = InWidth;
	TextureHeight = InHeight;

	RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	RenderTarget->InitCustomFormat(InWidth, InHeight, PF_B8G8R8A8, false);
	RenderTarget->bAutoGenerateMips = bAutoGenerateMips;
	RenderTarget->Filter = RenderFilter;
	RenderTarget->SRGB = bSRGB;
	RenderTarget->CompressionSettings = RenderCompression;
	RenderTarget->MipLoadOptions = RenderMipLoadOptions;
	RenderTarget->UpdateResource();

	FSlateBrush Brush;
	Brush.SetResourceObject(RenderTarget);
	Brush.ImageSize = FVector2D(InWidth, InHeight);
	if (DisplayImage)
		DisplayImage->SetBrush(Brush);
}


void UCefWebUiBrowserWidget::OnLoadStateChanged(uint8 InState)
{
	UE_LOG(LogCefWebUi, Log, TEXT("Cef loading state changed: %d"), InState);
}

void UCefWebUiBrowserWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResetRuntimeState();

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
	FTextureRHIRef OldRHI0 = SharedTextureRHI[0];
	FTextureRHIRef OldRHI1 = SharedTextureRHI[1];
	FTextureRHIRef OldRHI2 = SharedTextureRHI[2];
	FTextureRHIRef OldPopup = SharedPopupTextureRHI;

	ENQUEUE_RENDER_COMMAND(CefReleaseSharedTextures)(
		[OldRHI0, OldRHI1, OldRHI2, OldPopup](FRHICommandListImmediate&) mutable
		{
			OldRHI0.SafeRelease();
			OldRHI1.SafeRelease();
			OldRHI2.SafeRelease();
			OldPopup.SafeRelease();
		});

	for (uint32 i = 0; i < MaxSharedSlots; ++i)
	{
		SharedTextureRHI[i] = nullptr;
		LastSharedHandle[i] = nullptr;
	}
	if (SharedGpuFence)
	{
		reinterpret_cast<ID3D12Fence*>(SharedGpuFence)->Release();
		SharedGpuFence = nullptr;
	}
	if (GpuFenceWaitEvent)
	{
		CloseHandle(static_cast<HANDLE>(GpuFenceWaitEvent));
		GpuFenceWaitEvent = nullptr;
	}
	SharedPopupTextureRHI = nullptr;
	SharedSlotCount = 2;
	ResetRuntimeState();
	RenderTarget = nullptr;

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
	if (CVarCefWebUiUploadSkipWhenHidden.GetValueOnAnyThread() != 0 && !IsVisible())
		return;

	PollAndUpload();
}

void UCefWebUiBrowserWidget::EnsureSharedRHI()
{
	bool bReady = true;
	const uint32 slotCount = FMath::Clamp(SharedSlotCount, 1u, MaxSharedSlots);
	for (uint32 i = 0; i < slotCount; ++i)
	{
		if (!SharedTextureRHI[i].IsValid())
		{
			bReady = false;
			break;
		}
	}
	if (bReady)
		return;

	ENQUEUE_RENDER_COMMAND(CefOpenSharedTextures)(
		[this](FRHICommandListImmediate&)
		{
			ID3D12DynamicRHI* D3D12RHI = GetID3D12DynamicRHI();
			if (!D3D12RHI)
			{
				UE_LOG(LogCefWebUi, Error, TEXT("CefWidget: Failed to get ID3D12DynamicRHI"));
				return;
			}

			ID3D12Device* D3DDevice = D3D12RHI->RHIGetDevice(0);
			if (!D3DDevice)
			{
				UE_LOG(LogCefWebUi, Error, TEXT("CefWidget: Failed to get ID3D12Device"));
				return;
			}

			const uint32 slotCountInner = FMath::Clamp(SharedSlotCount, 1u, MaxSharedSlots);
			for (uint32 i = 0; i < slotCountInner; ++i)
			{
				if (SharedTextureRHI[i].IsValid())
					continue;

				wchar_t Name[64];
				swprintf(Name, 64, L"Global\\CEFHost_SharedTex_%u", i);
				HANDLE SharedHandle = nullptr;
				HRESULT hr = D3DDevice->OpenSharedHandleByName(Name, GENERIC_ALL, &SharedHandle);
				if (FAILED(hr) || !SharedHandle)
				{
					UE_LOG(LogCefWebUi, Error, TEXT("CefWidget: OpenSharedHandleByName[%u] failed: 0x%08X"), i, hr);
					return;
				}

				ID3D12Resource* D3DResource = nullptr;
				hr = D3DDevice->OpenSharedHandle(SharedHandle, IID_PPV_ARGS(&D3DResource));
				CloseHandle(SharedHandle);

				if (FAILED(hr) || !D3DResource)
				{
					UE_LOG(LogCefWebUi, Error, TEXT("CefWidget: OpenSharedHandle[%u] failed: 0x%08X"), i, hr);
					return;
				}
				SharedTextureRHI[i] = D3D12RHI->RHICreateTexture2DFromResource(
					PF_B8G8R8A8,
					ETextureCreateFlags::External | ETextureCreateFlags::ShaderResource,
					FClearValueBinding::None,
					D3DResource);

				D3DResource->Release();

				if (!SharedTextureRHI[i].IsValid())
				{
					UE_LOG(LogCefWebUi, Error,
					       TEXT("CefWidget: RHICreateTexture2DFromResource[%u] failed"), i);
					return;
				}
			}

			UE_LOG(LogCefWebUi, Log, TEXT("CefWidget: Opened %u shared textures by name"), slotCountInner);
		}
	);
}

bool UCefWebUiBrowserWidget::EnsureGpuFence()
{
	if (SharedGpuFence)
		return true;

	ID3D12DynamicRHI* D3D12RHI = GetID3D12DynamicRHI();
	if (!D3D12RHI)
		return false;

	ID3D12Device* D3DDevice = D3D12RHI->RHIGetDevice(0);
	if (!D3DDevice)
		return false;

	HANDLE SharedHandle = nullptr;
	HRESULT hr = D3DDevice->OpenSharedHandleByName(L"Global\\CEFHost_SharedFence", GENERIC_ALL, &SharedHandle);
	if (FAILED(hr) || !SharedHandle)
		return false;

	ID3D12Fence* Fence = nullptr;
	hr = D3DDevice->OpenSharedHandle(SharedHandle, IID_PPV_ARGS(&Fence));
	CloseHandle(SharedHandle);
	if (FAILED(hr) || !Fence)
		return false;

	SharedGpuFence = Fence;
	if (!GpuFenceWaitEvent)
		GpuFenceWaitEvent = CreateEventW(nullptr, Windows::FALSE, Windows::FALSE, nullptr);

	UE_LOG(LogCefWebUi, Log, TEXT("CefWidget: Opened shared GPU fence"));
	return true;
}

bool UCefWebUiBrowserWidget::EnsurePopupPlaneRHI()
{
	if (SharedPopupTextureRHI.IsValid())
		return true;

	ID3D12DynamicRHI* D3D12RHI = GetID3D12DynamicRHI();
	if (!D3D12RHI)
		return false;

	ID3D12Device* D3DDevice = D3D12RHI->RHIGetDevice(0);
	if (!D3DDevice)
		return false;

	HANDLE SharedHandle = nullptr;
	HRESULT hr = D3DDevice->OpenSharedHandleByName(L"Global\\CEFHost_SharedPopupTex", GENERIC_ALL, &SharedHandle);
	if (FAILED(hr) || !SharedHandle)
		return false;

	ID3D12Resource* D3DResource = nullptr;
	hr = D3DDevice->OpenSharedHandle(SharedHandle, IID_PPV_ARGS(&D3DResource));
	CloseHandle(SharedHandle);
	if (FAILED(hr) || !D3DResource)
		return false;

	SharedPopupTextureRHI = D3D12RHI->RHICreateTexture2DFromResource(
		PF_B8G8R8A8,
		ETextureCreateFlags::External | ETextureCreateFlags::ShaderResource,
		FClearValueBinding::None,
		D3DResource);
	D3DResource->Release();
	return SharedPopupTextureRHI.IsValid();
}

void UCefWebUiBrowserWidget::PushHostTuningIfNeeded(double NowSec)
{
	const double periodSec = FMath::Max(0.05, static_cast<double>(CVarCefWebUiHostTuningPushPeriodSec.GetValueOnAnyThread()));
	if (LastHostTuningPushTimeSec > 0.0 && (NowSec - LastHostTuningPushTimeSec) < periodSec)
		return;

	TSharedPtr<FCefControlWriter> Ctrl = ControlWriter.Pin();
	if (!Ctrl || !Ctrl->IsOpen())
		return;

	Ctrl->SetMaxInFlightBeginFrames(static_cast<uint32>(FMath::Max(0, CVarCefWebUiHostMaxInFlightBeginFrames.GetValueOnAnyThread())));
	Ctrl->SetFlushIntervalFrames(static_cast<uint32>(FMath::Max(0, CVarCefWebUiHostFlushIntervalFrames.GetValueOnAnyThread())));
	Ctrl->SetKeyframeIntervalUs(static_cast<uint32>(FMath::Max(0, CVarCefWebUiHostKeyframeIntervalUs.GetValueOnAnyThread())));
	LastHostTuningPushTimeSec = NowSec;
}

void UCefWebUiBrowserWidget::WaitForProducerGpuFence(uint64 FenceValue)
{
	if (CVarCefWebUiGpuFenceSync.GetValueOnAnyThread() == 0)
		return;
	if (FenceValue == 0)
		return;
	if (!EnsureGpuFence())
		return;
	if (!GpuFenceWaitEvent || !SharedGpuFence)
		return;

	ID3D12Fence* Fence = reinterpret_cast<ID3D12Fence*>(SharedGpuFence);
	if (Fence->GetCompletedValue() >= FenceValue)
		return;

	HRESULT hr = Fence->SetEventOnCompletion(FenceValue, static_cast<HANDLE>(GpuFenceWaitEvent));
	if (FAILED(hr))
		return;

	const double nowSec = FPlatformTime::Seconds();
	const int32 waitMode = CVarCefWebUiGpuFenceWaitMode.GetValueOnAnyThread();
	if (waitMode == 3)
		return;

	if (waitMode == 2)
	{
		WaitForSingleObject(static_cast<HANDLE>(GpuFenceWaitEvent), INFINITE);
		return;
	}

	const DWORD timeoutMs = static_cast<DWORD>(FMath::Clamp(CVarCefWebUiGpuFenceWaitTimeoutMs.GetValueOnAnyThread(), 1, 1000));
	const DWORD wr = WaitForSingleObject(static_cast<HANDLE>(GpuFenceWaitEvent), timeoutMs);
	if (wr == WAIT_TIMEOUT)
	{
		UE_LOG(LogCefWebUiTelemetry, Verbose, TEXT("CefWidget: GPU fence wait timeout"));
		if (waitMode == 0)
		{
			++GpuFenceTimeoutBurst;
			if (GpuFenceTimeoutBurst >= 3)
			{
				GpuFenceAutoBlockUntilSec = nowSec + 1.0; // telemetry hint window only
				GpuFenceTimeoutBurst = 0;
			}
		}
		return;
	}
	else if (waitMode == 0)
	{
		GpuFenceTimeoutBurst = 0;
	}
}

bool UCefWebUiBrowserWidget::TryAcquireFrame(FCefSharedFrame& OutFrame, bool& bOutIdleRecoveryFullCopy, double NowSec)
{
	bOutIdleRecoveryFullCopy = false;
	TSharedPtr<FCefFrameReader> Reader = FrameReader.Pin();
	if (!Reader)
		return false;

	SCOPE_CYCLE_COUNTER(STAT_CefWidget_PollFrame);
	if (Reader->PollSharedTexture(OutFrame))
		return true;

	const double idleRecoverySec = FMath::Max(0.0, static_cast<double>(CVarCefWebUiIdleDirtyRecoverySec.GetValueOnAnyThread()));
	if (!bHasLastUploadedFrame || !bLastUploadUsedDirty || idleRecoverySec <= 0.0)
		return false;
	if ((NowSec - LastUploadTimeSec) < idleRecoverySec)
		return false;
	if ((NowSec - LastIdleRecoveryCopyTimeSec) < idleRecoverySec)
		return false;

	OutFrame = LastUploadedFrame;
	OutFrame.bForceFullRefresh = true;
	OutFrame.DirtyCount = 0;
	LastIdleRecoveryCopyTimeSec = NowSec;
	bOutIdleRecoveryFullCopy = true;
	return true;
}

void UCefWebUiBrowserWidget::UpdateFrameGapTelemetry(const FCefSharedFrame& Frame, bool bIdleRecoveryFullCopy)
{
	if (bIdleRecoveryFullCopy)
		return;

	++TelemetryConsumedFrames;
	if (LastSeenFrameId != 0 && Frame.FrameId != (LastSeenFrameId + 1))
	{
		++TelemetryFrameGapCount;
		const int32 forceN = FMath::Max(0, CVarCefWebUiForceFullOnFrameGap.GetValueOnAnyThread());
		if (forceN > 0)
			PendingForceFullFrames = static_cast<uint32>(forceN);
	}
	LastSeenFrameId = Frame.FrameId;
	SET_DWORD_STAT(STAT_CefTel_ConsumedFrames, TelemetryConsumedFrames);
	SET_DWORD_STAT(STAT_CefTel_FrameGaps, TelemetryFrameGapCount);
}

void UCefWebUiBrowserWidget::UpdateCadenceFeedback(double NowSec, bool bIdleRecoveryFullCopy)
{
	if (bIdleRecoveryFullCopy)
		return;
	if (CVarCefWebUiCadenceFeedback.GetValueOnAnyThread() == 0)
		return;

	if (LastConsumerFrameTimeSec > 0.0)
	{
		const double dtSec = NowSec - LastConsumerFrameTimeSec;
		if (dtSec > 0.0)
		{
			const uint32 rawUs = static_cast<uint32>(FMath::Clamp(dtSec * 1000000.0, 4000.0, 66666.0));
			SmoothedCadenceUs = (SmoothedCadenceUs == 0) ? rawUs : ((SmoothedCadenceUs * 4u + rawUs * 6u) / 10u);

			const double feedbackPeriodSec = FMath::Max(0.01, static_cast<double>(CVarCefWebUiCadenceFeedbackPeriodSec.GetValueOnAnyThread()));
			if (NowSec - LastCadenceSentTimeSec >= feedbackPeriodSec)
			{
				if (TSharedPtr<FCefControlWriter> Ctrl = ControlWriter.Pin())
				{
					if (Ctrl->IsOpen())
					{
						Ctrl->SetConsumerCadenceUs(SmoothedCadenceUs);
						LastCadenceSentTimeSec = NowSec;
					}
				}
			}
		}
	}
	LastConsumerFrameTimeSec = NowSec;
}

bool UCefWebUiBrowserWidget::ResolveUseDirtyRects(const FCefSharedFrame& Frame, double NowSec)
{
	const bool bDirtyRectsEnabled = (CVarCefWebUiUseDirtyRects.GetValueOnAnyThread() != 0);
	bool bUseDirtyRectsRaw = bDirtyRectsEnabled && !Frame.bForceFullRefresh && Frame.DirtyCount > 0;

	const int32 maxRects = FMath::Max(0, CVarCefWebUiMaxDirtyRectsPerFrame.GetValueOnAnyThread());
	if (maxRects > 0 && Frame.DirtyCount > static_cast<uint8>(maxRects))
		bUseDirtyRectsRaw = false;

	const double disableAfterInputSec = FMath::Max(0.0, static_cast<double>(CVarCefWebUiDirtyDisableAfterInputMs.GetValueOnAnyThread()) / 1000.0);
	if (disableAfterInputSec > 0.0 && (NowSec - LastInputEventTimeSec) < disableAfterInputSec)
		bUseDirtyRectsRaw = false;

	if (PendingForceFullFrames > 0)
	{
		--PendingForceFullFrames;
		bUseDirtyRectsRaw = false;
	}

	const double safetyFullCopySec = FMath::Max(0.0, static_cast<double>(CVarCefWebUiDirtySafetyFullCopySec.GetValueOnAnyThread()));
	const bool bSafetyFullCopy = bUseDirtyRectsRaw && safetyFullCopySec > 0.0 &&
		((NowSec - LastSafetyFullCopyTimeSec) >= safetyFullCopySec);
	if (bSafetyFullCopy)
		LastSafetyFullCopyTimeSec = NowSec;
	return bUseDirtyRectsRaw && !bSafetyFullCopy;
}

void UCefWebUiBrowserWidget::UpdateCopyTelemetry(const FCefSharedFrame& Frame, bool bUseDirtyRects)
{
	if (Frame.bForceFullRefresh)
		++TelemetryForcedFullCount;
	if (bUseDirtyRects)
	{
		++TelemetryDirtyCopyCount;
		TelemetryDirtyRectCountSum += Frame.DirtyCount;
		uint64 rectArea = 0;
		const uint8 rectCount = FMath::Min<uint8>(Frame.DirtyCount, MAX_CEF_DIRTY_RECTS);
		for (uint8 i = 0; i < rectCount; ++i)
		{
			const FCefDirtyRect& r = Frame.DirtyRects[i];
			rectArea += static_cast<uint64>(FMath::Max(0, r.W)) * static_cast<uint64>(FMath::Max(0, r.H));
		}
		const uint64 remaining = (TelemetryDirtyRectAreaSum < MAX_uint32) ? (MAX_uint32 - TelemetryDirtyRectAreaSum) : 0;
		TelemetryDirtyRectAreaSum += static_cast<uint32>(FMath::Min<uint64>(rectArea, remaining));
	}
	else
	{
		++TelemetryFullCopyCount;
	}
	SET_DWORD_STAT(STAT_CefTel_ForcedFull, TelemetryForcedFullCount);
	SET_DWORD_STAT(STAT_CefTel_FullCopy, TelemetryFullCopyCount);
	SET_DWORD_STAT(STAT_CefTel_DirtyCopy, TelemetryDirtyCopyCount);
	SET_DWORD_STAT(STAT_CefTel_DirtyRectCountSum, TelemetryDirtyRectCountSum);
	SET_DWORD_STAT(STAT_CefTel_DirtyRectAreaSum, TelemetryDirtyRectAreaSum);
}

void UCefWebUiBrowserWidget::MaybeLogAndResetTelemetry(double NowSec)
{
	const double logPeriodSec = FMath::Max(0.0, static_cast<double>(CVarCefWebUiTelemetryLogPeriodSec.GetValueOnAnyThread()));
	if (logPeriodSec <= 0.0)
		return;
	if (LastTelemetryLogTimeSec <= 0.0)
	{
		LastTelemetryLogTimeSec = NowSec;
		return;
	}
	if (NowSec - LastTelemetryLogTimeSec < logPeriodSec)
		return;

	const uint32 frames = TelemetryConsumedFrames;
	const uint32 avgRects = (TelemetryDirtyCopyCount > 0) ? (TelemetryDirtyRectCountSum / TelemetryDirtyCopyCount) : 0;
	const uint32 avgArea = (TelemetryDirtyCopyCount > 0) ? (TelemetryDirtyRectAreaSum / TelemetryDirtyCopyCount) : 0;
	UE_LOG(LogCefWebUiTelemetry, Log,
		TEXT("[CefWidgetTelemetry] frames=%u gaps=%u forced_full=%u full_copy=%u dirty_copy=%u dirty_rects_avg=%u dirty_area_avg=%u"),
		frames,
		TelemetryFrameGapCount,
		TelemetryForcedFullCount,
		TelemetryFullCopyCount,
		TelemetryDirtyCopyCount,
		avgRects,
		avgArea);

	LastTelemetryLogTimeSec = NowSec;
	TelemetryConsumedFrames = 0;
	TelemetryFrameGapCount = 0;
	TelemetryForcedFullCount = 0;
	TelemetryFullCopyCount = 0;
	TelemetryDirtyCopyCount = 0;
	TelemetryDirtyRectCountSum = 0;
	TelemetryDirtyRectAreaSum = 0;
	SET_DWORD_STAT(STAT_CefTel_ConsumedFrames, 0);
	SET_DWORD_STAT(STAT_CefTel_FrameGaps, 0);
	SET_DWORD_STAT(STAT_CefTel_ForcedFull, 0);
	SET_DWORD_STAT(STAT_CefTel_FullCopy, 0);
	SET_DWORD_STAT(STAT_CefTel_DirtyCopy, 0);
	SET_DWORD_STAT(STAT_CefTel_DirtyRectCountSum, 0);
	SET_DWORD_STAT(STAT_CefTel_DirtyRectAreaSum, 0);
}

void UCefWebUiBrowserWidget::MaybeUpdateCursor(const FCefSharedFrame& Frame, double NowSec)
{
	if (Frame.CursorType == LastCursorType)
		return;

	const double hz = FMath::Max(0.0, static_cast<double>(CVarCefWebUiCursorUpdateRateHz.GetValueOnAnyThread()));
	const double minDt = (hz > 0.0) ? (1.0 / hz) : 0.0;
	if (minDt > 0.0 && (NowSec - LastCursorUpdateTimeSec) < minDt)
		return;

	LastCursorType = Frame.CursorType;
	LastCursorUpdateTimeSec = NowSec;
	SetCursor(FCefFrameReader::MapCefCursor(Frame.CursorType));
}

void UCefWebUiBrowserWidget::PollAndUpload()
{
	FCefSharedFrame Frame;
	const double nowSec = FPlatformTime::Seconds();
	bool bIdleRecoveryFullCopy = false;
	if (!TryAcquireFrame(Frame, bIdleRecoveryFullCopy, nowSec))
		return;
	if (TSharedPtr<FCefFrameReader> Reader = FrameReader.Pin())
	{
		const uint32 dropped = Reader->ConsumeDroppedPendingFrames();
		if (dropped > 0)
		{
			TelemetryFrameGapCount += dropped;
			SET_DWORD_STAT(STAT_CefTel_FrameGaps, TelemetryFrameGapCount);
		}
	}

	UpdateFrameGapTelemetry(Frame, bIdleRecoveryFullCopy);
	UpdateCadenceFeedback(nowSec, bIdleRecoveryFullCopy);
	MaybeUpdateCursor(Frame, nowSec);
	PushHostTuningIfNeeded(nowSec);

	SharedSlotCount = FMath::Clamp(Frame.SlotCount, 1u, MaxSharedSlots);
	EnsureRenderTarget(Frame.Width, Frame.Height);
	EnsureSharedRHI();

	const uint32 slotCount = FMath::Clamp(SharedSlotCount, 1u, MaxSharedSlots);
	const uint32 index = Frame.WriteSlot % slotCount;
	FTextureRHIRef SrcRHI = SharedTextureRHI[index];
	const bool bUsePopupPlane = (CVarCefWebUiUsePopupDedicatedPlane.GetValueOnAnyThread() != 0) &&
		((Frame.Flags & CefFrameFlag_PopupPlane) != 0);
	if (bUsePopupPlane)
		EnsurePopupPlaneRHI();
	FTextureRHIRef PopupSrcRHI = SharedPopupTextureRHI;
	FTextureResource* DstRes = RenderTarget ? RenderTarget->GetResource() : nullptr;
	WaitForProducerGpuFence(Frame.GpuFenceValue);
	const bool bUseDirtyRects = ResolveUseDirtyRects(Frame, nowSec);
	UpdateCopyTelemetry(Frame, bUseDirtyRects);

	LastUploadedFrame = Frame;
	bHasLastUploadedFrame = true;
	bLastUploadUsedDirty = bUseDirtyRects;
	LastUploadTimeSec = nowSec;

	const int32 dirtyInflatePx = FMath::Clamp(CVarCefWebUiDirtyRectInflatePx.GetValueOnAnyThread(), 0, kMaxDirtyRectInflatePx);
	const int32 coalesceGapPx = FMath::Clamp(CVarCefWebUiDirtyRectCoalesceGapPx.GetValueOnAnyThread(), 0, kMaxDirtyRectGapPx);
	const float dirtyAreaThreshold = FMath::Clamp(CVarCefWebUiDirtyAreaFullCopyThreshold.GetValueOnAnyThread(), 0.0f, 1.0f);

	ENQUEUE_RENDER_COMMAND(CefBlitToRenderTarget)(
		[SrcRHI, PopupSrcRHI, DstRes, Frame, bUseDirtyRects, bUsePopupPlane, dirtyInflatePx, coalesceGapPx, dirtyAreaThreshold](FRHICommandListImmediate& RHICmdList)
		{
			SCOPE_CYCLE_COUNTER(STAT_CefWidget_GPUBlit);
			if (!SrcRHI.IsValid() || !DstRes || !DstRes->TextureRHI)
				return;

			if (!bUseDirtyRects)
			{
				SCOPE_CYCLE_COUNTER(STAT_CefWidget_GPUBlitFull);
				RHICmdList.CopyTexture(SrcRHI, DstRes->TextureRHI, FRHICopyTextureInfo{});
			}
			else
			{
				SCOPE_CYCLE_COUNTER(STAT_CefWidget_GPUBlitDirty);

				FIntRect mergedRects[MAX_CEF_DIRTY_RECTS];
				uint32 mergedCount = 0;
				auto intersectsOrAdjacent = [coalesceGapPx](const FIntRect& a, const FIntRect& b)
				{
					// FIntRect uses min-inclusive/max-exclusive bounds.
					const int32 ax0 = a.Min.X - coalesceGapPx;
					const int32 ay0 = a.Min.Y - coalesceGapPx;
					const int32 ax1 = a.Max.X + coalesceGapPx;
					const int32 ay1 = a.Max.Y + coalesceGapPx;
					return !(ax1 < b.Min.X || b.Max.X < ax0 || ay1 < b.Min.Y || b.Max.Y < ay0);
				};
				auto unionRect = [](const FIntRect& a, const FIntRect& b)
				{
					return FIntRect(
						FMath::Min(a.Min.X, b.Min.X),
						FMath::Min(a.Min.Y, b.Min.Y),
						FMath::Max(a.Max.X, b.Max.X),
						FMath::Max(a.Max.Y, b.Max.Y));
				};

				const uint8 rectCount = FMath::Min<uint8>(Frame.DirtyCount, MAX_CEF_DIRTY_RECTS);
				for (uint8 i = 0; i < rectCount; ++i)
				{
					const FCefDirtyRect& r = Frame.DirtyRects[i];
					if (r.W <= 0 || r.H <= 0)
						continue;

					const int32 x0 = FMath::Clamp(r.X - dirtyInflatePx, 0, static_cast<int32>(Frame.Width));
					const int32 y0 = FMath::Clamp(r.Y - dirtyInflatePx, 0, static_cast<int32>(Frame.Height));
					const int32 x1 = FMath::Clamp(r.X + r.W + dirtyInflatePx, 0, static_cast<int32>(Frame.Width));
					const int32 y1 = FMath::Clamp(r.Y + r.H + dirtyInflatePx, 0, static_cast<int32>(Frame.Height));
					if (x1 <= x0 || y1 <= y0)
						continue;

					FIntRect rect(x0, y0, x1, y1);
					bool merged = false;
					for (uint32 m = 0; m < mergedCount; ++m)
					{
						if (intersectsOrAdjacent(mergedRects[m], rect))
						{
							mergedRects[m] = unionRect(mergedRects[m], rect);
							merged = true;
							break;
						}
					}
					if (!merged && mergedCount < MAX_CEF_DIRTY_RECTS)
						mergedRects[mergedCount++] = rect;
				}

				// Stable coalesce pass for merge chains.
				bool changed = true;
				while (changed)
				{
					changed = false;
					for (uint32 i = 0; i < mergedCount; ++i)
					{
						for (uint32 j = i + 1; j < mergedCount; )
						{
							if (intersectsOrAdjacent(mergedRects[i], mergedRects[j]))
							{
								mergedRects[i] = unionRect(mergedRects[i], mergedRects[j]);
								mergedRects[j] = mergedRects[mergedCount - 1];
								--mergedCount;
								changed = true;
								continue;
							}
							++j;
						}
					}
				}

				const int32 maxRectsPerFrame = FMath::Max(0, CVarCefWebUiMaxDirtyRectsPerFrame.GetValueOnAnyThread());
				uint64 mergedArea = 0;
				for (uint32 i = 0; i < mergedCount; ++i)
				{
					const int32 w = FMath::Max(0, mergedRects[i].Width());
					const int32 h = FMath::Max(0, mergedRects[i].Height());
					mergedArea += static_cast<uint64>(w) * static_cast<uint64>(h);
				}
				const uint64 fullArea = static_cast<uint64>(Frame.Width) * static_cast<uint64>(Frame.Height);
				const bool useFullCopy = (mergedCount == 0) ||
					(maxRectsPerFrame > 0 && mergedCount > static_cast<uint32>(maxRectsPerFrame)) ||
					(fullArea > 0 && (static_cast<double>(mergedArea) / static_cast<double>(fullArea)) > static_cast<double>(dirtyAreaThreshold));
				if (useFullCopy)
				{
					RHICmdList.CopyTexture(SrcRHI, DstRes->TextureRHI, FRHICopyTextureInfo{});
				}
				else
				{
					for (uint32 i = 0; i < mergedCount; ++i)
					{
						FRHICopyTextureInfo info;
						info.SourcePosition = FIntVector(mergedRects[i].Min.X, mergedRects[i].Min.Y, 0);
						info.DestPosition = FIntVector(mergedRects[i].Min.X, mergedRects[i].Min.Y, 0);
						info.Size = FIntVector(mergedRects[i].Width(), mergedRects[i].Height(), 1);
						RHICmdList.CopyTexture(SrcRHI, DstRes->TextureRHI, info);
					}
				}
			}

			// Overlay popup plane after main copy.
			if (bUsePopupPlane && PopupSrcRHI.IsValid() && Frame.bPopupVisible && Frame.PopupRect.W > 0 && Frame.PopupRect.H > 0)
			{
				const int32 x0 = FMath::Clamp(Frame.PopupRect.X, 0, static_cast<int32>(Frame.Width));
				const int32 y0 = FMath::Clamp(Frame.PopupRect.Y, 0, static_cast<int32>(Frame.Height));
				const int32 x1 = FMath::Clamp(Frame.PopupRect.X + Frame.PopupRect.W, 0, static_cast<int32>(Frame.Width));
				const int32 y1 = FMath::Clamp(Frame.PopupRect.Y + Frame.PopupRect.H, 0, static_cast<int32>(Frame.Height));
				if (x1 > x0 && y1 > y0)
				{
					FRHICopyTextureInfo popupInfo;
					popupInfo.SourcePosition = FIntVector(x0, y0, 0);
					popupInfo.DestPosition = FIntVector(x0, y0, 0);
					popupInfo.Size = FIntVector(x1 - x0, y1 - y0, 1);
					RHICmdList.CopyTexture(PopupSrcRHI, DstRes->TextureRHI, popupInfo);
				}
			}
		}
	);
	MaybeLogAndResetTelemetry(FPlatformTime::Seconds());
}

// ---- Input ------------------------------------------------------------------

FReply UCefWebUiBrowserWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	LastInputEventTimeSec = FPlatformTime::Seconds();
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
	LastInputEventTimeSec = FPlatformTime::Seconds();
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
	LastInputEventTimeSec = FPlatformTime::Seconds();
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
	LastInputEventTimeSec = FPlatformTime::Seconds();
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
	LastInputEventTimeSec = FPlatformTime::Seconds();
	TSharedPtr<FCefInputWriter> Writer = InputWriter.Pin();
	if (Writer)
		Writer->WriteKey(InKeyEvent.GetKeyCode(),
		                 InKeyEvent.GetModifierKeys().IsLeftControlDown() ? 0x0002 : 0, true);
	return FReply::Handled();
}

FReply UCefWebUiBrowserWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	LastInputEventTimeSec = FPlatformTime::Seconds();
	TSharedPtr<FCefInputWriter> Writer = InputWriter.Pin();
	if (Writer)
		Writer->WriteKey(InKeyEvent.GetKeyCode(),
		                 InKeyEvent.GetModifierKeys().IsLeftControlDown() ? 0x0002 : 0, false);
	return FReply::Handled();
}

FReply UCefWebUiBrowserWidget::NativeOnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent)
{
	LastInputEventTimeSec = FPlatformTime::Seconds();
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
