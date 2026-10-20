#include "Tests/CefWebSocketGameThreadEventTestTypes.h"

#include "Config/CefWebSocketCVars.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTLS.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "UObject/UObjectGlobals.h"
#include "Server/CefWebSocketGameThreadEvent.h"
#include "Subsystems/CefWebSocketSubsystem.h"

namespace
{
constexpr EAutomationTestFlags GameThreadEventTestFlags =
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

}

class FCefWebSocketGameThreadEventTestAccess
{
public:
	static void BeginRun(UCefWebSocketGameThreadEventTestServer& InServer, FName InNameId)
	{
		check(IsInGameThread());
		InServer.ClearGameThreadEvents();
		++InServer.GameThreadEventState->Generation;
		InServer.GameThreadEventState->DroppedEvents.Store(0);
		InServer.GameThreadEventState->DroppedMessages.Store(0);
		InServer.GameThreadEventState->bControlOverflowStopRequested.Store(false);
		InServer.GameThreadEventState->bAcceptEvents.Store(true);
		InServer.NameId = InNameId;
		InServer.ClientClass = UCefWebSocketGameThreadEventTestClient::StaticClass();
	}

	static void NotifyConnected(UCefWebSocketServerBase& InServer, const FCefWebSocketClientInfo& InInfo)
	{
		InServer.NotifyClientConnected(InInfo);
	}

	static void NotifyDisconnected(UCefWebSocketServerBase& InServer, int64 InClientId,
		ECefWebSocketCloseReason InReason)
	{
		InServer.NotifyClientDisconnected(InClientId, InReason);
	}

	static void NotifyServerError(UCefWebSocketServerBase& InServer, ECefWebSocketErrorCode InErrorCode,
		const FString& InMessage)
	{
		InServer.NotifyServerError(InErrorCode, InMessage);
	}

	static void NotifyClientError(UCefWebSocketServerBase& InServer, int64 InClientId,
		ECefWebSocketErrorCode InErrorCode, const FString& InMessage)
	{
		InServer.NotifyClientError(InClientId, InErrorCode, InMessage);
	}

	static void NotifyMessage(UCefWebSocketServerBase& InServer, int64 InClientId,
		const TArray<uint8>& InPayload, bool bInBinary)
	{
		InServer.NotifyClientMessage(InClientId, InPayload, bInBinary);
	}

	static int32 Drain(UCefWebSocketServerBase& InServer, int32 InMaxEvents)
	{
		return InServer.DrainGameThreadEvents(InMaxEvents);
	}

	static int32 Depth(const UCefWebSocketServerBase& InServer)
	{
		return InServer.GetGameThreadEventQueueDepth();
	}

	static uint64 Generation(const UCefWebSocketServerBase& InServer)
	{
		return InServer.GameThreadEventState->Generation.Load();
	}

	static void Dispatch(UCefWebSocketServerBase& InServer, FCefGameThreadEvent&& InEvent)
	{
		InServer.DispatchGameThreadEvent(MoveTemp(InEvent));
	}

	static void PreparePendingClient(UCefWebSocketServerBase& InServer, int64 InClientId)
	{
		FScopeLock lock(&InServer.ClientObjectsLock);
		InServer.PendingClientMessages.FindOrAdd(InClientId);
	}

	static void AddServer(UCefWebSocketSubsystem& InSubsystem, FName InNameId,
		UCefWebSocketServerBase* InServer)
	{
		InSubsystem.Servers.Add(InNameId, InServer);
	}

	static void InvokeServerInitialized(UCefWebSocketServerBase& InServer, UCefWebSocketSubsystem* InSubsystem)
	{
		InServer.ServerInitialized(InSubsystem);
	}
};

namespace
{
class FTestRunnable final : public FRunnable
{
public:
	explicit FTestRunnable(TFunction<void()>&& InFunction)
		: Function(MoveTemp(InFunction))
	{
	}

