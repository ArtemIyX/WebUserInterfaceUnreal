#include "Subsystems/CefWebSocketSubsystem.h"

#include "Config/CefWebSocketCVars.h"
#include "Logging/CefWebSocketLog.h"
#include "Server/CefWebSocketServerBase.h"

bool UCefWebSocketSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (const UGameInstance* GI = Cast<UGameInstance>(Outer))
	{
		if (GI->IsDedicatedServerInstance())
		{
			return false;
		}
	}
	return true;
}

void UCefWebSocketSubsystem::Deinitialize()
{
	StopAllServers();
	Super::Deinitialize();
}

FCefWebSocketServerCreateResult UCefWebSocketSubsystem::CreateOrGetServer(
	const FCefWebSocketServerCreateOptions& Options,
	TSubclassOf<UCefWebSocketServerBase> ServerClass,
	TSubclassOf<UCefWebSocketClientBase> ClientClass,
	UCefWebSocketServerBase*& OutServer)
{
	FCefWebSocketServerCreateResult Out;
	Out.Result = ECefWebSocketCreateResult::Failed;
	Out.BoundPort = 0;
	OutServer = nullptr;

	if (Options.NameId.IsNone())
	{
		UE_LOG(LogCefWebSocketServer, Error, TEXT("CreateOrGetServer: NameId is None"));
		return Out;
	}

	if (UCefWebSocketServerBase* Existing = GetServer(Options.NameId))
	{
		OutServer = Existing;
		Out.Result = ECefWebSocketCreateResult::ReturnedExisting;
		Out.BoundPort = Existing->GetBoundPort();
		return Out;
	}

	int32 BasePort = 0;
	bool bInitialAdjusted = false;
	if (!TryResolvePort(Options.RequestedPort, BasePort, bInitialAdjusted))
	{
		UE_LOG(LogCefWebSocketServer, Error, TEXT("CreateOrGetServer: Failed to resolve base port for %s"), *Options.NameId.ToString());
		return Out;
	}

	const int32 MaxScan = FMath::Max(1, CefWebSocketCVars::GetMaxPortScan());
	UClass* ServerUClass = ServerClass ? ServerClass.Get() : UCefWebSocketServerBase::StaticClass();

	for (int32 Attempt = 0; Attempt <= MaxScan; ++Attempt)
	{
		const int32 CandidatePort = BasePort + Attempt;
		UCefWebSocketServerBase* NewServer = NewObject<UCefWebSocketServerBase>(this, ServerUClass);
		if (!NewServer)
		{
			continue;
		}

		if (NewServer->StartServerInternal(Options.NameId, CandidatePort, ClientClass))
		{
			Servers.Add(Options.NameId, NewServer);
			OutServer = NewServer;
			Out.BoundPort = NewServer->GetBoundPort();
			Out.Result = (Attempt > 0 || bInitialAdjusted) ? ECefWebSocketCreateResult::PortAutoAdjusted : ECefWebSocketCreateResult::Created;
			UE_LOG(LogCefWebSocketServer, Log, TEXT("CreateOrGetServer: Started '%s' on port %d"), *Options.NameId.ToString(), Out.BoundPort);
			return Out;
		}
	}

	UE_LOG(LogCefWebSocketServer, Error, TEXT("CreateOrGetServer: Failed to start '%s' after %d attempts"), *Options.NameId.ToString(), MaxScan + 1);
	return Out;
}

UCefWebSocketServerBase* UCefWebSocketSubsystem::GetServer(FName NameId) const
{
	if (const TObjectPtr<UCefWebSocketServerBase>* Found = Servers.Find(NameId))
	{
		return *Found;
	}
	return nullptr;
}

bool UCefWebSocketSubsystem::StopServer(FName NameId)
{
	TObjectPtr<UCefWebSocketServerBase>* Found = Servers.Find(NameId);
	if (!Found || !*Found)
	{
		return false;
	}

	(*Found)->StopServer();
	Servers.Remove(NameId);
	return true;
}

void UCefWebSocketSubsystem::StopAllServers()
{
	for (const TPair<FName, TObjectPtr<UCefWebSocketServerBase>>& Pair : Servers)
	{
		if (Pair.Value)
		{
			Pair.Value->StopServer();
		}
	}
	Servers.Empty();
}

TArray<UCefWebSocketServerBase*> UCefWebSocketSubsystem::GetAllServers() const
{
	TArray<UCefWebSocketServerBase*> Out;
	Out.Reserve(Servers.Num());
	for (const TPair<FName, TObjectPtr<UCefWebSocketServerBase>>& Pair : Servers)
	{
		if (Pair.Value)
		{
			Out.Add(Pair.Value);
		}
	}
	return Out;
}

bool UCefWebSocketSubsystem::TryResolvePort(int32 RequestedPort, int32& OutResolvedPort, bool& bOutAdjusted) const
{
	OutResolvedPort = FMath::Max(1, RequestedPort);
	bOutAdjusted = false;

	if (RequestedPort <= 0)
	{
		OutResolvedPort = 7001;
		bOutAdjusted = true;
	}

	return true;
}
