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
	int Sock = lws_get_socket_fd(Wsi);
	socklen_t Len = sizeof(RemoteAddr);
	FMemory::Memzero(&RemoteAddr, sizeof(RemoteAddr));
	getpeername(Sock, (struct sockaddr*)&RemoteAddr, &Len);
}

FCefNetWebSocket::~FCefNetWebSocket()
{
	Flush();
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

	TArray<uint8> Buffer;
	Buffer.AddDefaulted(LWS_PRE);
	Buffer.Append(Data, Size);

	{
		FScopeLock Lock(&OutgoingLock);
		OutgoingBuffer.Add(MoveTemp(Buffer));
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
	if (!Wsi)
	{
		return;
	}

	for (;;)
	{
		int32 Count = 0;
		{
			FScopeLock Lock(&OutgoingLock);
			Count = OutgoingBuffer.Num();
		}
		if (Count == 0)
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
	ANSICHAR Buffer[INET_ADDRSTRLEN];
	FString Remote(ANSI_TO_TCHAR(inet_ntop(AF_INET, &RemoteAddr.sin_addr, Buffer, INET_ADDRSTRLEN)));
	if (bAppendPort)
	{
		Remote += FString::Printf(TEXT(":%i"), ntohs(RemoteAddr.sin_port));
	}
	return Remote;
}

FString FCefNetWebSocket::LocalEndPoint(bool bAppendPort)
{
	int Sock = lws_get_socket_fd(Wsi);
	struct sockaddr_in Addr;
	socklen_t Len = sizeof(Addr);
	getsockname(Sock, (struct sockaddr*)&Addr, &Len);

	ANSICHAR Buffer[INET_ADDRSTRLEN];
	FString Local(ANSI_TO_TCHAR(inet_ntop(AF_INET, &Addr.sin_addr, Buffer, INET_ADDRSTRLEN)));
	if (bAppendPort)
	{
		Local += FString::Printf(TEXT(":%i"), ntohs(Addr.sin_port));
	}
	return Local;
}

void FCefNetWebSocket::OnReceive(void* Data, uint32 Size, bool bIsBinary)
{
	ReceivedCallback.ExecuteIfBound(Data, static_cast<int32>(Size), bIsBinary);
}

void FCefNetWebSocket::OnRawWebSocketWritable(CefWebSocketInternal* InWsi)
{
	TArray<uint8> Packet;
	{
		FScopeLock Lock(&OutgoingLock);
		if (OutgoingBuffer.Num() == 0)
		{
			return;
		}
		Packet = MoveTemp(OutgoingBuffer[0]);
		OutgoingBuffer.RemoveAt(0);
	}

	uint32 DataToSend = Packet.Num() - LWS_PRE;
	uint32 Offset = 0;
	while (DataToSend > 0)
	{
		const int Sent = lws_write(InWsi, Packet.GetData() + LWS_PRE + Offset, DataToSend, LWS_WRITE_BINARY);
		if (Sent < 0)
		{
			ErrorCallBack.ExecuteIfBound();
			return;
		}
		Offset += static_cast<uint32>(Sent);
		DataToSend -= static_cast<uint32>(Sent);
	}
}

void FCefNetWebSocket::OnClose()
{
	SocketClosedCallback.ExecuteIfBound();
}
