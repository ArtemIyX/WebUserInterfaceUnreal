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

void FCefNetWebSocket::SetConnectedCallBack(FCefNetWebSocketInfoCallback CallBack)
{
	ConnectedCallBack = CallBack;
}

void FCefNetWebSocket::SetConnectionErrorCallBack(FCefNetWebSocketInfoCallback CallBack)
{
	ErrorCallBack = CallBack;
}

void FCefNetWebSocket::SetReceiveCallBack(FCefNetWebSocketPacketReceivedCallback CallBack)
{
	ReceivedCallback = CallBack;
}

void FCefNetWebSocket::SetSocketClosedCallBack(FCefNetWebSocketInfoCallback CallBack)
{
	SocketClosedCallback = CallBack;
}

bool FCefNetWebSocket::Send(const uint8* Data, uint32 Size, bool bPrependSize)
{
	if (!Data || Size == 0)
	{
		return false;
	}

	TArray<uint8> buffer;
	buffer.AddDefaulted(LWS_PRE);
	buffer.Append(Data, Size);

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

FString FCefNetWebSocket::RemoteEndPoint(bool bAppendPort)
{
	FString remote = RemoteIp;
	if (bAppendPort)
	{
		remote += FString::Printf(TEXT(":%i"), RemotePort);
	}
	return remote;
}

FString FCefNetWebSocket::LocalEndPoint(bool bAppendPort)
{
	const int32 sock = static_cast<int32>(lws_get_socket_fd(Wsi));
	struct sockaddr_in localAddr;
	socklen_t localAddrLen = sizeof(localAddr);
	getsockname(sock, reinterpret_cast<struct sockaddr*>(&localAddr), &localAddrLen);

	ANSICHAR ipBuffer[INET_ADDRSTRLEN];
	FString local(ANSI_TO_TCHAR(inet_ntop(AF_INET, &localAddr.sin_addr, ipBuffer, INET_ADDRSTRLEN)));
	if (bAppendPort)
	{
		local += FString::Printf(TEXT(":%i"), ntohs(localAddr.sin_port));
	}
	return local;
}

void FCefNetWebSocket::OnReceive(void* Data, uint32 Size, bool bIsBinary)
{
	ReceivedCallback.ExecuteIfBound(Data, static_cast<int32>(Size), bIsBinary);
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
