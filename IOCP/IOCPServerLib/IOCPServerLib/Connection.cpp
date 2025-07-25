#pragma once

#include "Connection.hpp"

Connection::Connection(const SOCKET listenSocket_, const int index) 
	: m_ListenSocket(listenSocket_), m_ClientIndex(index), m_ClientSocket(INVALID_SOCKET),
	mAcceptBuf{}, mRecvBuf{}, m_RecvOverlapped{}, m_SendOverlapped{}
{
	m_IsConnected = false;

	m_SendBuffer.Init(MAX_SEND_SLIDEBUFFER_SIZE);
	m_RecvBuffer.Init(MAX_RECV_SLIDEBUFFER_SIZE);
}

Connection::~Connection()
{
	if (m_ClientSocket != INVALID_SOCKET)
	{
		closesocket(m_ClientSocket);
	}
}

void Connection::Init()
{
	ZeroMemory(mAcceptBuf, 64);
	ZeroMemory(mRecvBuf, MAX_SOCKBUF);
	ZeroMemory(&m_RecvOverlapped, sizeof(stOverlappedEx));

	m_RecvOverlapped.m_userIndex = m_ClientIndex;

	m_IsConnected.store(false);

	m_SendBuffer.Clear();
	m_RecvBuffer.Clear();

	if (m_ClientSocket != INVALID_SOCKET)
	{
		closesocket(m_ClientSocket);
	}

	m_ClientSocket = INVALID_SOCKET;
}

void Connection::ResetConnection()
{
	Init();
	BindAcceptEx();

	return;
}

bool Connection::BindIOCP(const HANDLE hWorkIOCP_)
{
	auto hIOCP = CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_ClientSocket),
		hWorkIOCP_,
		(ULONG_PTR)(this),
		0);

	if (hIOCP == INVALID_HANDLE_VALUE || hIOCP != hWorkIOCP_)
	{
		return false;
	}

	m_IsConnected.store(true);

	return true;
}

bool Connection::BindRecv()
{
	if (m_IsConnected.load() == false)
	{
		return false;
	}

	m_RecvOverlapped.m_eOperation = eIOOperation::RECV;
	m_RecvOverlapped.m_wsaBuf.len = MAX_SOCKBUF;
	m_RecvOverlapped.m_wsaBuf.buf = mRecvBuf;

	ZeroMemory(&m_RecvOverlapped.m_overlapped, sizeof(WSAOVERLAPPED));

	DWORD flag = 0;
	DWORD bytes = 0;

	auto result = WSARecv(
		m_ClientSocket,
		&m_RecvOverlapped.m_wsaBuf,
		1,
		&bytes,
		&flag,
		&m_RecvOverlapped.m_overlapped,
		NULL
	);

	if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		return false;
	}

	return true;
}

bool Connection::SendMsg(PacketData* pData_)
{
	if (pData_ == nullptr)
	{
		return false;
	}

	if (m_IsConnected.load() == false)
	{
		return false;
	}

	SpinLockGuard guard(m_SpinLock);

	if (!m_SendBuffer.Enqueue(pData_->GetData(), pData_->GetSize()))
	{
		return false;
	}

	if (m_SendOverlapped.m_wsaBuf.len == 0) // 전송중인지 확인
	{
		bool bRet = SendIO();
		if (!bRet)
		{
			return false;
		}
	}

	return true;
}

