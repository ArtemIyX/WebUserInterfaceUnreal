#include "Sessions/CefWebUiBrowserSession.h"

#include "CefWebUi.h"
#include "Data/CefLoadState.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Services/CefControlWriter.h"
#include "Services/CefConsoleLogReader.h"
#include "Services/CefFrameReader.h"
#include "Services/CefWebUiRuntime.h"
#include "Slate/CefBrowserSurface.h"
#include "Subsystems/CefWebUiGameInstanceSubsystem.h"
#include "Widgets/SWeakWidget.h"

#pragma region Lifecycle

UCefWebUiBrowserSession::UCefWebUiBrowserSession(const FObjectInitializer& InObjectInitializer)
	: Super(InObjectInitializer)
{
}

void UCefWebUiBrowserSession::BeginDestroy()
{
	Shutdown();
	Super::BeginDestroy();
}

void UCefWebUiBrowserSession::Initialize(UCefWebUiGameInstanceSubsystem* InInOwnerSubsystem, FName InInSessionId)
{
	OwnerSubsystem = InInOwnerSubsystem;
	SessionId = InInSessionId;
	EnsureRuntimeStarted();
}

#pragma endregion

#pragma region Widget

void UCefWebUiBrowserSession::ShowInViewport(
	APlayerController* InPlayerController,
	int32 InZOrder,
	int32 InBrowserWidth,
	int32 InBrowserHeight)
{
	UGameViewportClient* gameViewportClient = GetGameViewportClient();
	if (!gameViewportClient)
	{
		return;
	}
	EnsureRuntimeStarted();

	if (!BrowserSurfaceWidget.IsValid())
	{
		SAssignNew(BrowserSurfaceWidget, SCefBrowserSurface)
			.BrowserSession(this)
			.BrowserWidth(FMath::Max(1, InBrowserWidth))
			.BrowserHeight(FMath::Max(1, InBrowserHeight));
	}
	else
	{
		BrowserSurfaceWidget->SetBrowserSession(this);
		BrowserSurfaceWidget->SetBrowserSize(FMath::Max(1, InBrowserWidth), FMath::Max(1, InBrowserHeight));
	}

	if (ViewportWidgetHost.IsValid())
	{
		gameViewportClient->RemoveViewportWidgetContent(ViewportWidgetHost.ToSharedRef());
		ViewportWidgetHost.Reset();
	}

	if (!InPlayerController)
	{
		if (UCefWebUiGameInstanceSubsystem* subsystem = OwnerSubsystem.Get())
		{
			if (UGameInstance* gameInstance = subsystem->GetGameInstance())
			{
				InPlayerController = gameInstance->GetFirstLocalPlayerController();
			}
		}
	}

	SAssignNew(ViewportWidgetHost, SWeakWidget)
		.PossiblyNullContent(BrowserSurfaceWidget.ToSharedRef());
	gameViewportClient->AddViewportWidgetContent(ViewportWidgetHost.ToSharedRef(), InZOrder);
}

void UCefWebUiBrowserSession::HideFromViewport()
{
	UGameViewportClient* gameViewportClient = GetGameViewportClient();
	if (!gameViewportClient || !ViewportWidgetHost.IsValid())
	{
		return;
	}

	gameViewportClient->RemoveViewportWidgetContent(ViewportWidgetHost.ToSharedRef());
	ViewportWidgetHost.Reset();
}

void UCefWebUiBrowserSession::Shutdown()
{
	HideFromViewport();
	BrowserSurfaceWidget.Reset();
	ShutdownRuntime();
}

bool UCefWebUiBrowserSession::IsShownInViewport() const
{
	return ViewportWidgetHost.IsValid();
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

void UCefWebUiBrowserSession::SetUrl(const FString& InInUrl)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->SetURL(InInUrl);
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

void UCefWebUiBrowserSession::SetZoomLevel(float InInLevel)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->SetZoomLevel(InInLevel);
	}
}

void UCefWebUiBrowserSession::SetFrameRate(int32 InInRate)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->SetFrameRate(static_cast<uint32>(FMath::Max(0, InInRate)));
	}
}

void UCefWebUiBrowserSession::ScrollTo(int32 InInX, int32 InInY)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->ScrollTo(InInX, InInY);
	}
}

void UCefWebUiBrowserSession::Resize(int32 InInWidth, int32 InInHeight)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->Resize(static_cast<uint32>(FMath::Max(1, InInWidth)), static_cast<uint32>(FMath::Max(1, InInHeight)));
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

void UCefWebUiBrowserSession::ExecuteJs(const FString& InInScript)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->ExecuteJS(InInScript);
	}
}

void UCefWebUiBrowserSession::OpenLocalFile(const FString& InInLocalFilePath)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->OpenLocalFile(InInLocalFilePath);
	}
}