	virtual uint32 Run() override
	{
		Function();
		return 0;
	}

private:
	TFunction<void()> Function;
};

void RunOnWorker(TFunction<void()>&& InFunction)
{
	FTestRunnable runnable(MoveTemp(InFunction));
	FRunnableThread* thread = FRunnableThread::Create(&runnable, TEXT("CefWsGameThreadEventTest"));
	if (!thread)
	{
		return;
	}
	thread->WaitForCompletion();
	delete thread;
}

void RunOnWorkers(TArray<TFunction<void()>>&& InFunctions)
{
	TArray<TUniquePtr<FTestRunnable>> runnables;
	TArray<FRunnableThread*> threads;
	runnables.Reserve(InFunctions.Num());
	threads.Reserve(InFunctions.Num());
	for (TFunction<void()>& function : InFunctions)
	{
		runnables.Add(MakeUnique<FTestRunnable>(MoveTemp(function)));
		threads.Add(FRunnableThread::Create(runnables.Last().Get(), TEXT("CefWsGameThreadEventTest")));
	}
	for (FRunnableThread* thread : threads)
	{
		if (thread)
		{
			thread->WaitForCompletion();
			delete thread;
		}
	}
}

UCefWebSocketGameThreadEventTestServer* MakeServer(FAutomationTestBase& InTest, FName InNameId = TEXT("Test"))
{
	UCefWebSocketGameThreadEventTestServer* server =
		NewObject<UCefWebSocketGameThreadEventTestServer>(GetTransientPackage());
	InTest.TestNotNull(TEXT("Test server created"), server);
	if (!server)
	{
		return nullptr;
	}
	FCefWebSocketGameThreadEventTestAccess::BeginRun(*server, InNameId);
	server->OnClientConnected.AddDynamic(server,
		&UCefWebSocketGameThreadEventTestServer::RecordConnectedDelegate);
	server->OnClientDisconnected.AddDynamic(server,
		&UCefWebSocketGameThreadEventTestServer::RecordDisconnectedDelegate);
	server->OnServerError.AddDynamic(server,
		&UCefWebSocketGameThreadEventTestServer::RecordServerErrorDelegate);
	server->OnClientError.AddDynamic(server,
		&UCefWebSocketGameThreadEventTestServer::RecordClientErrorDelegate);
	UCefWebSocketGameThreadEventTestClient::ActiveServer = server;
	return server;
}

FCefWebSocketClientInfo MakeClientInfo(int64 InClientId)
{
	FCefWebSocketClientInfo info;
	info.ClientId = InClientId;
	info.RemoteAddress = FString::Printf(TEXT("127.0.0.1:%lld"), InClientId);
	info.ConnectedAt = FDateTime::Now();
	return info;
}

UGameInstance* MakeGameInstance()
{
	return NewObject<UGameInstance>(GetTransientPackage());
}

struct FScopedCVarValue
{
	IConsoleVariable* Variable = nullptr;
	int32 PreviousValue = 0;

	FScopedCVarValue(const TCHAR* InName, int32 InValue)
	{
		Variable = IConsoleManager::Get().FindConsoleVariable(InName);
		if (Variable)
		{
			PreviousValue = Variable->GetInt();
			Variable->Set(InValue, ECVF_SetByCode);
		}
	}

	~FScopedCVarValue()
	{
		if (Variable)
		{
			Variable->Set(PreviousValue, ECVF_SetByCode);
		}
	}
};

int32 CountRecord(const UCefWebSocketGameThreadEventTestServer& InServer, const FString& InRecord)
{
	int32 count = 0;
	for (const FString& record : InServer.Records)
	{
		count += record == InRecord ? 1 : 0;
	}
	return count;
}

void CheckGameThreadCallbacks(FAutomationTestBase& InTest,
	const UCefWebSocketGameThreadEventTestServer& InServer)
{
	const uint32 gameThreadId = FPlatformTLS::GetCurrentThreadId();
	for (const uint32 callbackThreadId : InServer.CallbackThreadIds)
	{
		InTest.TestEqual(TEXT("Callback runs on the game thread"), static_cast<int64>(callbackThreadId),
			static_cast<int64>(gameThreadId));
	}
}
}

UCefWebSocketGameThreadEventTestServer* UCefWebSocketGameThreadEventTestClient::ActiveServer = nullptr;

void UCefWebSocketGameThreadEventTestClient::HandleBytesFromClient(const TArray<uint8>& InData)
{
	if (ActiveServer)
	{
		ActiveServer->RecordClientBytes();
	}
}

void UCefWebSocketGameThreadEventTestClient::HandleStringFromClient(const FString& InMessage)
{
	if (ActiveServer)
	{
		ActiveServer->RecordClientString(InMessage);
	}
}

void UCefWebSocketGameThreadEventTestServer::ResetRecords()
{
	Records.Reset();
	CallbackThreadIds.Reset();
	MessageIds.Reset();
	MessagePayloads.Reset();
	MessageBinaryFlags.Reset();
	MessageTexts.Reset();
	ConnectedClientVisibleInCallbacks.Reset();
	DisconnectedClientVisibleInCallbacks.Reset();
}

void UCefWebSocketGameThreadEventTestServer::RecordClientBytes()
{
	Records.Add(TEXT("ClientBytes"));
	CallbackThreadIds.Add(FPlatformTLS::GetCurrentThreadId());
}

void UCefWebSocketGameThreadEventTestServer::RecordClientString(const FString& InMessage)
{
	Records.Add(TEXT("ClientString"));
	CallbackThreadIds.Add(FPlatformTLS::GetCurrentThreadId());
	MessageTexts.Add(InMessage);
}

