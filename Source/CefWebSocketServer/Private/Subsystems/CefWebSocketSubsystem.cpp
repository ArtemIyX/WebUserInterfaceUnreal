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
	const FCefWebSocketServerCreateOptions& InOptions,
	TSubclassOf<UCefWebSocketServerBase> InServerClass,
	TSubclassOf<UCefWebSocketClientBase> InClientClass,
	UCefWebSocketServerBase*& OutServer)
{
	FCefWebSocketServerCreateResult out;
	out.Result = ECefWebSocketCreateResult::Failed;
	out.BoundPort = 0;
	OutServer = nullptr;

	if (InOptions.NameId.IsNone())
	{
		UE_LOG(LogCefWebSocketServer, Error, TEXT("CreateOrGetServer: NameId is None"));
		return out;
	}

	if (UCefWebSocketServerBase* Existing = GetServer(InOptions.NameId))
	{
		OutServer = Existing;
		out.Result = ECefWebSocketCreateResult::ReturnedExisting;
		out.BoundPort = Existing->GetBoundPort();
		return out;
	}

	int32 basePort = 0;
	bool bInitialAdjusted = false;
	if (!TryResolvePort(InOptions.RequestedPort, basePort, bInitialAdjusted))
	{
		UE_LOG(LogCefWebSocketServer, Error, TEXT("CreateOrGetServer: Failed to resolve base port for %s"), *InOptions.NameId.ToString());
		return out;
	}

	const int32 maxScan = FMath::Max(1, CefWebSocketCVars::GetMaxPortScan());
	UClass* ServerUClass = InServerClass ? InServerClass.Get() : UCefWebSocketServerBase::StaticClass();

	for (int32 Attempt = 0; Attempt <= maxScan; ++Attempt)
	{
		const int32 candidatePort = basePort + Attempt;
		UCefWebSocketServerBase* NewServer = NewObject<UCefWebSocketServerBase>(this, ServerUClass);
		if (!NewServer)
		{
			continue;
		}

		if (NewServer->StartServerInternal(InOptions.NameId, candidatePort, InClientClass,
		                                   InOptions.InDefaultPayloadFormat, InOptions.InPipelineConfig))
		{
			Servers.Add(InOptions.NameId, NewServer);
			OutServer = NewServer;
			out.BoundPort = NewServer->GetBoundPort();
			out.Result = (Attempt > 0 || bInitialAdjusted) ? ECefWebSocketCreateResult::PortAutoAdjusted : ECefWebSocketCreateResult::Created;
			UE_LOG(LogCefWebSocketServer, Log, TEXT("CreateOrGetServer: Started '%s' on port %d"), *InOptions.NameId.ToString(), out.BoundPort);
			return out;
		}
	}

	UE_LOG(LogCefWebSocketServer, Error, TEXT("CreateOrGetServer: Failed to start '%s' after %d attempts"), *InOptions.NameId.ToString(), maxScan + 1);
	return out;
}

UCefWebSocketServerBase* UCefWebSocketSubsystem::GetServer(FName InNameId) const
{
	if (const TObjectPtr<UCefWebSocketServerBase>* Found = Servers.Find(InNameId))
	{
		return *Found;
	}
	return nullptr;
}

bool UCefWebSocketSubsystem::StopServer(FName InNameId)
{
	TObjectPtr<UCefWebSocketServerBase>* Found = Servers.Find(InNameId);
	if (!Found || !*Found)
	{
		return false;
	}

	(*Found)->StopServer();
	Servers.Remove(InNameId);
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
	TArray<UCefWebSocketServerBase*> out;
	out.Reserve(Servers.Num());
	for (const TPair<FName, TObjectPtr<UCefWebSocketServerBase>>& Pair : Servers)
	{
		if (Pair.Value)
		{
			out.Add(Pair.Value);
		}
	}
	return out;
}

bool UCefWebSocketSubsystem::TryResolvePort(int32 InRequestedPort, int32& OutResolvedPort, bool& bOutAdjusted) const
{
	OutResolvedPort = FMath::Max(1, InRequestedPort);
	bOutAdjusted = false;

	if (InRequestedPort <= 0)
	{
		OutResolvedPort = 7001;
		bOutAdjusted = true;
	}

	return true;
}
