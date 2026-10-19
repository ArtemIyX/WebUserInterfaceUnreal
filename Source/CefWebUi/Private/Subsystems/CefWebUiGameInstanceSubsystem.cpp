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
	FName InSessionId, TSubclassOf<UCefWebUiBrowserSession> InSessionClass)
{
	const FName key = NormalizeSessionId(InSessionId);
	if (TObjectPtr<UCefWebUiBrowserSession>* Found = Sessions.Find(key))
	{
		return Found->Get();
	}

	if (!InSessionClass)
	{
		InSessionClass = DefaultSessionClass;
	}
	if (!InSessionClass)
	{
		InSessionClass = UCefWebUiBrowserSession::StaticClass();
	}

	UCefWebUiBrowserSession* Session = NewObject<UCefWebUiBrowserSession>(this, InSessionClass);
	Session->Initialize(this, key);
	Sessions.Add(key, Session);
	return Session;
}

UCefWebUiBrowserSession* UCefWebUiGameInstanceSubsystem::GetSession(FName InSessionId) const
{
	const FName key = NormalizeSessionId(InSessionId);
	if (const TObjectPtr<UCefWebUiBrowserSession>* Found = Sessions.Find(key))
	{
		return Found->Get();
	}
	return nullptr;
}

void UCefWebUiGameInstanceSubsystem::DestroySession(FName InSessionId)
{
	const FName key = NormalizeSessionId(InSessionId);
	if (TObjectPtr<UCefWebUiBrowserSession>* Found = Sessions.Find(key))
	{
		if (IsValid(*Found))
		{
			(*Found)->Shutdown();
		}
		Sessions.Remove(key);
	}
}

void UCefWebUiGameInstanceSubsystem::ShowSessionInViewport(
	FName InSessionId,
	APlayerController* InPlayerController,
	int32 InZOrder,
	int32 InBrowserWidth,
	int32 InBrowserHeight)
{
	if (UCefWebUiBrowserSession* Session = GetOrCreateSession(InSessionId, nullptr))
	{
		Session->ShowInViewport(InPlayerController, InZOrder, InBrowserWidth, InBrowserHeight);
	}
}

void UCefWebUiGameInstanceSubsystem::HideSessionFromViewport(FName InSessionId)
{
	if (UCefWebUiBrowserSession* Session = GetSession(InSessionId))
	{
		Session->HideFromViewport();
	}
}

void UCefWebUiGameInstanceSubsystem::SetDefaultSessionClass(TSubclassOf<UCefWebUiBrowserSession> InSessionClass)
{
	DefaultSessionClass = InSessionClass;
}

FName UCefWebUiGameInstanceSubsystem::NormalizeSessionId(FName InSessionId) const
{
	return InSessionId.IsNone() ? FName(TEXT("Default")) : InSessionId;
}