void UCefWebSocketGameThreadEventTestServer::RecordConnectedDelegate(const FCefWebSocketClientInfo& InClientInfo)
{
	Records.Add(TEXT("ConnectedDelegate"));
	CallbackThreadIds.Add(FPlatformTLS::GetCurrentThreadId());
	ConnectedClientVisibleInCallbacks.Add(IsValid(GetClient(InClientInfo.ClientId)));
}

void UCefWebSocketGameThreadEventTestServer::RecordDisconnectedDelegate(int64 InClientId,
	ECefWebSocketCloseReason InReason)
{
	Records.Add(TEXT("DisconnectedDelegate"));
	CallbackThreadIds.Add(FPlatformTLS::GetCurrentThreadId());
	DisconnectedClientVisibleInCallbacks.Add(IsValid(GetClient(InClientId)));
}

void UCefWebSocketGameThreadEventTestServer::RecordServerErrorDelegate(FName InServerName,
	ECefWebSocketErrorCode InErrorCode, const FString& InMessage)
{
	Records.Add(FString::Printf(TEXT("ServerErrorDelegate:%d:%s"), static_cast<int32>(InErrorCode), *InMessage));
	CallbackThreadIds.Add(FPlatformTLS::GetCurrentThreadId());
}

void UCefWebSocketGameThreadEventTestServer::RecordClientErrorDelegate(int64 InClientId,
	ECefWebSocketErrorCode InErrorCode, const FString& InMessage)
{
	Records.Add(FString::Printf(TEXT("ClientErrorDelegate:%lld:%d:%s"), InClientId,
		static_cast<int32>(InErrorCode), *InMessage));
	CallbackThreadIds.Add(FPlatformTLS::GetCurrentThreadId());
}

void UCefWebSocketGameThreadEventTestServer::HandleClientConnected(const FCefWebSocketClientInfo& InClientInfo)
{
	Records.Add(TEXT("ConnectedHandler"));
	CallbackThreadIds.Add(FPlatformTLS::GetCurrentThreadId());
	ConnectedClientVisibleInCallbacks.Add(IsValid(GetClient(InClientInfo.ClientId)));
	if (bStopOnConnected)
	{
		StopServer();
	}
}

void UCefWebSocketGameThreadEventTestServer::HandleClientDisconnected(int64 InClientId,
	ECefWebSocketCloseReason InReason)
{
	Records.Add(TEXT("DisconnectedHandler"));
	CallbackThreadIds.Add(FPlatformTLS::GetCurrentThreadId());
	DisconnectedClientVisibleInCallbacks.Add(IsValid(GetClient(InClientId)));
}

void UCefWebSocketGameThreadEventTestServer::HandleServerError(ECefWebSocketErrorCode InErrorCode,
	const FString& InMessage)
{
	Records.Add(FString::Printf(TEXT("ServerErrorHandler:%d:%s"), static_cast<int32>(InErrorCode), *InMessage));
	CallbackThreadIds.Add(FPlatformTLS::GetCurrentThreadId());
}

void UCefWebSocketGameThreadEventTestServer::HandleClientError(int64 InClientId,
	ECefWebSocketErrorCode InErrorCode, const FString& InMessage)
{
	Records.Add(FString::Printf(TEXT("ClientErrorHandler:%lld:%d:%s"), InClientId,
		static_cast<int32>(InErrorCode), *InMessage));
	CallbackThreadIds.Add(FPlatformTLS::GetCurrentThreadId());
}

void UCefWebSocketGameThreadEventTestServer::HandleClientMessage(int64 InClientId,
	const TArray<uint8>& InPayload, bool bInBinary)
{
	Records.Add(TEXT("GenericMessage"));
	CallbackThreadIds.Add(FPlatformTLS::GetCurrentThreadId());
	MessageIds.Add(InClientId);
	MessagePayloads.Add(InPayload);
	MessageBinaryFlags.Add(bInBinary);
}

void UCefWebSocketGameThreadEventTestServer::HandleClientBytes(UCefWebSocketClientBase* InClient,
	const TArray<uint8>& InData)
{
	Records.Add(TEXT("BytesHandler"));
	CallbackThreadIds.Add(FPlatformTLS::GetCurrentThreadId());
}

void UCefWebSocketGameThreadEventTestServer::HandleClientString(UCefWebSocketClientBase* InClient,
	const FString& InMessage)
{
	Records.Add(TEXT("StringHandler"));
	CallbackThreadIds.Add(FPlatformTLS::GetCurrentThreadId());
	MessageTexts.Add(InMessage);
}

void UCefWebSocketGameThreadEventTestServer::ServerInitialized(UCefWebSocketSubsystem* InCefSubsystem)
{
	Records.Add(TEXT("ServerInitialized"));
	CallbackThreadIds.Add(FPlatformTLS::GetCurrentThreadId());
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketWorkerThreadDefersDispatchTest,
	"CefWebSocket.GameThreadEvents.WorkerThreadDefersDispatch", GameThreadEventTestFlags)
