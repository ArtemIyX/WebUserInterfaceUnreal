#include "Sessions/CefWebUiBrowserSession.h"

#include "Data/CefLoadState.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Services/CefControlWriter.h"
#include "Services/CefFrameReader.h"
#include "Services/CefWebUiRuntime.h"
#include "Slate/CefWebUiSlateHostWidget.h"
#include "Subsystems/CefWebUiGameInstanceSubsystem.h"
#include "Widgets/CefWebUiBrowserWidget.h"

#pragma region Lifecycle

UCefWebUiBrowserSession::UCefWebUiBrowserSession(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
}

void UCefWebUiBrowserSession::BeginDestroy()
{
	Shutdown();
	Super::BeginDestroy();
}

void UCefWebUiBrowserSession::Initialize(UCefWebUiGameInstanceSubsystem* inOwnerSubsystem, FName inSessionId)
{
	OwnerSubsystem = inOwnerSubsystem;
	SessionId = inSessionId;
	EnsureRuntimeStarted();
}

#pragma endregion

#pragma region Widget

UCefWebUiSlateHostWidget* UCefWebUiBrowserSession::GetWidget() const
{
	return Widget.Get();
}

UCefWebUiSlateHostWidget* UCefWebUiBrowserSession::CreateOrGetWidget(
	TSubclassOf<UCefWebUiSlateHostWidget> widgetClass,
	APlayerController* playerController,
	int32 zOrder)
{
	if (IsValid(Widget))
	{
		if (!Widget->IsInViewport())
		{
			Widget->AddToViewport(zOrder);
		}
		return Widget.Get();
	}

	UCefWebUiGameInstanceSubsystem* subsystem = OwnerSubsystem.Get();
	if (!subsystem)
	{
		return nullptr;
	}
	EnsureRuntimeStarted();

	if (!widgetClass)
	{
		widgetClass = subsystem->GetDefaultWidgetClass();
	}
	if (!widgetClass)
	{
		return nullptr;
	}

	UGameInstance* gameInstance = subsystem->GetGameInstance();
	if (!gameInstance)
	{
		return nullptr;
	}

	APlayerController* targetPlayerController = playerController ? playerController : gameInstance->GetFirstLocalPlayerController();
	if (!targetPlayerController)
	{
		return nullptr;
	}

	UCefWebUiSlateHostWidget* createdWidget = CreateWidget<UCefWebUiSlateHostWidget>(targetPlayerController, widgetClass);
	if (!createdWidget)
	{
		return nullptr;
	}

	createdWidget->SetBrowserSession(this);
	createdWidget->AddToViewport(zOrder);
	Widget = createdWidget;
	return createdWidget;
}

void UCefWebUiBrowserSession::DestroyWidget()
{
	if (!IsValid(Widget))
	{
		Widget = nullptr;
		return;
	}

	Widget->SetBrowserSession(nullptr);
	Widget->RemoveFromParent();
	Widget = nullptr;
}

void UCefWebUiBrowserSession::Shutdown()
{
	DestroyWidget();
	ShutdownRuntime();
}

void UCefWebUiBrowserSession::OnWidgetDestroyed(UCefWebUiSlateHostWidget* inWidget)
{
	if (Widget == inWidget)
	{
		Widget = nullptr;
	}
}

#pragma endregion

#pragma region Runtime Access

TWeakPtr<FCefFrameReader> UCefWebUiBrowserSession::GetFrameReaderPtr() const
{
	return RuntimeFrameReader;
}

TWeakPtr<FCefInputWriter> UCefWebUiBrowserSession::GetInputWriterPtr() const
{
	return Runtime ? Runtime->GetInputWriterPtr() : TWeakPtr<FCefInputWriter>();
}

TWeakPtr<FCefControlWriter> UCefWebUiBrowserSession::GetControlWriterPtr() const
{
	return Runtime ? Runtime->GetControlWriterPtr() : TWeakPtr<FCefControlWriter>();
}

#pragma endregion

#pragma region Control

void UCefWebUiBrowserSession::GoBack()
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->GoBack();
	}
}

void UCefWebUiBrowserSession::GoForward()
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->GoForward();
	}
}

void UCefWebUiBrowserSession::StopLoad()
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->StopLoad();
	}
}

void UCefWebUiBrowserSession::Reload()
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->Reload();
	}
}

void UCefWebUiBrowserSession::SetUrl(const FString& inUrl)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->SetURL(inUrl);
	}
}

void UCefWebUiBrowserSession::SetPaused(bool bInPaused)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->SetPaused(bInPaused);
	}
}

void UCefWebUiBrowserSession::SetHidden(bool bInHidden)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->SetHidden(bInHidden);
	}
}

