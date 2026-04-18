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