bool FCefWebSocketWorkerThreadDefersDispatchTest::RunTest(const FString& Parameters)
{
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	const FCefWebSocketClientInfo info = MakeClientInfo(1);
	const TArray<uint8> payload{1, 2, 3};
	RunOnWorker([server, info, payload]() {
		FCefWebSocketGameThreadEventTestAccess::NotifyConnected(*server, info);
		FCefWebSocketGameThreadEventTestAccess::NotifyServerError(*server, ECefWebSocketErrorCode::Unknown, TEXT("server"));
		FCefWebSocketGameThreadEventTestAccess::NotifyClientError(*server, 1, ECefWebSocketErrorCode::InvalidPayload, TEXT("client"));
		FCefWebSocketGameThreadEventTestAccess::NotifyMessage(*server, 1, payload, true);
		FCefWebSocketGameThreadEventTestAccess::NotifyDisconnected(*server, 1, ECefWebSocketCloseReason::ClientClosed);
	});
	TestEqual(TEXT("No callbacks run before drain"), server->Records.Num(), 0);
	TestEqual(TEXT("All events are queued"), FCefWebSocketGameThreadEventTestAccess::Depth(*server), 5);
	FCefWebSocketGameThreadEventTestAccess::Drain(*server, 5);
	TestTrue(TEXT("Callbacks ran after drain"), server->Records.Num() > 0);
	CheckGameThreadCallbacks(*this, *server);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketGameThreadDispatchSynchronousTest,
	"CefWebSocket.GameThreadEvents.GameThreadDispatchIsSynchronous", GameThreadEventTestFlags)
