# Project Log

Use this file to capture plugin changes so future sessions can quickly understand:
- what changed,
- why it changed,
- when it changed,
- and what should happen next.

---

## Entry Template

### Date
YYYY-MM-DD HH:MM

### Changed
- 

### Why
- 

### Impact
- 

---

## 2026-04-15 10:20

### Changed
- Refactored `SCefBrowserSurface` update flow:
  - Removed cvar-driven runtime tuning in surface code.
  - Switched from active timer invalidation to `OnFrameReady`-driven invalidation.
  - Added host tuning push with fixed values:
    - frame rate `60`
    - max in-flight begin frames `1`
    - flush interval `2`
    - keyframe interval `150000us`
  - Added shared-fence readiness gate with deferred retry/fail-open presentation path to avoid black-frame stalls.
- Refactored `FCefFrameReader` queue behavior:
  - Removed cvar-driven queue policy.
  - Added fixed small FIFO queue (`size=2`) with monotonic frame delivery checks.
  - Preserved load-state delegate delivery and dropped-frame telemetry counter path.
- Hardened load-finished callback path:
  - `BindWhenFinishedLoading` forces runtime/state refresh before queueing callback.
  - `FCefFrameReader::Start()` emits startup load-state once on game thread.
  - Session load-finished completion was corrected to require terminal states (`Ready`/`Error`), not `Idle`.
- Added session-level diagnostics:
  - Load-state transition log lines in `UCefWebUiBrowserSession`.

### Why
- Reduce jitter from unstable producer/consumer pacing and queue policy drift.
- Remove unstable runtime cvar combinations during debugging.
- Ensure `BindWhenFinishedLoading` does not miss transitions due to start-order races.

### Impact
- Rendering path restored with frame-driven invalidation and deterministic tuning values.
- Load-state callback flow is observable in logs and no longer tied only to paint-driven updates.
- Session callback sequencing is stricter and avoids early `Idle` completion.

---

## 2026-04-15 16:40

### Changed
- Tuned `SCefBrowserSurface` for smoother baseline without reintroducing black-screen regressions:
  - Added fixed active repaint timer at `60Hz` via `RegisterActiveTimer(1/60s, ...)`.
  - Added `HandleActiveTimer(...)` to invalidate paint on a stable cadence.
  - Disabled GPU fence readiness gating in surface path (`kUseGpuFenceCheck = false`).
  - Disabled host keyframe forcing push from plugin (`kHostKeyframeIntervalUs = 0`).

### Why
- Current jitter root was primarily cadence instability and occasional fence-gate stalls.
- Stable repaint cadence reduced visible snap/jitter while keeping rendering alive.

### Impact
- Telemetry improved to near-zero `gaps` and `fence_not_ready` in many windows.
- Rendering remained stable.
- Residual issue still exists: intermittent brief stutters/latency spikes, not fully resolved yet.

---

## 2026-04-15 20:10

### Changed
- Increased `FCefFrameReader` pending queue depth (`2 -> 8`) for burst tolerance.
- Switched frame-ready dispatch to per-event game-thread broadcast (removed coalescing gate).
- Updated `FCefFrameReader::PollSharedTexture` to newest-frame-first consumption:
  - drops stale queued frames,
  - presents latest available frame immediately,
  - preserves monotonic delivery checks.
- Updated `SCefBrowserSurface` cadence/diagnostics:
  - `ComputeVolatility()` returns `true`,
  - added `Tick(...)` backup paint invalidation,
  - kept active timer invalidation,
  - extended telemetry with:
    - `paint_calls`
    - `tick_calls`
    - `timer_calls`
    - `poll_success`

### Why
- Determine whether stutter/gaps were caused by reader queueing/coalescing or by UE widget paint cadence.
- Add direct observability of surface repaint cadence versus host frame production.

### Impact
- Diagnostics clearly showed UE editor-side cadence limit:
  - `paint_calls/tick_calls/timer_calls` near ~80 per 2s window (~40 FPS),
  - while host produced ~60 FPS.
- This mismatch explains persistent `gaps`; issue is primarily UE editor viewport cadence, not host IPC/render transport.
- In runtime/game mode at 60/120 FPS, playback is smooth and near-browser quality.

---

## 2026-04-17 12:52

