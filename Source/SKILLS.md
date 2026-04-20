# SKILLS.md - UE5 CefWebUi Plugin Knowledge Base

**Project Name:** CefWebUi (Unreal Engine 5 Plugin)
**Purpose:** High-performance embedded Chromium browser using CEF in Off-Screen Rendering (OSR) mode with **zero-copy GPU texture sharing** from a separate CEF Host.exe process.

This plugin communicates with the external CEF Host (C++ D3D11) via:
- Named shared D3D11 textures (`Global\CEFHost_SharedTex_0/1`)
- Shared memory + events for frame metadata, input, and control

---

## 1. Overall Architecture

- **CEF Host.exe** (external process, D3D11):
  - Creates D3D11 device + shared textures (double buffered, named NT handles)
  - Renders CEF content using `CefRenderHandler::OnAcceleratedPaint`
  - Composites popups on the GPU
  - Writes frame metadata to `CEFHost_Frame` shared memory
  - Listens to input/control via ring buffers + events

- **UE5 Plugin Side** (this code):
  - `FCefFrameReader`: Reads frame header + signals from shared memory on a background thread
  - `FCefInputWriter`: Sends mouse/keyboard events to CEF Host via lock-free ring buffer
  - `FCefControlWriter`: Sends high-level commands (URL, resize, JS, devtools, etc.)
  - `UCefWebUiBrowserWidget`: UUserWidget that displays the browser, handles Slate input, and performs GPU copy from shared texture → `UTextureRenderTarget2D`

**Key Principle:**  
**Zero CPU pixel copying.** Everything stays on the GPU. UE5 opens the shared D3D11 textures via `OpenSharedHandleByName` and copies them into a render target using `RHICopyTexture`.

---

## 2. Core Classes & Responsibilities

### FCefFrameReader (CefFrameReader.cpp)
- Opens `CEFHost_Frame` shared memory and `CEFHost_FrameReady` event
- Runs on its own `FRunnableThread`
- Waits on event → checks `sequence` counter to detect new frames
- Copies header data (`width`, `height`, `write_slot`, `cursor_type`, `load_state`) under lock
- Broadcasts `OnLoadStateChanged` on Game Thread when load state changes
- Provides `PollSharedTexture()` for the widget to consume pending frames

**Important:** Does **not** open the texture itself — only metadata.

### FCefInputWriter (CefInputWriter.cpp)
- Mirror of host’s `InputRingBuffer` (256 slots)
- Lock-free ring buffer with `std::atomic` + release/acquire ordering
- Public API: `WriteMouseMove`, `WriteMouseButton`, `WriteMouseScroll`, `WriteKey`, `WriteChar`
- Drops events silently if ring is full

### FCefControlWriter (CefControlWriter.cpp)
- Mirror of host’s `ControlRingBuffer` (64 slots)
- Supports string payloads up to 2048 char16_t
- Rich API: `SetURL`, `ExecuteJS`, `Resize`, `SetZoomLevel`, `OpenDevTools`, `ClearCookies`, etc.
- Uses `FScopeLock` around write

### UCefWebUiBrowserWidget (CefWebUiBrowserWidget.cpp)
**Main user-facing class** (UUserWidget)

**Key responsibilities:**
- Manages `UTextureRenderTarget2D` for display
- Opens two shared D3D11 textures by **name** on Render Thread (`EnsureSharedRHI`)
- In `NativeTick` → `PollAndUpload()`:
  - Polls frame from `FrameReader`
  - Updates cursor via `SetCursor`
  - Ensures render target size
  - Enqueues `RHICopyTexture` from shared texture (selected by `write_slot`) to render target
- Forwards all Slate input events (`NativeOnMouseMove`, `NativeOnMouseButtonDown/Up`, `NativeOnKey*`, etc.) to `InputWriter`
- Blueprint-exposed functions (`BP_*`) for control (GoBack, SetURL, ExecuteJS, etc.)
- `GetBrowserCoords()`: converts Slate local coords → CEF browser pixel coords (with clamping)

**Rendering Path:**
Shared D3D11 Texture (Host) → D3D12 shared resource (via `OpenSharedHandleByName`) → `RHICreateTexture2DFromResource` → `CopyTexture` → RenderTarget → UImage

---

## 3. Shared Memory & IPC Contract (Must Stay Synchronized)

All structures **must** match exactly with the CEF Host side (`SharedMemoryLayout.h`):

- `FCefFrameHeader` (packed, matches `FrameHeader`)
- `FCefInputEvent` / `FInputRingBuffer`
- `FCefControlEvent` / `FControlRingBuffer`
- Event names and shared memory names are hardcoded and identical on both sides
- Texture names: `Global\CEFHost_SharedTex_0` and `_1`
- Frame format: **B8G8R8A8_UNORM** only

**Never change layout without updating both sides.**

---

## 4. UE5-Specific Rules & Patterns

- All Windows API calls are wrapped with `AllowWindowsPlatformTypes` / `HideWindowsPlatformTypes`
- D3D12 RHI is used (`ID3D12DynamicRHI`, `OpenSharedHandleByName`)
- Texture opening and releasing happens on **Render Thread** via `ENQUEUE_RENDER_COMMAND`
- Heavy operations (copy) are enqueued with `SCOPE_CYCLE_STAT`
- Use `FScopeLock` for any shared data between threads
- `AsyncTask(ENamedThreads::GameThread, ...)` for any Blueprint delegate broadcasts
- Widget uses `NativeTick` for polling (not event-driven for frame upload)
- Input is sent immediately on Slate events (no buffering on UE side except the ring)

