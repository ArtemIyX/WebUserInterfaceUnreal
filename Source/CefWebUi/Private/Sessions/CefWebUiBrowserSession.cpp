#include "Sessions/CefWebUiBrowserSession.h"

#include "Data/CefLoadState.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
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