bool Connection::SendIO()
{
	int datasize = m_SendBuffer.GetSize();
	if (datasize > MAX_SOCKBUF)
	{
		datasize = MAX_SOCKBUF;
	}

	ZeroMemory(&m_SendOverlapped, sizeof(stOverlappedEx));

	m_SendOverlapped.m_wsaBuf.len = datasize;
	m_SendOverlapped.m_wsaBuf.buf = m_SendBuffer.GetBuf();
	m_SendOverlapped.m_eOperation = eIOOperation::SEND;
	m_SendOverlapped.m_userIndex = m_ClientIndex;

	DWORD dwRecvNumBytes = 0;

	auto result = WSASend(m_ClientSocket,
		&(m_SendOverlapped.m_wsaBuf),
		1,
		&dwRecvNumBytes,
		0,
		(LPWSAOVERLAPPED) & (m_SendOverlapped),
		NULL);

	if (result == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
	{
		return false;
	}

	LOG_DEBUG(datasize, "bytes msg sent.");

	return true;
}

bool Connection::ReSend()
{
	SpinLockGuard guard(m_SpinLock);

	if (!SendIO())
	{
		return false;
	}

	return true;
}

void Connection::SendCompleted()
{
	SpinLockGuard guard(m_SpinLock);

	m_SendBuffer.Pop(m_SendOverlapped.m_wsaBuf.len);
	m_SendOverlapped.m_wsaBuf.len = 0;

	if (!m_SendBuffer.IsEmpty())
	{
		SendIO();
	}

	return;
}

void Connection::Close(bool bIsForce_)
{
	m_IsConnected = false;

	struct linger stLinger = { 0,0 };

	if (bIsForce_)
	{
		stLinger.l_onoff = 1;
	}

	shutdown(m_ClientSocket, SD_BOTH);
	setsockopt(m_ClientSocket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	closesocket(m_ClientSocket);
	m_ClientSocket = INVALID_SOCKET;

	return;
}

bool Connection::GetIP(uint32_t& out_)
{
	int bufSize = MAX_SOCKBUF;
	int nRet = setsockopt(m_ClientSocket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&bufSize), sizeof(bufSize));

	if (nRet == SOCKET_ERROR)
	{
		// WSAGetLastError 확인
		return false;
	}

	int noDelay = 1;
	nRet = setsockopt(m_ClientSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));
	
	if (nRet == SOCKET_ERROR)
	{
		// WSAGetLastError 확인
		return false;
	}

	setsockopt(m_ClientSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&m_ListenSocket, (int)sizeof(SOCKET));

	if (nRet == SOCKET_ERROR)
	{
		// WSAGetLastError 확인
		return false;
	}

	SOCKADDR* l_addr = nullptr; SOCKADDR* r_addr = nullptr; int l_size = 0, r_size = 0;

	GetAcceptExSockaddrs(mAcceptBuf,
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		&l_addr,
		&l_size,
		&r_addr,
		&r_size
	);

	SOCKADDR_IN address = { 0 };
	int addressSize = sizeof(address);

	nRet = getpeername(m_ClientSocket, (struct sockaddr*)&address, &addressSize);

	if (nRet)
	{
		LOG_ERR("getpeername Failed");
		return false;
	}

	out_ = address.sin_addr.S_un.S_addr;

	return true;
}

bool Connection::StorePartialMessage(char* str_, uint32_t size_)
{
	SpinLockGuard guard(m_SpinLock);

	return m_RecvBuffer.Enqueue(str_, size_);
}

bool Connection::GetReqMessage(std::string_view& out_)
{
	SpinLockGuard guard(m_SpinLock);

	uint32_t len = m_RecvBuffer.Peek();

	if (len > PACKET_SIZE)
	{
		LOG_ERR("Too Big.");
		return false;
	}

	if (len == 0)
	{
		return false;
	}

	if (m_RecvBuffer.GetSize() < len)
	{
		return false;
	}

	out_ = std::string_view(m_RecvBuffer.GetBuf(), len);

	return true;
}

bool Connection::PopRecvBuffer(std::string_view& sv_)
{
	return m_RecvBuffer.Pop(sv_.size());
}

bool Connection::BindAcceptEx()
{
	ZeroMemory(&m_RecvOverlapped.m_overlapped, sizeof(WSAOVERLAPPED));

	m_ClientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (m_ClientSocket == INVALID_SOCKET)
	{
		return false;
	}

	DWORD bytes = 0;
	DWORD flags = 0;
	m_RecvOverlapped.m_wsaBuf.len = 0;
	m_RecvOverlapped.m_wsaBuf.buf = nullptr;
	m_RecvOverlapped.m_eOperation = eIOOperation::ACCEPT;

	auto result = AcceptEx(
		m_ListenSocket,
		m_ClientSocket,
		mAcceptBuf,
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		&bytes,
		reinterpret_cast<LPOVERLAPPED>(&m_RecvOverlapped)
	);

	if (result == FALSE && WSAGetLastError() != WSA_IO_PENDING)
	{
		return false;
	}

	return true;
}