### Changed
- Updated host tuning push default in `SCefBrowserSurface`:
  - `kHostMaxInFlightBeginFrames` set from `2` to `0`.

### Why
- User-observed interaction latency (slider drag and burst typing stalls) needed A/B with no begin-frame in-flight cap.

### Impact
- Plugin now requests uncapped begin-frame in-flight behavior from host by default for latency validation.

---

## 2026-04-17 13:35

### Changed
- Fixed keyboard shortcut modifier mapping in `SCefBrowserSurface`:
  - replaced incorrect key modifier encoding with CEF-compatible flags via `BuildCefKeyModifiers(...)`.
  - `OnKeyDown/OnKeyUp` now send full modifier state (Ctrl/Shift/Alt/Command, left/right, repeat, keypad).
  - `OnKeyChar` now suppresses char event forwarding while Ctrl/Alt/Command is held.
- Refactored `SCefBrowserSurface` support code:
  - moved constants to `Public/Slate/CefBrowserSurfaceConstants.h`.
  - moved free helper declarations to `Public/Slate/CefBrowserSurfaceFunctions.h`.
  - moved stats declarations to `Public/Slate/CefBrowserSurfaceStats.h`.
  - moved helper implementations to `Private/Slate/CefBrowserSurfaceFunctions.cpp`.
  - removed anonymous namespace usage for these helpers/constants.
- Fixed popup-plane readiness handling:
  - `EnsurePopupPlaneRhi()` result is now checked and used in paint/poll flow.
- Applied `#pragma region` only inside class body in `CefBrowserSurface.h` (removed file-level regions).

### Why
- `Ctrl+X` and other shortcuts were not working due to wrong modifier bit mapping.
- Requested code organization split by concern and region-style policy update.
- Popup-plane open failures needed explicit handling.

### Impact
- Browser shortcuts now map correctly through UE input path.
- Surface code is split into clearer units with public headers for shared constants/helpers.
- Popup-plane path now fails safely when shared popup texture is not ready.

---

## 2026-04-18 12:05

### Changed
- Phase 1 for new `CefWebSocketServer` module:
  - expanded module dependencies for websocket server/runtime/json support.
  - added separated public data files:
    - `Data/CefWebSocketEnums.h`
    - `Data/CefWebSocketStructs.h`
    - `Data/CefWebSocketDelegates.h`
  - added separated infrastructure files:
    - logs (`Private/Logging/CefWebSocketLog.h/.cpp`)
    - cvars (`Private/Config/CefWebSocketCVars.h/.cpp`)
    - stats (`Private/Stats/CefWebSocketStats.h/.cpp`)
  - added subsystem/server/client contracts:
    - `Subsystems/CefWebSocketSubsystem.h/.cpp`
    - `Server/CefWebSocketServerBase.h/.cpp`
    - `Server/CefWebSocketClientBase.h/.cpp`
  - added initial server instance scaffolding:
    - `Private/Server/CefWebSocketServerInstance.h/.cpp`
  - added backend interface contracts:
    - `Public/Networking/ICefNetWebSocket.h`
    - `Public/Networking/ICefWebSocketServerBackend.h`
    - `Public/Networking/CefNetWebSocketDelegates.h`

### Why
- Prepare stable API/ownership boundaries before implementing low-level websocket backend and FRunnable thread model.
- Keep files small and split by concern (enums/structs/logs/stats/cvars/subsystem).

### Impact
- `CefWebSocketServer` now has complete public contracts and module structure for phased implementation.
- Runtime behavior is still scaffold-level; backend threading and real socket flow are not wired yet.

---

## 2026-04-18 12:45

### Changed
- Phase 2 for `CefWebSocketServer`:
  - added low-level websocket backend implementation files:
    - `Private/Networking/CefWebSocketPrivate.h`
    - `Private/Networking/CefNetWebSocket.h/.cpp`
    - `Private/Networking/CefWebSocketServerBackend.h/.cpp`
  - updated packet callback delegate to include text/binary flag.
  - wired `FCefWebSocketServerInstance` to:
    - create and initialize backend,
    - bind connected/disconnected/packet callbacks,
    - register server tick through `FTSTicker`,
    - maintain real client maps (`ClientId <-> Socket`),
    - route client connect/disconnect/message notifications to `UCefWebSocketServerBase`.
  - updated `UCefWebSocketSubsystem::CreateOrGetServer` to attempt port auto-increment startup loop and return `PortAutoAdjusted` when needed.
  - implemented server-side UObject client creation/removal notifications in `UCefWebSocketServerBase`.

