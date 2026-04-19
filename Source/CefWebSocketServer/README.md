# CefWebSocketServer Usage

## Overview
`CefWebSocketServer` provides a `UGameInstanceSubsystem` for named local websocket servers.

- One server UObject per `NameId`
- Multiple clients per server
- 4-stage pipeline threads:
  - Read: websocket socket read/tick
  - Handle: inbound decode + handler dispatch
  - Send: outbound request encode/routing
  - Write: socket write flush

## Blueprint Quick Start
1. Get `CefWebSocketSubsystem` from `GameInstance`.
2. Build `FCefWebSocketServerCreateOptions`:
   - `NameId`
   - `RequestedPort`
   - `InPipelineConfig`
3. Call `CreateOrGetServer(Options, ServerClass, ClientClass, OutServer)`.
4. Bind server events:
   - `OnClientConnected`
   - `OnClientDisconnected`
   - `OnServerError`
   - `OnClientError`
5. Send:
   - `SendToClientString`
   - `SendToClientBytes`
   - `BroadcastString`
   - `BroadcastBytes`
   - `BroadcastStringExcept`
   - `BroadcastBytesExcept`
6. Stop with `StopServer(NameId)` or `StopAllServers()`.

## C++ Quick Start
```cpp
UCefWebSocketSubsystem* wsSubsystem = GetGameInstance()->GetSubsystem<UCefWebSocketSubsystem>();
if (!wsSubsystem)
{
    return;
}

FCefWebSocketServerCreateOptions options;
options.NameId = FName(TEXT("UiBridge"));
options.RequestedPort = 7001;
options.InPipelineConfig.InPayloadFormat = ECefWebSocketPayloadFormat::JsonString;
options.InPipelineConfig.InInboundQueueMax = 4096;
options.InPipelineConfig.InSendQueueMax = 4096;

UCefWebSocketServerBase* server = nullptr;
const FCefWebSocketServerCreateResult createResult = wsSubsystem->CreateOrGetServer(
    options,
    UCefWebSocketServerBase::StaticClass(),
    UCefWebSocketClientBase::StaticClass(),
    server);

if (createResult.Result == ECefWebSocketCreateResult::Failed || !server)
{
    return;
}

server->BroadcastString(TEXT("server-online"));
```

## Developer Control
`UCefWebSocketServerBase` runtime controls:
- `SetPayloadFormat(ECefWebSocketPayloadFormat)`
- `SetPipelineConfig(const FCefWebSocketPipelineConfig&)`
- `SetPacketCodec(const TSharedPtr<ICefWebSocketPacketCodec>&)` (C++ only)

Built-in payload formats:
- `Binary`
- `Utf8String`
- `JsonString`
- `XmlString`
- `Custom` (use custom codec)

Custom codec contract:
- Implement `ICefWebSocketPacketCodec`
- `DecodeInbound(...)`
- `EncodeSendRequest(...)`

## Subclassing
- Derive server from `UCefWebSocketServerBase`:
  - override `HandleClientBytes(...)`
  - override `HandleClientString(...)`
- Derive client from `UCefWebSocketClientBase`:
  - override `HandleBytesFromClient(...)`
  - override `HandleStringFromClient(...)`

## JSON Sending (C++)
Use server methods:
- `SendToClientJson(...)`
- `BroadcastJson(...)`

## Debug Console Commands
- `ws.list`
- `ws.stop <name>`
- `ws.kick <server> <clientId>`
- `ws.stats <name>`

`ws.list` and `ws.stats` include stage queue depth fields:
- inbound
- handle
- send
- write

## CVars
- `cefws.max_message_bytes`
- `cefws.max_queue_messages_per_client`
- `cefws.max_queue_bytes_per_client`
- `cefws.write_idle_sleep_ms`
- `cefws.shutdown_timeout_ms`
- `cefws.max_port_scan`
- `cefws.validate_utf8`
- `cefws.log_traffic`

## Notes
- Servers are persisted by `UGameInstanceSubsystem` lifetime.
- `CreateOrGetServer` returns existing server for same `NameId`.
- Port may auto-adjust when requested port is unavailable.
