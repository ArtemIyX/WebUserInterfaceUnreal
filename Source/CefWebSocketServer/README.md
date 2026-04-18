# CefWebSocketServer Usage

## Overview
`CefWebSocketServer` provides a `UGameInstanceSubsystem` for named local websocket servers.

- One server UObject per `NameId`
- Multiple clients per server
- Read thread for inbound processing
- Write thread with per-client outbound queue

## Blueprint Quick Start
1. Get `CefWebSocketSubsystem` from `GameInstance`.
2. Build `FCefWebSocketServerCreateOptions`:
   - `NameId`: unique server id (`FName`)
   - `RequestedPort`: desired port (`0` or `<=0` starts from default auto port)
3. Call `CreateOrGetServer(Options, ServerClass, ClientClass, OutServer)`.
4. Bind events on server:
   - `OnClientConnected`
   - `OnClientDisconnected`
   - `OnServerError`
   - `OnClientError`
5. Send data:
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

## Subclassing
- Derive custom server class from `UCefWebSocketServerBase`.
  - Override `HandleClientBytes(...)`.
  - Override `HandleClientString(...)`.
- Derive custom client class from `UCefWebSocketClientBase`.
  - Override `HandleBytesFromClient(...)`.
  - Override `HandleStringFromClient(...)`.

`NotifyClientMessage` path is read-thread driven. BP lifecycle/error delegates are marshaled to game thread.

## JSON Sending (C++)
Use server C++ methods:
- `SendToClientJson(int64, const TSharedPtr<FJsonObject>&, const TSharedPtr<FJsonValue>&)`
- `BroadcastJson(const TSharedPtr<FJsonObject>&, const TSharedPtr<FJsonValue>&)`

Serialization is condensed JSON. Return value is `ECefWebSocketSendResult`.

## Debug Console Commands
- `ws.list`
- `ws.stop <name>`
- `ws.kick <server> <clientId>`
- `ws.stats <name>`

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