### Why
- Move from contracts-only scaffolding to actual server lifecycle and connection handling.
- Establish stable runtime path before introducing dedicated read/write FRunnable threads.

### Impact
- Servers now attempt to bind and run with real websocket backend callbacks.
- Client objects are created on connect and removed on disconnect.
- Next phase will migrate backend ticking/sending off ticker path to dedicated read/write threads.

---

## 2026-04-18 12:59

### Changed
- Phase 3 for `CefWebSocketServer` threading and dispatch:
  - added dedicated FRunnable thread files:
    - `Private/Threads/CefWebSocketReadRunnable.h/.cpp`
    - `Private/Threads/CefWebSocketWriteRunnable.h/.cpp`
  - migrated backend processing from ticker model to dedicated read/write thread flow in `FCefWebSocketServerInstance`:
    - read thread now ticks backend service loop.
    - write thread now drains per-client outbound queues.
    - added write wake event for low-idle spin.
  - added per-client outbound queue state and bounded queue behavior in server instance:
    - message/byte queue accounting per client,
    - oldest-drop pressure handling,
    - queue-full send result fallback.
  - updated `UCefWebSocketServerBase` dispatch behavior:
    - connect/disconnect/server-error/client-error notifications marshaled to game thread,
    - payload handling remains on read thread,
    - synchronized client UObject map with critical section.

### Why
- Implement required two-thread model (read/write) with scalable multi-client queues.
- Avoid ticker-coupled network pacing and keep callback safety for Blueprint-facing events.

### Impact
- Server runtime now uses dedicated websocket IO worker threads.
- Outbound sends are decoupled from caller thread and backpressure is bounded per client.
- BP delegates are now game-thread safe for lifecycle/error events.

---


---

## 2026-04-18 13:00

### Changed
- Phase 4 for `CefWebSocketServer` observability/debug tooling:
  - added split debug command files:
    - `Private/Debug/CefWebSocketDebugCommands.h/.cpp`
  - wired debug command lifecycle in module startup/shutdown.
  - added console commands:
    - `ws.list`
    - `ws.kick <server> <clientId>`
    - `ws.stop <name>`
    - `ws.stats <name>`

### Why
- Provide first-pass runtime inspection and operational controls without custom UI.

### Impact
- Runtime debugging of active servers/clients is available through UE console commands.

---

## 2026-04-18 20:06

### Changed
- Stabilized `CefWebSocketServer` compile/runtime integration after Phase 4:
  - fixed reflected delegate registration in `Data/CefWebSocketDelegates.h` by adding `UDELEGATE()` for dynamic multicast delegates.
  - fixed map value construction issue for `FCefClientState`:
    - changed per-client outbox to pointer-owned queue (`TUniquePtr<TQueue<...>>`) in `CefWebSocketServerInstance`.
  - fixed socket endpoint storage in `CefNetWebSocket`:
    - removed header-level `sockaddr_in` member,
    - stored remote endpoint as `RemoteIp` + `RemotePort`.
  - removed destructor-time virtual call warning:
    - added non-virtual `FlushInternal()` and switched `~FCefNetWebSocket()` to call it directly.
  - applied style normalization on touched websocket files:
    - lower camel case local variables,
    - `In...` prefix for command args in debug command handlers,
    - replaced plain `int` with `int32` where allowed by APIs.

### Why
- Resolve current build failures/warnings and enforce requested code conventions.

### Impact
- Websocket server code path compiles cleanly for the addressed issues.
- Networking/destructor behavior is safer and code style is consistent with project rules.

---

## 2026-04-18 20:10

### Changed
- Added websocket usage documentation:
  - `Source/CefWebSocketServer/README.md`
- Documented:
  - subsystem/server/client setup flow,
  - Blueprint and C++ quick start,
  - subclassing points,
  - JSON send API,
  - debug commands,
  - cvars and lifecycle notes.

### Why
- Provide a single practical reference for integrating and operating websocket servers in projects.

### Impact
- Team can onboard and test websocket server flow without reading implementation files first.

## 2026-04-19 10:38

