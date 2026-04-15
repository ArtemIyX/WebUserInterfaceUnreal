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
