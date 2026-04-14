#include "Subsystems/CefWebUiGameInstanceSubsystem.h"

#include "Sessions/CefWebUiBrowserSession.h"
#include "Widgets/CefWebUiBrowserWidget.h"

void UCefWebUiGameInstanceSubsystem::Deinitialize()
{
	for (TPair<FName, TObjectPtr<UCefWebUiBrowserSession>>& It : Sessions)
	{
		if (IsValid(It.Value))
		{
			It.Value->DestroyWidget();
		}
	}
	Sessions.Empty();
	Super::Deinitialize();
}

UCefWebUiBrowserSession* UCefWebUiGameInstanceSubsystem::GetOrCreateSession(FName SessionId)
{
	const FName Key = NormalizeSessionId(SessionId);
	if (TObjectPtr<UCefWebUiBrowserSession>* Found = Sessions.Find(Key))
	{
		return Found->Get();
	}

	UCefWebUiBrowserSession* Session = NewObject<UCefWebUiBrowserSession>(this);
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

UCefWebUiBrowserWidget* UCefWebUiGameInstanceSubsystem::GetSessionWidget(FName SessionId) const
{
	if (UCefWebUiBrowserSession* Session = GetSession(SessionId))
	{
		return Session->GetWidget();
	}
	return nullptr;
}

UCefWebUiBrowserWidget* UCefWebUiGameInstanceSubsystem::CreateOrGetSessionWidget(
	FName SessionId,
	TSubclassOf<UCefWebUiBrowserWidget> WidgetClass,
	APlayerController* PlayerController,
	int32 ZOrder)
{
	UCefWebUiBrowserSession* Session = GetOrCreateSession(SessionId);
	return Session ? Session->CreateOrGetWidget(WidgetClass, PlayerController, ZOrder) : nullptr;
}

void UCefWebUiGameInstanceSubsystem::SetDefaultWidgetClass(TSubclassOf<UCefWebUiBrowserWidget> InWidgetClass)
{
	DefaultWidgetClass = InWidgetClass;
}

FName UCefWebUiGameInstanceSubsystem::NormalizeSessionId(FName SessionId) const
{
	return SessionId.IsNone() ? FName(TEXT("Default")) : SessionId;
}