### Changed
- Ran a full plugin style sweep across `Source` C++ headers/sources.
- Standardized function parameter naming to `In*`/`Out*` (including bool params as `bIn*`/`bOut*`) on touched APIs and implementations.
- Normalized function-scope local variables to lower camel case in touched files.
- Fixed rename fallout introduced during bulk pass (restored invalid member/lock names and event field accesses).
- Restored `CefWebUi/Private/Slate/CefBrowserSurface.cpp` from git to avoid bad token-level auto-renames.

### Why
- Enforce requested plugin-wide naming conventions.
- Reduce style drift and keep signatures/implementations consistent.

### Impact
- Plugin source is now aligned with requested param/local naming rules across the swept files.
- Known bulk-rename regressions were corrected before commit.
- `CefBrowserSurface.cpp` remained functionally unchanged in this commit due to explicit restore.

---

## 2026-04-19 11:48

### Changed
- Added `CefProtobuf` module build wiring for bundled Protobuf third-party runtime.
- Updated `Source/CefProtobuf/CefProtobuf.Build.cs`:
  - added `PublicSystemIncludePaths` for `Source/ThirdParty/Protobuf/include`.
  - added Win64 static link for `Source/ThirdParty/Protobuf/lib/Win64/libprotobuf-lite.lib`.
- Verified third-party layout now includes:
  - `Source/ThirdParty/Protobuf/include`
  - `Source/ThirdParty/Protobuf/lib/Win64/libprotobuf-lite.lib`
  - `Source/ThirdParty/Protobuf/bin/Win64/protoc.exe`

### Why
- Prepare plugin-side protobuf framework dependencies without adding message schemas yet.
- Keep protobuf runtime deterministic and bundled in-plugin.

### Impact
- `CefProtobuf` module is ready to include protobuf headers and link lite runtime on Win64.
- Future schema/codegen work can be added without changing third-party wiring.

---

## 2026-04-19 11:54

### Changed
- Added `CefProtobuf` core protocol contracts for future websocket/protobuf integrations:
  - `Source/CefProtobuf/Public/Protocol/CefWsEnvelope.h`
  - `Source/CefProtobuf/Public/Protocol/CefWsCodec.h`
- Added envelope/result abstractions:
  - `FCefWsEnvelope`
  - `ECefProtoEncodeResult`
  - `ECefProtoDecodeResult`
- Added codec interface abstraction:
  - `ICefWsCodec` with `Encode`, `Decode`, `GetSchemaVersion`.
- Updated umbrella header:
  - `Source/CefProtobuf/Public/CefProtobuf.h` now includes protocol headers.

### Why
- Establish schema-agnostic plugin contracts before introducing game-specific `.proto` messages.
- Keep `CefProtobuf` reusable for future projects and message sets.

### Impact
- Other plugin modules can depend on `CefProtobuf` contracts immediately.
- Next implementation steps can focus on concrete codec format and protobuf-backed adapter.

---

## 2026-04-19 13:00

### Changed
- Implemented concrete binary websocket codec:
  - `Source/CefProtobuf/Public/Protocol/CefWsBinaryCodec.h`
  - `Source/CefProtobuf/Private/Protocol/CefWsBinaryCodec.cpp`
- Updated umbrella protobuf header to expose binary codec:
  - `Source/CefProtobuf/Public/CefProtobuf.h`
- Added explicit helper namespace for binary read/write internals:
  - `namespace CefWsBinaryIo` in `CefWsBinaryCodec.cpp`.
- Kept protocol classes/contracts in global scope as requested (`FCefWsEnvelope`, `ICefWsCodec`, `FCefWsBinaryCodec`).

### Why
- Provide a reusable, schema-agnostic wire codec for websocket transport.
- Establish deterministic envelope framing before game-specific protobuf schemas are added.

### Impact
- Plugin now has a concrete codec for envelope serialization/deserialization with version/payload validation.
- Future protobuf message support can plug into envelope `Payload` using message-type dispatch.

---

## 2026-04-19 14:22

### Changed
- Added websocket pipeline payload format enum:
  - `ECefWebSocketPayloadFormat` (`Binary`, `Utf8String`, `JsonString`, `XmlString`, `Custom`).
