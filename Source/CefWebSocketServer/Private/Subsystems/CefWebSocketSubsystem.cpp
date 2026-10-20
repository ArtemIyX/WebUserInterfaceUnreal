#include "Subsystems/CefWebSocketSubsystem.h"

#include "Config/CefWebSocketCVars.h"
#include "Logging/CefWebSocketLog.h"
#include "Server/CefWebSocketServerBase.h"

UCefWebSocketSubsystem::UCefWebSocketSubsystem()
	: FTickableGameObject(ETickableTickType::NewObject)
{
}

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

void UCefWebSocketSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bInitialized = true;
	SetTickableTickType(ETickableTickType::Conditional);
}

void UCefWebSocketSubsystem::Deinitialize()
{
	bInitialized = false;
	SetTickableTickType(ETickableTickType::Never);
	StopAllServers();
	Super::Deinitialize();
}

FCefWebSocketServerCreateResult UCefWebSocketSubsystem::CreateOrGetServer(
	const FCefWebSocketServerCreateOptions& InOptions,
	TSubclassOf<UCefWebSocketServerBase> InServerClass,
	TSubclassOf<UCefWebSocketClientBase> InClientClass,
	UCefWebSocketServerBase*& OutServer)
{
	check(IsInGameThread());
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
		                                   InOptions.InPipelineConfig))
		{
			Servers.Add(InOptions.NameId, NewServer);
			OutServer = NewServer;
			out.BoundPort = NewServer->GetBoundPort();
			out.Result = (Attempt > 0 || bInitialAdjusted) ? ECefWebSocketCreateResult::PortAutoAdjusted : ECefWebSocketCreateResult::Created;
			UE_LOG(LogCefWebSocketServer, Log, TEXT("CreateOrGetServer: Started '%s' on port %d"), *InOptions.NameId.ToString(), out.BoundPort);
			
			check(IsInGameThread());
			NewServer->ServerInitialized(this);
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
	check(IsInGameThread());
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
	check(IsInGameThread());
	for (const TPair<FName, TObjectPtr<UCefWebSocketServerBase>>& Pair : Servers)
	{
		if (Pair.Value)
		{
			Pair.Value->StopServer();
		}
	}
	Servers.Empty();
	RoundRobinStartIndex = 0;
}

void UCefWebSocketSubsystem::Tick(float InDeltaTime)
{
	check(IsInGameThread());
	(void)InDeltaTime;

	int32 remainingBudget = FMath::Max(0, CefWebSocketCVars::GetGameThreadEventsPerTick());
	if (remainingBudget == 0)
	{
		return;
	}

	const TArray<UCefWebSocketServerBase*> servers = GetAllServers();
	if (servers.IsEmpty())
	{
		RoundRobinStartIndex = 0;
		return;
	}

	const int32 serverCount = servers.Num();
	const int32 startIndex = RoundRobinStartIndex % serverCount;
	while (remainingBudget > 0)
	{
		bool bDispatchedAny = false;
		for (int32 offset = 0; offset < serverCount && remainingBudget > 0; ++offset)
		{
			UCefWebSocketServerBase* server = servers[(startIndex + offset) % serverCount];
			if (!IsValid(server))
			{
				continue;
			}
			if (server->ConsumeControlEventOverflowStopRequest())
			{
				// A lost lifecycle event is more dangerous than dropping the affected server.
				server->StopServer();
				continue;
			}
			if (server->GetGameThreadEventQueueDepth() <= 0)
			{
				continue;
			}

			const int32 drainedCount = server->DrainGameThreadEvents(1);
			remainingBudget -= drainedCount;
			bDispatchedAny |= drainedCount > 0;
		}

		if (!bDispatchedAny)
		{
			break;
		}
	}

	RoundRobinStartIndex = (startIndex + 1) % serverCount;
}

TStatId UCefWebSocketSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCefWebSocketSubsystem, STATGROUP_Tickables);
}

bool UCefWebSocketSubsystem::IsTickable() const
{
	return bInitialized && !IsTemplate() && GetGameInstance() != nullptr;
}

UWorld* UCefWebSocketSubsystem::GetTickableGameObjectWorld() const
{
	// A game-instance subsystem owns one global dispatch budget across its worlds.
	return nullptr;
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