void UCefWebUiBrowserSession::SetFocus(bool bInFocus)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->SetFocus(bInFocus);
	}
}

void UCefWebUiBrowserSession::SetZoomLevel(float inLevel)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->SetZoomLevel(inLevel);
	}
}

void UCefWebUiBrowserSession::SetFrameRate(int32 inRate)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->SetFrameRate(static_cast<uint32>(FMath::Max(0, inRate)));
	}
}

void UCefWebUiBrowserSession::ScrollTo(int32 inX, int32 inY)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->ScrollTo(inX, inY);
	}
}

void UCefWebUiBrowserSession::Resize(int32 inWidth, int32 inHeight)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->Resize(static_cast<uint32>(FMath::Max(1, inWidth)), static_cast<uint32>(FMath::Max(1, inHeight)));
	}
}

void UCefWebUiBrowserSession::SetMuted(bool bInMuted)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->SetMuted(bInMuted);
	}
}

void UCefWebUiBrowserSession::OpenDevTools()
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->OpenDevTools();
	}
}

void UCefWebUiBrowserSession::CloseDevTools()
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->CloseDevTools();
	}
}

void UCefWebUiBrowserSession::SetInputEnabled(bool bInEnabled)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->SetInputEnabled(bInEnabled);
	}
}

void UCefWebUiBrowserSession::ExecuteJs(const FString& inScript)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->ExecuteJS(inScript);
	}
}

void UCefWebUiBrowserSession::ClearCookies()
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->ClearCookies();
	}
}

#pragma endregion

#pragma region Loading

void UCefWebUiBrowserSession::BindWhenFinishedLoading(const FCefWebUiWhenFinishedLoadingDelegate& callback)
{
	if (!callback.IsBound())
	{
		return;
	}

	if (bInitialLoadingFinished)
	{
		FCefWebUiWhenFinishedLoadingDelegate callbackCopy = callback;
		callbackCopy.Execute(this);
		return;
	}

	PendingFinishedLoadingCallbacks.Add(callback);
}

void UCefWebUiBrowserSession::HandleWidgetLoadStateChanged(uint8 inState)
{
	if (bInitialLoadingFinished)
	{
		return;
	}
	if (inState != static_cast<uint8>(ECefLoadState::Ready))
	{
		return;
	}

	bInitialLoadingFinished = true;
	OnFinishedLoading.Broadcast(this);

	for (const FCefWebUiWhenFinishedLoadingDelegate& callback : PendingFinishedLoadingCallbacks)
	{
		if (callback.IsBound())
		{
			FCefWebUiWhenFinishedLoadingDelegate callbackCopy = callback;
			callbackCopy.Execute(this);
		}
	}
	PendingFinishedLoadingCallbacks.Reset();
}

#pragma endregion

#pragma region Runtime Internal

void UCefWebUiBrowserSession::EnsureRuntimeStarted()
{
	if (!Runtime)
	{
		Runtime = MakeUnique<FCefWebUiRuntime>();
	}
	Runtime->EnsureStarted();

	if (!RuntimeFrameReader.IsValid())
	{
		RuntimeFrameReader = Runtime->GetFrameReaderPtr();
	}

	if (TSharedPtr<FCefFrameReader> frameReader = RuntimeFrameReader.Pin())
	{
		if (!LoadStateDelegateHandle.IsValid())
		{
			LoadStateDelegateHandle = frameReader->OnLoadStateChanged.AddUObject(
				this, &UCefWebUiBrowserSession::HandleWidgetLoadStateChanged);
		}
		HandleWidgetLoadStateChanged(static_cast<uint8>(frameReader->GetLastKnownLoadState()));
	}
}

TSharedPtr<FCefControlWriter> UCefWebUiBrowserSession::GetOrOpenControlWriter()
{
	EnsureRuntimeStarted();

	TSharedPtr<FCefControlWriter> controlWriter = GetControlWriterPtr().Pin();
	if (!controlWriter.IsValid())
	{
		return nullptr;
	}
	if (!controlWriter->IsOpen())
	{
		controlWriter->Open();
	}
	return controlWriter;
}

void UCefWebUiBrowserSession::ShutdownRuntime()
{
	if (TSharedPtr<FCefFrameReader> frameReader = RuntimeFrameReader.Pin())
	{
		if (LoadStateDelegateHandle.IsValid())
		{
			frameReader->OnLoadStateChanged.Remove(LoadStateDelegateHandle);
		}
	}
	LoadStateDelegateHandle.Reset();
	RuntimeFrameReader.Reset();

	if (!Runtime)
	{
		return;
	}
	Runtime->Shutdown();
	Runtime.Reset();
}

#pragma endregion
