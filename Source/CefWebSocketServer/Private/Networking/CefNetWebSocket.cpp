#include "Networking/CefNetWebSocket.h"

#include "Logging/CefWebSocketLog.h"

#define UI UI_ST
THIRD_PARTY_INCLUDES_START
#include "libwebsockets.h"
THIRD_PARTY_INCLUDES_END
#undef UI

FCefNetWebSocket::FCefNetWebSocket(CefWebSocketInternalContext* InContext, CefWebSocketInternal* InWsi)
	: Context(InContext)
	, Wsi(InWsi)
{
	const int32 sock = static_cast<int32>(lws_get_socket_fd(Wsi));
	struct sockaddr_in remoteAddr;
	FMemory::Memzero(&remoteAddr, sizeof(remoteAddr));
	socklen_t remoteAddrLen = sizeof(remoteAddr);
	getpeername(sock, reinterpret_cast<struct sockaddr*>(&remoteAddr), &remoteAddrLen);

	ANSICHAR ipBuffer[INET_ADDRSTRLEN];
	RemoteIp = ANSI_TO_TCHAR(inet_ntop(AF_INET, &remoteAddr.sin_addr, ipBuffer, INET_ADDRSTRLEN));
	RemotePort = static_cast<int32>(ntohs(remoteAddr.sin_port));
}

FCefNetWebSocket::~FCefNetWebSocket()
{
	FlushInternal();
	ReceivedCallback.Unbind();
}

void FCefNetWebSocket::SetConnectedCallBack(FCefNetWebSocketInfoCallback InCallBack)
{
	ConnectedCallBack = InCallBack;
}

void FCefNetWebSocket::SetConnectionErrorCallBack(FCefNetWebSocketInfoCallback InCallBack)
{
	ErrorCallBack = InCallBack;
}

void FCefNetWebSocket::SetReceiveCallBack(FCefNetWebSocketPacketReceivedCallback InCallBack)
{
	ReceivedCallback = InCallBack;
}

void FCefNetWebSocket::SetSocketClosedCallBack(FCefNetWebSocketInfoCallback InCallBack)
{
	SocketClosedCallback = InCallBack;
}

bool FCefNetWebSocket::Send(const uint8* InData, uint32 InSize, bool bInPrependSize)
{
	if (!InData || InSize == 0)
	{
		return false;
	}

	TArray<uint8> buffer;
	buffer.AddDefaulted(LWS_PRE);
	buffer.Append(InData, InSize);

	{
		FScopeLock lock(&OutgoingLock);
		OutgoingBuffer.Add(MoveTemp(buffer));
	}

	if (Wsi)
	{
		lws_callback_on_writable(Wsi);
	}

	return true;
}

void FCefNetWebSocket::Tick()
{
}

void FCefNetWebSocket::Flush()
{
	FlushInternal();
}

void FCefNetWebSocket::FlushInternal()
{
	if (!Wsi)
	{
		return;
	}

	for (;;)
	{
		int32 messageCount = 0;
		{
			FScopeLock lock(&OutgoingLock);
			messageCount = OutgoingBuffer.Num();
		}
		if (messageCount == 0)
		{
			break;
		}
		lws_callback_on_writable(Wsi);
		break;
	}
}

void FCefNetWebSocket::Close()
{
	if (!Wsi)
	{
		return;
	}

	lws_close_reason(Wsi, LWS_CLOSE_STATUS_NORMAL, nullptr, 0);
	lws_callback_on_writable(Wsi);
}

FString FCefNetWebSocket::RemoteEndPoint(bool bInAppendPort)
{
	FString remote = RemoteIp;
	if (bInAppendPort)
	{
		remote += FString::Printf(TEXT(":%i"), RemotePort);
	}
	return remote;
}

FString FCefNetWebSocket::LocalEndPoint(bool bInAppendPort)
{
	const int32 sock = static_cast<int32>(lws_get_socket_fd(Wsi));
	struct sockaddr_in localAddr;
	socklen_t localAddrLen = sizeof(localAddr);
	getsockname(sock, reinterpret_cast<struct sockaddr*>(&localAddr), &localAddrLen);

	ANSICHAR ipBuffer[INET_ADDRSTRLEN];
	FString local(ANSI_TO_TCHAR(inet_ntop(AF_INET, &localAddr.sin_addr, ipBuffer, INET_ADDRSTRLEN)));
	if (bInAppendPort)
	{
		local += FString::Printf(TEXT(":%i"), ntohs(localAddr.sin_port));
	}
	return local;
}

void FCefNetWebSocket::OnReceive(void* InData, uint32 InSize, bool bInIsBinary)
{
	ReceivedCallback.ExecuteIfBound(InData, static_cast<int32>(InSize), bInIsBinary);
}

void FCefNetWebSocket::OnRawWebSocketWritable(CefWebSocketInternal* InWsi)
{
	TArray<uint8> packet;
	{
		FScopeLock lock(&OutgoingLock);
		if (OutgoingBuffer.Num() == 0)
		{
			return;
		}
		packet = MoveTemp(OutgoingBuffer[0]);
		OutgoingBuffer.RemoveAt(0);
	}

	uint32 bytesToSend = packet.Num() - LWS_PRE;
	uint32 sentOffset = 0;
	while (bytesToSend > 0)
	{
		const int32 sent = static_cast<int32>(lws_write(InWsi, packet.GetData() + LWS_PRE + sentOffset, bytesToSend, LWS_WRITE_BINARY));
		if (sent < 0)
		{
			ErrorCallBack.ExecuteIfBound();
			return;
		}
		sentOffset += static_cast<uint32>(sent);
		bytesToSend -= static_cast<uint32>(sent);
	}
}

void FCefNetWebSocket::OnClose()
{
	SocketClosedCallback.ExecuteIfBound();
}
