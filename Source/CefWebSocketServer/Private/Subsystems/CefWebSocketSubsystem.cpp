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

	int32 ResolvedPort = 0;
	bool bAdjusted = false;
	if (!TryResolvePort(Options.RequestedPort, ResolvedPort, bAdjusted))
	{
		UE_LOG(LogCefWebSocketServer, Error, TEXT("CreateOrGetServer: Failed to resolve port for %s"), *Options.NameId.ToString());
		return Out;
	}

	UClass* ServerUClass = ServerClass ? ServerClass.Get() : UCefWebSocketServerBase::StaticClass();
	UCefWebSocketServerBase* NewServer = NewObject<UCefWebSocketServerBase>(this, ServerUClass);
	if (!NewServer)
	{
		UE_LOG(LogCefWebSocketServer, Error, TEXT("CreateOrGetServer: Failed to allocate server object for %s"), *Options.NameId.ToString());
		return Out;
	}

	if (!NewServer->StartServerInternal(Options.NameId, ResolvedPort, ClientClass))
	{
		UE_LOG(LogCefWebSocketServer, Error, TEXT("CreateOrGetServer: StartServerInternal failed for %s"), *Options.NameId.ToString());
		return Out;
	}

	Servers.Add(Options.NameId, NewServer);
	OutServer = NewServer;
	Out.BoundPort = ResolvedPort;
	Out.Result = bAdjusted ? ECefWebSocketCreateResult::PortAutoAdjusted : ECefWebSocketCreateResult::Created;
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

	const int32 MaxScan = FMath::Max(1, CefWebSocketCVars::GetMaxPortScan());
	TSet<int32> UsedPorts;
	for (const TPair<FName, TObjectPtr<UCefWebSocketServerBase>>& Pair : Servers)
	{
		if (Pair.Value)
		{
			UsedPorts.Add(Pair.Value->GetBoundPort());
		}
	}

	int32 Candidate = OutResolvedPort;
	for (int32 i = 0; i <= MaxScan; ++i)
	{
		if (!UsedPorts.Contains(Candidate))
		{
			OutResolvedPort = Candidate;
			bOutAdjusted = (Candidate != RequestedPort);
			return true;
		}
		++Candidate;
	}

	return false;
}