- Added pipeline-oriented data/config fields:
  - `FCefWebSocketServerCreateOptions::InDefaultPayloadFormat`
  - `FCefWebSocketPipelineConfig`
  - stage queue depth stats fields in `FCefWebSocketServerStats`.
- Added pipeline contracts:
  - `Public/Pipeline/CefWebSocketPipelineTypes.h`
  - `Public/Pipeline/ICefWebSocketPacketCodec.h`
- Added default codecs:
  - `Public/Pipeline/CefWebSocketDefaultCodecs.h`
  - `Private/Pipeline/CefWebSocketDefaultCodecs.cpp`

### Why
- Define explicit extension points for developer-controlled encoding/decoding.
- Prepare transport pipeline types before splitting runtime into 4 threads.

### Impact
- Plugin now has codec interfaces and payload-format abstraction for future custom protocols.
- Runtime refactor can consume these contracts without changing public API later.

---

## 2026-04-19 14:22

### Changed
- Added new worker thread classes for planned 4-stage architecture:
  - `Private/Threads/CefWebSocketHandleRunnable.h/.cpp`
  - `Private/Threads/CefWebSocketSendRunnable.h/.cpp`
- Extended `FCefWebSocketServerInstance` thread API with stage entry points:
  - `PumpInboundOnHandleThread()`
  - `PumpOutgoingOnSendThread()`
- Added temporary no-op implementations for the new stage pumps to keep build flow stable before full queue wiring.

### Why
- Introduce dedicated execution lanes for Handle and Send stages.
- Prepare incremental migration without breaking existing runtime path.

### Impact
- Thread primitives for 4-stage pipeline now exist.
- Next task will wire these stages into server instance queues and lifecycle.

---

## 2026-04-19 14:26

### Changed
- Refactored `FCefWebSocketServerInstance` into a 4-stage pipeline runtime:
  - Read stage: backend tick + socket callbacks enqueue inbound packets.
  - Handle stage: `PumpInboundOnHandleThread()` dequeues inbound packets, runs codec decode, dispatches to `NotifyClientMessage`.
  - Send stage: `PumpOutgoingOnSendThread()` dequeues logical send requests, encodes via codec, produces write packets.
  - Write stage: `PumpOutgoingOnWriteThread()` drains per-client write queues and sends bytes to sockets.
- Added full 4-thread lifecycle management in start/stop:
  - read/handle/send/write runnable creation and shutdown synchronization.
  - wake events for handle/send/write queues.
- Added codec integration in server instance:
  - built-in codec selection by payload format.
  - optional custom codec override path.
- Added queue depth accounting for stage visibility and stats fields updates.

### Why
- Implement requested architecture split to decouple IO, packet handling, encoding, and writes.
- Enable future protocol extensibility with codec-based routing.

### Impact
- Server instance no longer handles inbound packets directly in read callback path.
- Outbound APIs now enqueue logical send requests and are encoded on dedicated send thread.
- Runtime now has explicit per-stage wake/scheduling behavior for better control and scaling.

---

## 2026-04-19 14:29

### Changed
- Exposed developer control hooks on `UCefWebSocketServerBase`:
  - `SetPayloadFormat(...)`
  - `SetPipelineConfig(...)`
  - `SetPacketCodec(...)`
- Extended internal server startup contract to pass pipeline controls:
  - `StartServerInternal(..., InPayloadFormat, InPipelineConfig)`
- Wired subsystem create options into startup:
  - `InOptions.InDefaultPayloadFormat`
  - `InOptions.InPipelineConfig`
- Added create-options field for pipeline config and reordered struct declarations for compile-safe USTRUCT usage.
- Wired pipeline config limits inside `FCefWebSocketServerInstance`:
  - inbound queue max
  - send queue max
  - write queue per-client max
  - payload format default from pipeline config

### Why
- Allow developers to choose format/codec and queue policy without editing engine internals.
- Ensure configuration is applied at server construction/startup boundary.

### Impact
- Server behavior can now be tuned per server instance from create options or runtime setters.
- Pipeline queue/backpressure behavior is now externally configurable.

---

## 2026-04-19 14:30

### Changed
- Updated debug command output to reflect pipeline architecture:
  - `ws.list` now prints payload format and stage queue depths.
  - `ws.stats` now prints payload format plus inbound/handle/send/write queue depths.
