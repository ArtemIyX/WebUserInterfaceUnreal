#include "Subsystems/CefWebUiGameInstanceSubsystem.h"

#include "Sessions/CefWebUiBrowserSession.h"

UCefWebUiGameInstanceSubsystem::UCefWebUiGameInstanceSubsystem()
{
	DefaultSessionClass = UCefWebUiBrowserSession::StaticClass();
}

void UCefWebUiGameInstanceSubsystem::Deinitialize()
{
	for (TPair<FName, TObjectPtr<UCefWebUiBrowserSession>>& It : Sessions)
	{
		if (IsValid(It.Value))
		{
			It.Value->Shutdown();
		}
	}
	Sessions.Empty();
	Super::Deinitialize();
}

UCefWebUiBrowserSession* UCefWebUiGameInstanceSubsystem::GetOrCreateSession(
	FName SessionId, TSubclassOf<UCefWebUiBrowserSession> SessionClass)
{
	const FName Key = NormalizeSessionId(SessionId);
	if (TObjectPtr<UCefWebUiBrowserSession>* Found = Sessions.Find(Key))
	{
		return Found->Get();
	}

	if (!SessionClass)
	{
		SessionClass = DefaultSessionClass;
	}
	if (!SessionClass)
	{
		SessionClass = UCefWebUiBrowserSession::StaticClass();
	}

	UCefWebUiBrowserSession* Session = NewObject<UCefWebUiBrowserSession>(this, SessionClass);
	Session->Initialize(this, Key);
	Sessions.Add(Key, Session);
	return Session;
}

UCefWebUiBrowserSession* UCefWebUiGameInstanceSubsystem::GetSession(FName SessionId) const
{
	const FName Key = NormalizeSessionId(SessionId);
	if (const TObjectPtr<UCefWebUiBrowserSession>* Found = Sessions.Find(Key))
	{
		return Found->Get();
	}
	return nullptr;
}

void UCefWebUiGameInstanceSubsystem::DestroySession(FName SessionId)
{
	const FName Key = NormalizeSessionId(SessionId);
	if (TObjectPtr<UCefWebUiBrowserSession>* Found = Sessions.Find(Key))
	{
		if (IsValid(*Found))
		{
			(*Found)->Shutdown();
		}
		Sessions.Remove(Key);
	}
}

void UCefWebUiGameInstanceSubsystem::ShowSessionInViewport(
	FName SessionId,
	APlayerController* PlayerController,
	int32 ZOrder,
	int32 BrowserWidth,
	int32 BrowserHeight)
{
	if (UCefWebUiBrowserSession* Session = GetOrCreateSession(SessionId, nullptr))
	{
		Session->ShowInViewport(PlayerController, ZOrder, BrowserWidth, BrowserHeight);
	}
}

void UCefWebUiGameInstanceSubsystem::HideSessionFromViewport(FName SessionId)
{
	if (UCefWebUiBrowserSession* Session = GetSession(SessionId))
	{
		Session->HideFromViewport();
	}
}

void UCefWebUiGameInstanceSubsystem::SetDefaultSessionClass(TSubclassOf<UCefWebUiBrowserSession> InSessionClass)
{
	DefaultSessionClass = InSessionClass;
}

FName UCefWebUiGameInstanceSubsystem::NormalizeSessionId(FName SessionId) const
{
	return SessionId.IsNone() ? FName(TEXT("Default")) : SessionId;
}