---

## 5. Important Constants & Limits

- Max resolution: 3840×2160 (same as host)
- Input ring: 256 events
- Control ring: 64 events
- Control string max: 2048 char16_t
- Double-buffered shared textures (index via `write_slot & 1`)

---

## 6. Coding Style & Best Practices (Strict)

- Keep all shared-memory structs **#pragma pack(push,1)** and identical to host
- Use `UE_LOG(LogCefWebUi, ...)` for all logging
- Error handling: log with `Error` or `Warning`, never crash silently
- Always check `.IsValid()` / `.IsOpen()` before using writers/readers
- Blueprint functions prefixed with `BP_` (e.g. `BP_SetURL`)
- Use `FMath::Clamp` and proper rounding in coordinate conversion
- Release resources safely on `NativeDestruct` using `ENQUEUE_RENDER_COMMAND`
- Modifiers in key events are simplified (only Left Ctrl is currently mapped)

---

## 7. Module Integration

- `FCefWebUiModule` (not shown but referenced) owns:
  - `TSharedPtr<FCefFrameReader>`
  - `TSharedPtr<FCefInputWriter>`
  - `TSharedPtr<FCefControlWriter>`
- Widget gets weak pointers to them via module

- Host executable path is provided by `UCefWebUiBPLibrary::GetHostExePath()`

---

## 8. Common Tasks & Gotchas

**Adding new control command:**
1. Add to `ECefControlEventType` enum (Data/CefControlEventType.h)
2. Add field to union in `FCefControlEvent`
3. Implement writer function in `CefControlWriter.cpp`
4. Add `BP_` wrapper in widget
5. Mirror change in CEF Host `PumpControl()`

**Changing resolution:**
- Call `BP_Resize()` → control writer → host resizes and recreates textures
- Widget auto-adapts via `EnsureRenderTarget()`

**Debugging:**
- Check `LogCefWebUi` logs
- Verify shared handles open successfully
- Watch `sequence` counter and `write_slot`
- Ensure Host.exe is running before widget constructs

**Performance:**
- GPU copy happens every new frame
- Input thread on host side uses spin + wait
- Keep `NativeTick` lightweight

---

## 9. CefContentHttpServer Module

This module serves UE assets over HTTP (currently image-focused) and is integrated through a `UGameInstanceSubsystem`.

### Core Runtime Pieces

- `FCefContentHttpServerModule`
  - Owns module-singleton services:
    - `FCefContentImageCacheService`
    - `FCefContentImageEncodeService`
    - `FCefContentHttpServerRuntimeService`
  - Exposes:
    - `GetImageCacher()`
    - `GetImageEncoder()`
    - `GetHttpServerService()`

- `UCefContentHttpServerSubsystem`
  - Starts HTTP server on `Initialize`
  - Stops HTTP server on `Deinitialize`
  - Default port: `18080`
  - Main APIs:
    - `StartServer(int32 InPortOverride = 0)`
    - `StopServer()`
    - `IsServerRunning()`
    - `SetRequestHandlerClass(...)`
    - `GetImageByPackagePath(...)`
    - `ClearCachedImages()`
    - `GetCachedImageCount()`

- `FCefContentHttpServerRuntimeService`
  - Binds route: `GET /img`
  - Parses asset path from:
    - Query param: `asset` (also accepts `aasset`)
    - JSON body: `{ "asset": "..." }` (also supports `"asset="` key)
    - Raw body fallback: `asset=...`
  - Delegates request to handler UObject

- `UCefContentHttpImageRequestHandler`
  - Pluggable request strategy (BlueprintNativeEvent)
  - Contract:
    - input: `FCefContentHttpImageRequestContext`
    - output: `FCefContentHttpImageResponse`

- `UCefContentDefaultImageRequestHandler`
  - Default behavior:
    1. Resolve `asset` path from request
    2. Load texture via module cache service
    3. Encode texture to PNG via module encode service
    4. Return `200 image/png` bytes

### Route Contract

- Endpoint: `http://localhost:<port>/img`
- Query usage:
  - `/img?asset=/Game/Textures/T_Logo.T_Logo`
- Body usage:
  - JSON: `{ "asset": "/Game/Textures/T_Logo.T_Logo" }`
  - Form-like: `asset=/Game/Textures/T_Logo.T_Logo`

### Logging and Stats

- Log category: `LogCefContentHttpServer`
- Stats:
  - `STATGROUP_CefContentHttpServer`
  - `STAT_CefContentHttpServer_CachedImages`

### Build Dependencies (CefContentHttpServer)

- Private:
  - `HTTPServer`
  - `ImageWrapper`
  - `Json`

### Extension Guidance

- For custom request behavior, create a class derived from `UCefContentHttpImageRequestHandler`.
- Register handler class via subsystem `SetRequestHandlerClass(...)`.
- Keep module-owned services as singletons; do not transfer ownership to subsystem callers.

You are now an expert on the **UE5 side** of this CEF integration.

When the user asks you to modify, fix, extend, or add features to this UE5 plugin:
- Always keep perfect sync with the CEF Host side (shared memory layout, event names, texture names, B8G8R8A8 format, double-buffering logic)
- Respect UE5 threading rules (Game Thread vs Render Thread)
- Prefer minimal, clean code with proper RHI enqueuing
- Maintain zero-copy philosophy