- Added `GetPayloadFormat()` accessor to server public API (`UCefWebSocketServerBase`).
- Updated `Source/CefWebSocketServer/README.md`:
  - documented 4-thread pipeline model,
  - documented payload format and codec control APIs,
  - documented create-options pipeline fields,
  - documented stage queue stats visibility.

### Why
- Make new architecture observable and easy to operate for plugin consumers.
- Ensure future developers understand where to customize encoding/handling flow.

### Impact
- Runtime diagnostics now expose per-stage pressure points.
- Documentation now matches current architecture and extension points.

---
## 2026-04-19 14:36

### Changed
- Removed redundant startup payload format source in websocket server API.
- Deleted FCefWebSocketServerCreateOptions::InDefaultPayloadFormat.
- Simplified startup contract to StartServerInternal(..., const FCefWebSocketPipelineConfig&).
- Updated subsystem startup callsite to pass only pipeline config.
- Updated README create-options docs and C++ sample to use InPipelineConfig.InPayloadFormat.

### Why
- PayloadFormat existed in two startup inputs (InDefaultPayloadFormat and InPipelineConfig.InPayloadFormat), causing ambiguity.
- Keep one canonical configuration path to avoid mismatch and confusion.

### Impact
- Server startup now derives payload format only from FCefWebSocketPipelineConfig.
- Public create options are clearer and less error-prone for future developers.

---
## 2026-04-19 18:31

### Changed
- Added CefDispatch core type-erased dispatch value contracts:
  - ICefDispatchValue
  - FCefDispatchTypeId
  - TCefDispatchValue<T>
  - CefDispatchTryGetValue<T>(...)
- Added uint32 -> factory registry:
  - FCefDispatchRegistry
  - RegisterFactory(...), UnregisterFactory(...), Decode(...)
  - dispatch decode result enum ECefDispatchFactoryResult.
- Updated FCefDispatchModule to own a shared registry singleton (GetRegistry()) and expose Get()/IsAvailable() helpers.

### Why
- Create minimal abstraction where route factories can return any payload type (protobuf, custom struct, string) via type erasure.
- Keep dispatch core independent from protobuf and transport modules.

### Impact
- Plugin now has a reusable runtime dispatch core keyed by MessageType.
- Downstream modules can register factories and resolve typed payloads safely.

---
## 2026-04-19 18:32

### Changed
- Added semi-auto dispatch registration support:
  - FCefDispatchFactoryRegistrar
  - CEF_DISPATCH_REGISTER_FACTORY(...)
  - CEF_DISPATCH_REGISTER_FACTORY_REPLACE(...)
- Added deferred registration queue in module bootstrap:
  - registrations made during static init are buffered,
  - flushed into FCefDispatchRegistry during StartupModule().
- Added FCefDispatchModule::RegisterDeferredFactory(...) for registrar path.

### Why
- Allow future developers to register message factories in local .cpp files without central manual wiring.
- Keep registration safe even when static objects run before module startup.

### Impact
- MessageType -> Factory routes can now be self-registered with macros.
- Existing registry API remains compatible with manual registration.

---
## 2026-04-19 18:33

### Changed
- Added dispatch value utility helpers:
  - mutable CefDispatchTryGetValue<T>(...)
  - MakeCefDispatchValue(...) creator helper.
- Added Source/CefDispatch/README.md with:
  - manual factory registration example,
  - semi-auto macro registration example,
  - typed decode/read example,
  - protobuf integration note.

### Why
- Reduce boilerplate for route factories and typed payload extraction.
- Document how CefDispatch supports protobuf and non-protobuf payloads with the same API.

### Impact
- Developers can register and consume typed dispatch values faster.
- Module usage is now documented for future teams.

---
## 2026-04-19 19:01

### Changed
- Subtask 1/14: added connection health controls for websocket runtime.
- Added CVars:
  - cefws.heartbeat_interval_sec
  - cefws.idle_timeout_sec
- Added per-client activity/heartbeat timestamps in FCefWebSocketServerInstance client state.
- Read thread now runs SweepConnectionHealthOnReadThread() after backend tick:
  - periodic lightweight heartbeat payload send,
  - idle timeout disconnect for inactive clients.

### Why
- Detect stale local websocket clients and keep connections observable/alive during low traffic.

### Impact
- Server can automatically close idle clients and periodically emit heartbeat traffic without user code.

---
## 2026-04-19 19:01