bool FCefWebSocketGameThreadDispatchSynchronousTest::RunTest(const FString& Parameters)
{
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	FCefWebSocketGameThreadEventTestAccess::NotifyConnected(*server, MakeClientInfo(2));
	FCefWebSocketGameThreadEventTestAccess::NotifyServerError(*server, ECefWebSocketErrorCode::Unknown, TEXT("error"));
	FCefWebSocketGameThreadEventTestAccess::NotifyClientError(*server, 2, ECefWebSocketErrorCode::InvalidPayload, TEXT("error"));
	FCefWebSocketGameThreadEventTestAccess::NotifyMessage(*server, 2, TArray<uint8>{7}, true);
	FCefWebSocketGameThreadEventTestAccess::NotifyDisconnected(*server, 2, ECefWebSocketCloseReason::ClientClosed);
	TestEqual(TEXT("Game-thread notifications do not queue"), FCefWebSocketGameThreadEventTestAccess::Depth(*server), 0);
	TestTrue(TEXT("Callbacks run synchronously"), server->Records.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketMixedEventsPreserveFifoTest,
	"CefWebSocket.GameThreadEvents.MixedEventsPreserveFifoAndPayloads", GameThreadEventTestFlags)
bool FCefWebSocketMixedEventsPreserveFifoTest::RunTest(const FString& Parameters)
{
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	const TArray<uint8> binaryPayload{0xAA, 0xBB};
	const TArray<uint8> textPayload{ 'o', 'k' };
	RunOnWorker([server, binaryPayload, textPayload]() {
		FCefWebSocketGameThreadEventTestAccess::NotifyConnected(*server, MakeClientInfo(3));
		FCefWebSocketGameThreadEventTestAccess::NotifyServerError(*server, ECefWebSocketErrorCode::Unknown, TEXT("s"));
		FCefWebSocketGameThreadEventTestAccess::NotifyClientError(*server, 3, ECefWebSocketErrorCode::InvalidPayload, TEXT("c"));
		FCefWebSocketGameThreadEventTestAccess::NotifyMessage(*server, 3, binaryPayload, true);
		FCefWebSocketGameThreadEventTestAccess::NotifyMessage(*server, 3, textPayload, false);
		FCefWebSocketGameThreadEventTestAccess::NotifyDisconnected(*server, 3, ECefWebSocketCloseReason::Timeout);
	});
	FCefWebSocketGameThreadEventTestAccess::Drain(*server, 6);
	TestEqual(TEXT("Message count is preserved"), server->MessagePayloads.Num(), 2);
	if (server->MessagePayloads.Num() >= 2 && server->MessageIds.Num() >= 2 && server->MessageTexts.Num() > 0)
	{
		TestTrue(TEXT("First payload is binary"), server->MessagePayloads[0] == binaryPayload);
		TestTrue(TEXT("Second payload is text bytes"), server->MessagePayloads[1] == textPayload);
		TestEqual(TEXT("Message client ids are preserved"), server->MessageIds[0], int64(3));
		TestEqual(TEXT("Message client ids remain ordered"), server->MessageIds[1], int64(3));
		TestEqual(TEXT("Text content is decoded"), server->MessageTexts.Last(), FString(TEXT("ok")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketFrameBudgetExactTest,
	"CefWebSocket.GameThreadEvents.FrameBudgetIsExact", GameThreadEventTestFlags)
bool FCefWebSocketFrameBudgetExactTest::RunTest(const FString& Parameters)
{
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	RunOnWorker([server]() {
		for (int32 index = 0; index < 513; ++index)
		{
			FCefWebSocketGameThreadEventTestAccess::NotifyServerError(*server, ECefWebSocketErrorCode::Unknown,
				FString::FromInt(index));
		}
	});
	TestEqual(TEXT("Drain executes exactly the budget"), FCefWebSocketGameThreadEventTestAccess::Drain(*server, 512), 512);
	TestEqual(TEXT("One event remains"), FCefWebSocketGameThreadEventTestAccess::Depth(*server), 1);
	TestEqual(TEXT("Final event drains next"), FCefWebSocketGameThreadEventTestAccess::Drain(*server, 512), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketGlobalBudgetAcrossServersTest,
	"CefWebSocket.GameThreadEvents.GlobalBudgetAcrossServers", GameThreadEventTestFlags)
bool FCefWebSocketGlobalBudgetAcrossServersTest::RunTest(const FString& Parameters)
{
	FScopedCVarValue budget(TEXT("cefws.game_thread_events_per_tick"), 512);
	UCefWebSocketGameThreadEventTestServer* first = MakeServer(*this, TEXT("First"));
	UCefWebSocketGameThreadEventTestServer* second = MakeServer(*this, TEXT("Second"));
	if (!first || !second)
	{
		return false;
	}
	UGameInstance* gameInstance = MakeGameInstance();
	UCefWebSocketSubsystem* subsystem = NewObject<UCefWebSocketSubsystem>(gameInstance);
	FCefWebSocketGameThreadEventTestAccess::AddServer(*subsystem, TEXT("First"), first);
	FCefWebSocketGameThreadEventTestAccess::AddServer(*subsystem, TEXT("Second"), second);
	RunOnWorker([first, second]() {
		for (int32 index = 0; index < 400; ++index)
		{
			FCefWebSocketGameThreadEventTestAccess::NotifyServerError(*first, ECefWebSocketErrorCode::Unknown, TEXT("a"));
			FCefWebSocketGameThreadEventTestAccess::NotifyServerError(*second, ECefWebSocketErrorCode::Unknown, TEXT("b"));
		}
	});
	subsystem->Tick(0.0f);
	TestEqual(TEXT("Combined callbacks obey the global budget"),
		CountRecord(*first, TEXT("ServerErrorHandler:255:a")) + CountRecord(*second, TEXT("ServerErrorHandler:255:b")), 512);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketBusyServerRoundRobinTest,
	"CefWebSocket.GameThreadEvents.BusyServerDoesNotStarveOtherServers", GameThreadEventTestFlags)
bool FCefWebSocketBusyServerRoundRobinTest::RunTest(const FString& Parameters)
{
	FScopedCVarValue budget(TEXT("cefws.game_thread_events_per_tick"), 2);
	UCefWebSocketGameThreadEventTestServer* first = MakeServer(*this, TEXT("Busy"));
	UCefWebSocketGameThreadEventTestServer* second = MakeServer(*this, TEXT("Other"));
	if (!first || !second)
	{
		return false;
	}
	UGameInstance* gameInstance = MakeGameInstance();
	UCefWebSocketSubsystem* subsystem = NewObject<UCefWebSocketSubsystem>(gameInstance);
	FCefWebSocketGameThreadEventTestAccess::AddServer(*subsystem, TEXT("Busy"), first);
	FCefWebSocketGameThreadEventTestAccess::AddServer(*subsystem, TEXT("Other"), second);
	RunOnWorker([first, second]() {
		for (int32 index = 0; index < 8; ++index)
		{
			FCefWebSocketGameThreadEventTestAccess::NotifyServerError(*first, ECefWebSocketErrorCode::Unknown, TEXT("busy"));
		}
		FCefWebSocketGameThreadEventTestAccess::NotifyServerError(*second, ECefWebSocketErrorCode::Unknown, TEXT("other"));
	});
	subsystem->Tick(0.0f);
	TestEqual(TEXT("Other server receives service in the first pass"),
		CountRecord(*second, TEXT("ServerErrorHandler:255:other")), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketConcurrentProducersLosslessTest,
	"CefWebSocket.GameThreadEvents.ConcurrentProducersAreLosslessWithinCapacity", GameThreadEventTestFlags)
bool FCefWebSocketConcurrentProducersLosslessTest::RunTest(const FString& Parameters)
{
	FScopedCVarValue capacity(TEXT("cefws.game_thread_event_queue_max"), 256);
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	FCefWebSocketGameThreadEventTestAccess::NotifyConnected(*server, MakeClientInfo(4));
	server->ResetRecords();
	TArray<TFunction<void()>> producerFunctions;
	for (int32 producer = 0; producer < 4; ++producer)
	{
		producerFunctions.Add([server, producer]() {
			for (int32 index = 0; index < 16; ++index)
			{
				const TArray<uint8> payload{static_cast<uint8>(producer * 16 + index)};
				FCefWebSocketGameThreadEventTestAccess::NotifyMessage(*server, 4, payload, true);
			}
		});
	}
	RunOnWorkers(MoveTemp(producerFunctions));
	FCefWebSocketGameThreadEventTestAccess::Drain(*server, 128);
	TSet<uint8> ids;
	for (const TArray<uint8>& payload : server->MessagePayloads)
	{
		if (payload.Num() == 1)
		{
			ids.Add(payload[0]);
		}
	}
	TestEqual(TEXT("Every producer payload is delivered once"), ids.Num(), 64);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketQueueCapacityAtomicTest,
	"CefWebSocket.GameThreadEvents.QueueCapacityIsAtomic", GameThreadEventTestFlags)
bool FCefWebSocketQueueCapacityAtomicTest::RunTest(const FString& Parameters)
{
	FScopedCVarValue capacity(TEXT("cefws.game_thread_event_queue_max"), 8);
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	constexpr int32 attempts = 64;
	TArray<TFunction<void()>> producerFunctions;
	for (int32 producer = 0; producer < 4; ++producer)
	{
		producerFunctions.Add([server, producer, attempts]() {
			for (int32 index = producer; index < attempts; index += 4)
			{
				FCefWebSocketGameThreadEventTestAccess::NotifyMessage(*server, 5,
					TArray<uint8>{static_cast<uint8>(index)}, true);
			}
		});
	}
	RunOnWorkers(MoveTemp(producerFunctions));
	const int32 accepted = FCefWebSocketGameThreadEventTestAccess::Depth(*server);
	const FCefWebSocketServerStats stats = server->GetStats();
	TestTrue(TEXT("Accepted events do not exceed capacity"), accepted <= 8);
	TestEqual(TEXT("Rejected count is exact"), stats.DroppedGameThreadEvents, int64(attempts - accepted));
	FCefWebSocketGameThreadEventTestAccess::Drain(*server, attempts);
	TestEqual(TEXT("Queue depth returns to zero"), FCefWebSocketGameThreadEventTestAccess::Depth(*server), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketMessageOverflowNoRecursionTest,
	"CefWebSocket.GameThreadEvents.MessageOverflowDoesNotRecurse", GameThreadEventTestFlags)
bool FCefWebSocketMessageOverflowNoRecursionTest::RunTest(const FString& Parameters)
{
	FScopedCVarValue capacity(TEXT("cefws.game_thread_event_queue_max"), 4);
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	RunOnWorker([server]() {
		for (int32 index = 0; index < 4; ++index)
		{
			FCefWebSocketGameThreadEventTestAccess::NotifyMessage(*server, 6, TArray<uint8>{static_cast<uint8>(index)}, true);
		}
	});
	TestEqual(TEXT("Message overflow leaves only reserved capacity events queued"),
		FCefWebSocketGameThreadEventTestAccess::Depth(*server), 3);
	TestEqual(TEXT("One message is dropped"), server->GetStats().DroppedGameThreadEvents, int64(1));
	TestEqual(TEXT("Overflow does not enqueue an error"), server->Records.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketControlEventsRemainConsistentTest,
	"CefWebSocket.GameThreadEvents.ControlEventsRemainConsistentAtCapacity", GameThreadEventTestFlags)
bool FCefWebSocketControlEventsRemainConsistentTest::RunTest(const FString& Parameters)
{
	FScopedCVarValue capacity(TEXT("cefws.game_thread_event_queue_max"), 4);
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	FCefWebSocketGameThreadEventTestAccess::NotifyConnected(*server, MakeClientInfo(7));
	server->ResetRecords();
	RunOnWorker([server]() {
		for (int32 index = 0; index < 3; ++index)
		{
			FCefWebSocketGameThreadEventTestAccess::NotifyMessage(*server, 7, TArray<uint8>{static_cast<uint8>(index)}, true);
		}
		FCefWebSocketGameThreadEventTestAccess::NotifyDisconnected(*server, 7, ECefWebSocketCloseReason::Error);
	});
	TestEqual(TEXT("Control event uses reserved capacity"), FCefWebSocketGameThreadEventTestAccess::Depth(*server), 4);
	FCefWebSocketGameThreadEventTestAccess::Drain(*server, 4);
	TestFalse(TEXT("Disconnect removes the client"), IsValid(server->GetClient(7)));
	TestEqual(TEXT("Disconnect callback runs"), CountRecord(*server, TEXT("DisconnectedHandler")), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketStopRejectsAndFlushesTest,
	"CefWebSocket.GameThreadEvents.StopRejectsAndFlushesEvents", GameThreadEventTestFlags)
bool FCefWebSocketStopRejectsAndFlushesTest::RunTest(const FString& Parameters)
{
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	RunOnWorker([server]() {
		for (int32 index = 0; index < 5; ++index)
		{
			FCefWebSocketGameThreadEventTestAccess::NotifyServerError(*server, ECefWebSocketErrorCode::Unknown, TEXT("queued"));
		}
	});
	server->StopServer();
	TestEqual(TEXT("Stop flushes queued events"), FCefWebSocketGameThreadEventTestAccess::Depth(*server), 0);
	TestEqual(TEXT("No callback runs after stop"), server->Records.Num(), 0);
	RunOnWorker([server]() {
		FCefWebSocketGameThreadEventTestAccess::NotifyServerError(*server, ECefWebSocketErrorCode::Unknown, TEXT("rejected"));
	});
	TestEqual(TEXT("Stopped server rejects new events"), FCefWebSocketGameThreadEventTestAccess::Depth(*server), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketRestartRejectsOldGenerationTest,
	"CefWebSocket.GameThreadEvents.RestartRejectsOldGeneration", GameThreadEventTestFlags)
bool FCefWebSocketRestartRejectsOldGenerationTest::RunTest(const FString& Parameters)
{
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	const uint64 oldGeneration = FCefWebSocketGameThreadEventTestAccess::Generation(*server);
	server->StopServer();
	FCefWebSocketGameThreadEventTestAccess::BeginRun(*server, TEXT("Restarted"));
	FCefServerErrorEvent stalePayload;
	stalePayload.ErrorCode = ECefWebSocketErrorCode::Unknown;
	stalePayload.Message = TEXT("stale");
	FCefWebSocketGameThreadEventTestAccess::Dispatch(*server,
		FCefGameThreadEvent(oldGeneration, MoveTemp(stalePayload)));
	RunOnWorker([server]() {
		FCefWebSocketGameThreadEventTestAccess::NotifyServerError(*server, ECefWebSocketErrorCode::Unknown, TEXT("new"));
	});
	FCefWebSocketGameThreadEventTestAccess::Drain(*server, 1);
	TestEqual(TEXT("Only the current generation is delivered"),
		CountRecord(*server, TEXT("ServerErrorHandler:255:new")), 1);
	TestEqual(TEXT("Stale event is discarded"), CountRecord(*server, TEXT("ServerErrorHandler:255:stale")), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketStopDuringCallbackSafeTest,
	"CefWebSocket.GameThreadEvents.StopDuringCallbackIsSafe", GameThreadEventTestFlags)
bool FCefWebSocketStopDuringCallbackSafeTest::RunTest(const FString& Parameters)
{
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	server->bStopOnConnected = true;
	RunOnWorker([server]() {
		FCefWebSocketGameThreadEventTestAccess::NotifyConnected(*server, MakeClientInfo(8));
		FCefWebSocketGameThreadEventTestAccess::NotifyServerError(*server, ECefWebSocketErrorCode::Unknown, TEXT("later"));
	});
	FCefWebSocketGameThreadEventTestAccess::Drain(*server, 2);
	TestEqual(TEXT("Callback may stop and clear its server safely"), CountRecord(*server, TEXT("ServerErrorHandler:255:later")), 0);
	TestEqual(TEXT("Queue is clear after reentrant stop"), FCefWebSocketGameThreadEventTestAccess::Depth(*server), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketClientConnectedOrderingTest,
	"CefWebSocket.GameThreadEvents.ClientConnectedCreatesClientBeforeCallbacks", GameThreadEventTestFlags)
bool FCefWebSocketClientConnectedOrderingTest::RunTest(const FString& Parameters)
{
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	FCefWebSocketGameThreadEventTestAccess::NotifyConnected(*server, MakeClientInfo(9));
	TestEqual(TEXT("Both connection callbacks see the initialized client"), server->ConnectedClientVisibleInCallbacks.Num(), 2);
	for (const bool bVisible : server->ConnectedClientVisibleInCallbacks)
	{
		TestTrue(TEXT("Client exists before connection callback"), bVisible);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketMessageBeforeClientBufferedTest,
	"CefWebSocket.GameThreadEvents.MessageBeforeClientIsBuffered", GameThreadEventTestFlags)
bool FCefWebSocketMessageBeforeClientBufferedTest::RunTest(const FString& Parameters)
{
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	FCefWebSocketGameThreadEventTestAccess::PreparePendingClient(*server, 10);
	const TArray<uint8> payload{42};
	FCefWebSocketGameThreadEventTestAccess::NotifyMessage(*server, 10, payload, true);
	TestEqual(TEXT("Pending message does not dispatch early"), server->MessagePayloads.Num(), 0);
	FCefWebSocketGameThreadEventTestAccess::NotifyConnected(*server, MakeClientInfo(10));
	TestEqual(TEXT("Pending message dispatches once after connection"), server->MessagePayloads.Num(), 1);
	if (server->MessagePayloads.Num() == 1)
	{
		TestTrue(TEXT("Pending payload is preserved"), server->MessagePayloads[0] == payload);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketDisconnectOrderingTest,
	"CefWebSocket.GameThreadEvents.DisconnectRemovesClientBeforeCallbacks", GameThreadEventTestFlags)
bool FCefWebSocketDisconnectOrderingTest::RunTest(const FString& Parameters)
{
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	FCefWebSocketGameThreadEventTestAccess::NotifyConnected(*server, MakeClientInfo(11));
	server->ResetRecords();
	FCefWebSocketGameThreadEventTestAccess::NotifyDisconnected(*server, 11, ECefWebSocketCloseReason::ClientClosed);
	TestEqual(TEXT("Both disconnect callbacks see no client"), server->DisconnectedClientVisibleInCallbacks.Num(), 2);
	for (const bool bVisible : server->DisconnectedClientVisibleInCallbacks)
	{
		TestFalse(TEXT("Client is removed before disconnect callback"), bVisible);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketGenericMessageOnceTest,
	"CefWebSocket.GameThreadEvents.GenericMessageHandlerRunsOnce", GameThreadEventTestFlags)
bool FCefWebSocketGenericMessageOnceTest::RunTest(const FString& Parameters)
{
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	FCefWebSocketGameThreadEventTestAccess::NotifyConnected(*server, MakeClientInfo(12));
	server->ResetRecords();
	FCefWebSocketGameThreadEventTestAccess::NotifyMessage(*server, 12, TArray<uint8>{1}, true);
	FCefWebSocketGameThreadEventTestAccess::NotifyMessage(*server, 12, TArray<uint8>{2}, false);
	TestEqual(TEXT("Generic hook runs once per message"), CountRecord(*server, TEXT("GenericMessage")), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketByteAndTextRoutingTest,
	"CefWebSocket.GameThreadEvents.ByteAndTextRoutingMatchesExistingContract", GameThreadEventTestFlags)
bool FCefWebSocketByteAndTextRoutingTest::RunTest(const FString& Parameters)
{
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	FCefWebSocketGameThreadEventTestAccess::NotifyConnected(*server, MakeClientInfo(13));
	server->ResetRecords();
	FCefWebSocketGameThreadEventTestAccess::NotifyMessage(*server, 13, TArray<uint8>{'b'}, true);
	TestEqual(TEXT("Binary message runs generic and bytes hooks"), CountRecord(*server, TEXT("GenericMessage")), 1);
	TestEqual(TEXT("Binary message runs bytes hook"), CountRecord(*server, TEXT("BytesHandler")), 1);
	TestEqual(TEXT("Binary message skips string hook"), CountRecord(*server, TEXT("StringHandler")), 0);
	FCefWebSocketGameThreadEventTestAccess::NotifyMessage(*server, 13, TArray<uint8>{'t'}, false);
	TestEqual(TEXT("Text message runs generic hook"), CountRecord(*server, TEXT("GenericMessage")), 2);
	TestEqual(TEXT("Text message runs bytes hook"), CountRecord(*server, TEXT("BytesHandler")), 2);
	TestEqual(TEXT("Text message runs string hook"), CountRecord(*server, TEXT("StringHandler")), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketInvalidClientMessageTest,
	"CefWebSocket.GameThreadEvents.InvalidClientMessageIsDiscarded", GameThreadEventTestFlags)
bool FCefWebSocketInvalidClientMessageTest::RunTest(const FString& Parameters)
{
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	if (!server)
	{
		return false;
	}
	FCefWebSocketGameThreadEventTestAccess::NotifyMessage(*server, 999, TArray<uint8>{1}, true);
	TestEqual(TEXT("Unknown client message is discarded"), server->Records.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCefWebSocketServerInitializedThreadTest,
	"CefWebSocket.GameThreadEvents.ServerInitializedRunsOnGameThread", GameThreadEventTestFlags)
bool FCefWebSocketServerInitializedThreadTest::RunTest(const FString& Parameters)
{
	UCefWebSocketGameThreadEventTestServer* server = MakeServer(*this);
	UGameInstance* gameInstance = MakeGameInstance();
	UCefWebSocketSubsystem* subsystem = NewObject<UCefWebSocketSubsystem>(gameInstance);
	if (!server || !subsystem)
	{
		return false;
	}
	server->ResetRecords();
	FCefWebSocketGameThreadEventTestAccess::InvokeServerInitialized(*server, subsystem);
	TestEqual(TEXT("ServerInitialized runs once"), CountRecord(*server, TEXT("ServerInitialized")), 1);
	CheckGameThreadCallbacks(*this, *server);
	return true;
}
