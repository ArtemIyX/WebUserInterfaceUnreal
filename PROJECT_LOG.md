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