### Changed
- Subtask 2/14: added configurable write-queue overflow policy.
- Added CVar cefws.queue_drop_policy:
  -   = drop oldest queued messages (existing behavior)
  - 1 = reject new message when queue is full.
- Updated per-client write queue enqueue logic in FCefWebSocketServerInstance::EnqueueWritePacket(...) to honor policy.

### Why
- Different projects need different backpressure semantics (preserve freshest vs preserve queued data).

### Impact
- Queue overflow behavior is now runtime-configurable without code changes.

---
## 2026-04-19 19:02

### Changed
- Subtask 3/14: implemented adaptive write batching in write stage.
- Added CVars:
  - cefws.write_batch_max_messages
  - cefws.write_batch_max_bytes
- Updated PumpOutgoingOnWriteThread() to drain multiple queued packets per client per pass (bounded by message and byte limits).

### Why
- Reduce write-thread overhead and improve burst throughput for many small packets.

### Impact
- Fewer wake cycles and lock transitions are needed to flush per-client queues.
- Batch size is tunable at runtime.

---
## 2026-04-19 19:04

### Changed
- Subtask 4/14: tuned read-thread idle backoff with runtime controls.
- Added CVars:
  - cefws.read_busy_sleep_ms
  - cefws.read_idle_max_sleep_ms
- Changed read-stage tick API to return activity signal (TickBackendOnReadThread() -> bool).
- Added read-activity flag updates on connect/disconnect/packet callbacks.
- Updated read runnable to use exponential idle backoff between busy/idle sleep bounds.

### Why
- Reduce CPU usage during idle periods while keeping read responsiveness when traffic resumes.

### Impact
- Read thread now adapts sleep duration automatically based on observed activity.

---
## 2026-04-19 19:05

### Changed
- Subtask 5/14: added stricter payload size guardrails.
- Added CVars:
  - cefws.max_text_message_bytes
  - cefws.max_outbound_message_bytes
- Inbound handling now rejects oversized text frames separately from global message limit.
- Outbound queue path now rejects oversized send requests early (ECefWebSocketSendResult::TooLarge).

### Why
- Protect local websocket runtime from large-frame spikes and enforce explicit text/binary limits.

### Impact
- Oversized text inbound or outbound payloads are now blocked before queue pressure escalates.

---
## 2026-04-19 19:06

### Changed
- Subtask 6/14: added graceful shutdown drain behavior.
- Added CVar cefws.shutdown_drain_ms.
- FCefWebSocketServerInstance::Stop() now performs bounded queue-drain loop before stopping worker threads.
- Added queue-drain readiness helper (AreQueuesDrained()).

### Why
- Reduce message loss during intentional server shutdown by allowing in-flight queues to flush first.

### Impact
- Stop path now supports short controlled drain window before full teardown.

---
## 2026-04-19 19:07

### Changed
- Subtask 7/14: optimized broadcast fan-out path.
- Broadcast APIs no longer enumerate target clients on caller thread.
- Send thread now resolves broadcast target list lazily just before encode.

### Why
- Reduce caller-thread lock work and centralize fan-out target resolution in pipeline send stage.

### Impact
- Broadcast requests enqueue faster and use fresher client snapshot at actual send time.

---
## 2026-04-19 19:08

### Changed
- Subtask 8/14: expanded websocket debug command surface.
- Added console commands:
  - ws.clients <name> (lists connected client ids/endpoints/timestamps)
  - ws.cvars (prints key runtime tuning variables)
- Updated debug command startup/shutdown registration for new commands.

### Why
- Improve live diagnostics during development and performance tuning.

### Impact
- Operators can inspect client population and runtime tuning state directly from UE console.

---
## 2026-04-19 19:08

### Changed
- Subtask 9/14: added Unreal Stats/Insights instrumentation for websocket stages.
- Added stage cycle stats:
  - STAT_CefWs_ReadPump
  - STAT_CefWs_HandlePump
  - STAT_CefWs_SendPump
  - STAT_CefWs_WritePump
- Added queue depth counters:
  - inbound/send/write queue depth stats.
- Wired counter updates into pipeline pumps.

### Why
- Improve visibility into stage timing and queue pressure for performance tuning.

### Impact
- Insights/stat captures now expose per-stage execution and queue depth trends.

---