void UCefWebUiBrowserSession::LoadHtmlString(const FString& InInHtml)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->LoadHtmlString(InInHtml);
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

void UCefWebUiBrowserSession::BindWhenFinishedLoading(const FCefWebUiWhenFinishedLoadingDelegate& InCallback)
{
	if (!InCallback.IsBound())
	{
		return;
	}

	EnsureRuntimeStarted();
	if (TSharedPtr<FCefFrameReader> frameReader = RuntimeFrameReader.Pin())
	{
		HandleWidgetLoadStateChanged(static_cast<uint8>(frameReader->GetLastKnownLoadState()));
	}

	if (bInitialLoadingFinished)
	{
		FCefWebUiWhenFinishedLoadingDelegate callbackCopy = InCallback;
		callbackCopy.Execute(this);
		return;
	}

	PendingFinishedLoadingCallbacks.Add(InCallback);
}

void UCefWebUiBrowserSession::HandleWidgetLoadStateChanged(uint8 InInState)
{
	UE_LOG(LogCefWebUi, Log, TEXT("[%s] Load state event: %u"),
		*SessionId.ToString(),
		static_cast<uint32>(InInState));

	if (bInitialLoadingFinished)
	{
		return;
	}
	const uint8 readyState = static_cast<uint8>(ECefLoadState::Ready);
	const uint8 errorState = static_cast<uint8>(ECefLoadState::Error);
	if (InInState != readyState && InInState != errorState)
	{
		return;
	}

	bInitialLoadingFinished = true;
	UE_LOG(LogCefWebUi, Log, TEXT("[%s] Initial loading finished (state=%u). callbacks=%d"),
		*SessionId.ToString(),
		static_cast<uint32>(InInState),
		PendingFinishedLoadingCallbacks.Num());
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

void UCefWebUiBrowserSession::HandleConsoleLogMessage(
	ECefConsoleLogLevel InInLevel,
	const FString& InInMessage,
	const FString& InInSource,
	int32 InInLine)
{
	const TCHAR* levelText = TEXT("Log");
	switch (InInLevel)
	{
	case ECefConsoleLogLevel::Warning:
		levelText = TEXT("Warning");
		break;
	case ECefConsoleLogLevel::Error:
		levelText = TEXT("Error");
		break;
	default:
		break;
	}

	switch (InInLevel)
	{
	case ECefConsoleLogLevel::Warning:
		UE_LOG(LogCefWebUiJsConsole, Warning,
			TEXT("[%s][%s] %s (%s:%d)"),
			*SessionId.ToString(),
			levelText,
			*InInMessage,
			*InInSource,
			InInLine);
		break;
	case ECefConsoleLogLevel::Error:
		UE_LOG(LogCefWebUiJsConsole, Error,
			TEXT("[%s][%s] %s (%s:%d)"),
			*SessionId.ToString(),
			levelText,
			*InInMessage,
			*InInSource,
			InInLine);
		break;
	default:
		UE_LOG(LogCefWebUiJsConsole, Log,
			TEXT("[%s][%s] %s (%s:%d)"),
			*SessionId.ToString(),
			levelText,
			*InInMessage,
			*InInSource,
			InInLine);
		break;
	}

	OnConsoleMessage.Broadcast(InInLevel, InInMessage, InInSource, InInLine);
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
	if (!RuntimeConsoleLogReader.IsValid())
	{
		RuntimeConsoleLogReader = Runtime->GetConsoleLogReaderPtr();
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
	if (TSharedPtr<FCefConsoleLogReader> consoleReader = RuntimeConsoleLogReader.Pin())
	{
		if (!ConsoleLogDelegateHandle.IsValid())
		{
			ConsoleLogDelegateHandle = consoleReader->OnConsoleLogMessage.AddUObject(
				this, &UCefWebUiBrowserSession::HandleConsoleLogMessage);
		}
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

UGameViewportClient* UCefWebUiBrowserSession::GetGameViewportClient() const
{
	UCefWebUiGameInstanceSubsystem* subsystem = OwnerSubsystem.Get();
	if (!subsystem)
	{
		return nullptr;
	}
	UGameInstance* gameInstance = subsystem->GetGameInstance();
	return gameInstance ? gameInstance->GetGameViewportClient() : nullptr;
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
	if (TSharedPtr<FCefConsoleLogReader> consoleReader = RuntimeConsoleLogReader.Pin())
	{
		if (ConsoleLogDelegateHandle.IsValid())
		{
			consoleReader->OnConsoleLogMessage.Remove(ConsoleLogDelegateHandle);
		}
	}
	ConsoleLogDelegateHandle.Reset();
	RuntimeConsoleLogReader.Reset();

	if (!Runtime)
	{
		return;
	}
	Runtime->Shutdown();
	Runtime.Reset();
}

#pragma endregion
