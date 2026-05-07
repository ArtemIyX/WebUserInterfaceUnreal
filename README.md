# CefWebUi Plugin

CEF-powered Web UI + local WebSocket stack for Unreal Engine, with an external `Host.exe` renderer process.
<img width="1280" height="720" alt="blueprints" src="https://github.com/user-attachments/assets/4343cb57-61c4-46dc-b007-44e4aad9597a" />


---

## 1) What This Plugin Includes

This plugin is split into 5 runtime modules:

- `CefWebUi`
  - Browser session API, shared-memory IPC, input/control writers, frame reader, Slate browser surface.
- `CefWebSocketServer`
  - Local websocket server subsystem with threaded pipeline and pluggable payload codecs.
- `CefProtobuf`
  - Binary websocket envelope/codec helpers intended for protobuf-based payloads.
- `CefDispatch`
  - Format-agnostic `MessageType(uint32) -> Factory` registry (works with protobuf or any custom type).
- `CefContentHttpServer`
  - Local HTTP image endpoint (`/img`) with pluggable request handler strategy.

---

## 2) External Dependency: CEF Host

This plugin **requires** the external CEF host package (renderer process + CEF runtime files).

Download host package from:

- [CEF_HOST](https://github.com/ArtemIyX/CefHost)

After download, place host files into:

- `Plugins/CefWebUi/Source/ThirdParty/Cef`

Expected key files in that folder:

- `Host.exe`
- `libcef.dll`
- CEF resources (`*.pak`, `*.dat`, `*.bin`, `locales/`, etc.)

If `Host.exe` is missing or misplaced, runtime startup will fail.

---

## 3) How It Works (High Level)

1. UE starts `Host.exe`.
2. Host renders Chromium off-screen.
3. Host publishes frame metadata + shared GPU textures through IPC/shared resources.
4. UE reads frame metadata and copies shared textures into its render path.
5. UE forwards mouse/keyboard and control commands (URL, JS, resize, etc.) back to host.

Data/control path:

- Frame stream: Host -> UE
- Input/control stream: UE -> Host
- Optional console stream: Host -> UE logs/events

---

## 4) Install and Enable

1. Copy plugin into your project:
   - `YourProject/Plugins/CefWebUi`
2. Put host package into:
   - `YourProject/Plugins/CefWebUi/Source/ThirdParty/Cef`
3. Enable plugin in ``.uproject``:
```json
"Plugins": [
		{
			"Name": "CefWebUi",
			"Enabled": true
		}
	]
```
4. Generate project files
5. Build project

---

## 5) Minimal Blueprint Quick Start

Use Blueprint function library:

- `GetCefWebUiSubsystem`
- `GetOrCreateBrowserSession`
- `ShowBrowserSessionInViewport`

Minimal flow:

1. `ShowBrowserSessionInViewport`
   - `SessionId = "Default"` (or your own name)
   - `BrowserWidth`, `BrowserHeight`
2. Get returned `UCefWebUiBrowserSession`.
3. Call `SetUrl("https://your-page")`.
4. Optional controls:
   - `ExecuteJs`
   - `Reload`
   - `Resize`
   - `SetFocus`

If you have a blueprint screenshot for onboarding, add it here:

```md
![Minimal Blueprint Setup](Resources/Docs/blueprint_quick_start.png)
```

---

## 6) C++ Quick Start

```cpp
#include "Subsystems/CefWebUiGameInstanceSubsystem.h"
#include "Sessions/CefWebUiBrowserSession.h"

void UMyGameInstance::InitWebUi()
{
    UCefWebUiGameInstanceSubsystem* subsystem = GetSubsystem<UCefWebUiGameInstanceSubsystem>();
    if (!subsystem)
    {
        return;
    }

    UCefWebUiBrowserSession* session = subsystem->GetOrCreateSession(FName(TEXT("MainUi")), nullptr);
    if (!session)
    {
        return;
    }

    session->ShowInViewport(/*PlayerController*/ nullptr, /*ZOrder*/ 10, /*Width*/ 1920, /*Height*/ 1080);
    session->SetUrl(TEXT("https://example.com"));
    session->SetFocus(true);
}
```

Control examples:

```cpp
session->ExecuteJs(TEXT("console.log('hello from ue');"));
session->Resize(1600, 900);
session->Reload();
session->OpenDevTools();
```

---

## 7) Why `CefProtobuf` Module Exists

`CefProtobuf` gives a ready binary envelope/codec layer for websocket payloads where:

- payloads are binary, not plain JSON text,
- message routing is done with typed envelope metadata (for example message type ids),
- protobuf serialization/deserialization is desired.

Use it when you want compact, schema-based transport between UE and browser/backend.

---

## 8) Why `CefDispatch` Module Exists

`CefDispatch` solves routing/creation of typed runtime objects from message ids:

- Register factory per `uint32 MessageType`.
- Decode inbound bytes by message type.
- Return any value type (`protobuf object`, custom struct, `FString`, raw wrapper, etc.).

This keeps transport and business object creation decoupled.

---

## 9) Protobuf + Dispatch Integration Pattern

Typical pattern for binary protocol:

1. Websocket codec (`CefProtobuf`) extracts envelope:
   - `MessageType`, payload bytes.
2. Dispatch registry (`CefDispatch`) uses `MessageType` factory:
   - parse payload into desired object.
3. Application consumes typed value.

This supports both:

- fully protobuf messages
- mixed payload ecosystem (protobuf + custom binary/text)

---

## 10) WebSocket Server Quick Start

`CefWebSocketServer` provides local server lifecycle through subsystem.

Basic usage:

1. Get `UCefWebSocketSubsystem`.
2. Build `FCefWebSocketServerCreateOptions` (`NameId`, `RequestedPort`, pipeline config).
3. Call `CreateOrGetServer(...)`.
4. Bind events:
   - `OnClientConnected`
   - `OnClientDisconnected`
   - `OnServerError`
   - `OnClientError`
5. Send:
   - `SendToClientString/Bytes`
   - `BroadcastString/Bytes`

Custom transport logic:

- Implement `ICefWebSocketPacketCodec` for your format.
- Switch payload mode to `Custom`.

---

## 11) Content HTTP Server Quick Start

`CefContentHttpServer` is started/stopped by `UCefContentHttpServerSubsystem` lifecycle.

Default:

- Port: `18080`
- Route: `GET /img`
- Default handler:
  1. Resolve `asset` from query/body.
  2. Load texture via module image cache.
  3. Encode texture to PNG.
  4. Return `image/png`.

Example request:

```text
http://localhost:18080/img?asset=/Game/Folder/T_Image
```

If your file is:

```text
Content/Folder/T_Image.upackage
```

then request path should be:

```text
/Game/Folder/T_Image
```

The module normalizes to object path internally (`/Game/Folder/T_Image.T_Image`).

Body alternatives:

- JSON body: `{"asset":"/Game/Folder/T_Image"}`
- Raw body: `asset=/Game/Folder/T_Image`

### Override Request Handling

Create custom handler:

```cpp
#include "Handlers/CefContentHttpImageRequestHandler.h"
#include "MyCustomImageHandler.generated.h"

UCLASS()
class UMyCustomImageHandler : public UCefContentHttpImageRequestHandler
{
    GENERATED_BODY()

public:
    virtual bool HandleImageRequest_Implementation(
        const FCefContentHttpImageRequestContext& InRequestContext,
        FCefContentHttpImageResponse& OutResponse,
        FString& OutError) override
    {
        // custom logic...
        OutResponse.StatusCode = 200;
        OutResponse.ContentType = TEXT("application/json");
        const FString payload = TEXT("{\"ok\":true}");
        FTCHARToUTF8 utf8(*payload);
        OutResponse.Body.Append(reinterpret_cast<const uint8*>(utf8.Get()), utf8.Length());
        return true;
    }
};
```

Assign handler class through subsystem:

```cpp
#include "Subsystems/CefContentHttpServerSubsystem.h"

void UMyGameInstance::InitHttpHandler()
{
    if (UCefContentHttpServerSubsystem* subsystem = GetSubsystem<UCefContentHttpServerSubsystem>())
    {
        subsystem->SetRequestHandlerClass(UMyCustomImageHandler::StaticClass(), true);
    }
}
```

---

## 12) Runtime Settings (Developer Settings)

Project Settings includes `Cef Web UI` section:

- `bShowHostConsole`
  - Opens host process with visible console window.
- `HostAdditionalArgs`
  - Extra command-line args passed to `Host.exe`.

Use these for diagnostics and host runtime tuning.

---

## 13) Common Troubleshooting

### Host not starting

- Verify `Host.exe` exists at:
  - `Plugins/CefWebUi/Source/ThirdParty/Cef/Host.exe`
- Verify CEF runtime files are beside it.

### Black/empty browser

- Confirm host package version matches plugin IPC expectations.
- Check UE logs for shared memory/handle open failures.
- Ensure URL loading actually happens (`SetUrl`, `LoadHtmlString`).

### Input not working

- Ensure session is visible and focused (`SetFocus(true)`).
- Verify no overlay widget is swallowing mouse/keyboard input.
- Verify resize/input mapping flow if you changed viewport sizing behavior.

### WebSocket payload decoding issues

- Ensure payload format and codec match on both ends.
- If using protobuf, validate envelope/message type mapping.

---

## 14) Suggested First Integration Steps

1. Run minimal browser session in viewport.
2. Load local simple HTML first (then remote URL).
3. Bind `OnFinishedLoading` and JS console messages.
4. Add websocket server only after browser path is stable.
5. Add protobuf envelope/dispatch routing last.

### Notes

- Do not edit `ThirdParty` headers as part of plugin logic changes.
- Keep host package in `Source/ThirdParty/Cef` consistent across machines.
- If IPC structs/events are changed, UE and host must be updated together.

---

## License

Project is licensed under [MIT](LICENSE)
