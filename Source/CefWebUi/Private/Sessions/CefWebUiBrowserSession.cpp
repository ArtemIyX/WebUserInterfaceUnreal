#include "Sessions/CefWebUiBrowserSession.h"

#include "Data/CefLoadState.h"
#include "Services/CefWebUiRuntime.h"
#include "Subsystems/CefWebUiGameInstanceSubsystem.h"
#include "Widgets/CefWebUiBrowserWidget.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"

void UCefWebUiBrowserSession::BeginDestroy()
{
	Shutdown();
	Super::BeginDestroy();
}

void UCefWebUiBrowserSession::Initialize(UCefWebUiGameInstanceSubsystem* InOwnerSubsystem, FName InSessionId)
{
	OwnerSubsystem = InOwnerSubsystem;
	SessionId = InSessionId;
	EnsureRuntimeStarted();
}

UCefWebUiBrowserWidget* UCefWebUiBrowserSession::GetWidget() const
{
	return Widget.Get();
}

UCefWebUiBrowserWidget* UCefWebUiBrowserSession::CreateOrGetWidget(
	TSubclassOf<UCefWebUiBrowserWidget> WidgetClass,
	APlayerController* PlayerController,
	int32 ZOrder)
{
	if (IsValid(Widget))
	{
		if (!Widget->IsInViewport())
		{
			Widget->AddToViewport(ZOrder);
		}
		return Widget.Get();
	}

	UCefWebUiGameInstanceSubsystem* Subsystem = OwnerSubsystem.Get();
	if (!Subsystem)
		return nullptr;
	EnsureRuntimeStarted();

	if (!WidgetClass)
	{
		WidgetClass = Subsystem->GetDefaultWidgetClass();
	}
	if (!WidgetClass)
		return nullptr;

	UGameInstance* GI = Subsystem->GetGameInstance();
	if (!GI)
		return nullptr;

	APlayerController* PC = PlayerController ? PlayerController : GI->GetFirstLocalPlayerController();
	if (!PC)
		return nullptr;

	UCefWebUiBrowserWidget* Created = CreateWidget<UCefWebUiBrowserWidget>(PC, WidgetClass);
	if (!Created)
		return nullptr;

	Created->SetBrowserSession(this);
	Created->AddToViewport(ZOrder);
	Widget = Created;
	return Created;
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

TWeakPtr<FCefFrameReader> UCefWebUiBrowserSession::GetFrameReaderPtr() const
{
	return Runtime ? Runtime->GetFrameReaderPtr() : TWeakPtr<FCefFrameReader>();
}

TWeakPtr<FCefInputWriter> UCefWebUiBrowserSession::GetInputWriterPtr() const
{
	return Runtime ? Runtime->GetInputWriterPtr() : TWeakPtr<FCefInputWriter>();
}

TWeakPtr<FCefControlWriter> UCefWebUiBrowserSession::GetControlWriterPtr() const
{
	return Runtime ? Runtime->GetControlWriterPtr() : TWeakPtr<FCefControlWriter>();
}

void UCefWebUiBrowserSession::BindWhenFinishedLoading(const FCefWebUiWhenFinishedLoadingDelegate& Callback)
{
	if (!Callback.IsBound())
		return;

	if (bInitialLoadingFinished)
	{
		FCefWebUiWhenFinishedLoadingDelegate Copy = Callback;
		Copy.Execute(this);
		return;
	}

	PendingFinishedLoadingCallbacks.Add(Callback);
}

void UCefWebUiBrowserSession::OnWidgetDestroyed(UCefWebUiBrowserWidget* InWidget)
{
	if (Widget == InWidget)
	{
		Widget = nullptr;
	}
}

void UCefWebUiBrowserSession::HandleWidgetLoadStateChanged(uint8 InState)
{
	if (bInitialLoadingFinished)
		return;
	if (InState != static_cast<uint8>(ECefLoadState::Ready))
		return;

	bInitialLoadingFinished = true;
	OnFinishedLoading.Broadcast(this);

	for (const FCefWebUiWhenFinishedLoadingDelegate& Callback : PendingFinishedLoadingCallbacks)
	{
		if (Callback.IsBound())
		{
			FCefWebUiWhenFinishedLoadingDelegate Copy = Callback;
			Copy.Execute(this);
		}
	}
	PendingFinishedLoadingCallbacks.Reset();
}

void UCefWebUiBrowserSession::EnsureRuntimeStarted()
{
	if (!Runtime)
	{
		Runtime = new FCefWebUiRuntime();
	}
	Runtime->EnsureStarted();
}

void UCefWebUiBrowserSession::ShutdownRuntime()
{
	if (!Runtime)
		return;
	Runtime->Shutdown();
	delete Runtime;
	Runtime = nullptr;
}
