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

void UCefWebUiBrowserSession::ShowInViewport(
	APlayerController* playerController,
	int32 zOrder,
	int32 browserWidth,
	int32 browserHeight)
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
			.BrowserWidth(FMath::Max(1, browserWidth))
			.BrowserHeight(FMath::Max(1, browserHeight));
	}
	else
	{
		BrowserSurfaceWidget->SetBrowserSession(this);
		BrowserSurfaceWidget->SetBrowserSize(FMath::Max(1, browserWidth), FMath::Max(1, browserHeight));
	}

	if (ViewportWidgetHost.IsValid())
	{
		gameViewportClient->RemoveViewportWidgetContent(ViewportWidgetHost.ToSharedRef());
		ViewportWidgetHost.Reset();
	}

	if (!playerController)
	{
		if (UCefWebUiGameInstanceSubsystem* subsystem = OwnerSubsystem.Get())
		{
			if (UGameInstance* gameInstance = subsystem->GetGameInstance())
			{
				playerController = gameInstance->GetFirstLocalPlayerController();
			}
		}
	}

	SAssignNew(ViewportWidgetHost, SWeakWidget)
		.PossiblyNullContent(BrowserSurfaceWidget.ToSharedRef());
	gameViewportClient->AddViewportWidgetContent(ViewportWidgetHost.ToSharedRef(), zOrder);
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

void UCefWebUiBrowserSession::OpenLocalFile(const FString& inLocalFilePath)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->OpenLocalFile(inLocalFilePath);
	}
}

void UCefWebUiBrowserSession::LoadHtmlString(const FString& inHtml)
{
	if (TSharedPtr<FCefControlWriter> controlWriter = GetOrOpenControlWriter())
	{
		controlWriter->LoadHtmlString(inHtml);
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

void UCefWebUiBrowserSession::HandleConsoleLogMessage(
	ECefConsoleLogLevel inLevel,
	const FString& inMessage,
	const FString& inSource,
	int32 inLine)
{
	const TCHAR* levelText = TEXT("Log");
	switch (inLevel)
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

	switch (inLevel)
	{
	case ECefConsoleLogLevel::Warning:
		UE_LOG(LogCefWebUiJsConsole, Warning,
			TEXT("[%s][%s] %s (%s:%d)"),
			*SessionId.ToString(),
			levelText,
			*inMessage,
			*inSource,
			inLine);
		break;
	case ECefConsoleLogLevel::Error:
		UE_LOG(LogCefWebUiJsConsole, Error,
			TEXT("[%s][%s] %s (%s:%d)"),
			*SessionId.ToString(),
			levelText,
			*inMessage,
			*inSource,
			inLine);
		break;
	default:
		UE_LOG(LogCefWebUiJsConsole, Log,
			TEXT("[%s][%s] %s (%s:%d)"),
			*SessionId.ToString(),
			levelText,
			*inMessage,
			*inSource,
			inLine);
		break;
	}

	OnConsoleMessage.Broadcast(inLevel, inMessage, inSource, inLine);
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
