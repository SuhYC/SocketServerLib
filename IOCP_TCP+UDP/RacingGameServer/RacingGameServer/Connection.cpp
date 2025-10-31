#include "Connection.hpp"

Connection::Connection(const SOCKET listenSocket_, const int index_)
	: m_ListenSocket(listenSocket_), m_ClientIndex(index_), m_AcceptBuf({}), m_RecvBuf({}),
	m_IsConnected(false), m_UDPAddr({}), m_RecvOverlapped({}), m_eJobProcess(JobProcess::PARSING), m_UDPToken(0)
{
	if (!m_RecvBuffer.Init(MAX_RECV_SLIDEBUFFER_SIZE))
	{
		LOG_ERR("[", index_, "] Client Failed to Allocate RecvSlideBuffer.");
	}
	
	if (!m_TCPSendBuffer.Allocate(MAX_SEND_SLIDEBUFFER_SIZE))
	{
		LOG_ERR("[", index_, "] Client Failed to Allocate SendSlideBuffer.");
	}

}

Connection::~Connection()
{
	if (m_TcpSocket != INVALID_SOCKET)
	{
		closesocket(m_TcpSocket);
	}
}

void Connection::Init()
{
	m_TcpSocket = INVALID_SOCKET;
	m_UDPAddr.sin_port = 0; // 연결된 udp주소 없음

	ZeroMemory(&m_RecvOverlapped, sizeof(m_RecvOverlapped));
	ZeroMemory(m_AcceptBuf, 64);
	ZeroMemory(m_RecvBuf, MAX_SOCKBUF);

	ZeroMemory(&m_UDPAddr, sizeof(m_UDPAddr));

	m_RecvOverlapped.m_userIndex = m_ClientIndex;
	m_IsConnected = false;
	m_RecvBuffer.Clear();

	if (m_TcpSocket != INVALID_SOCKET)
	{
		closesocket(m_TcpSocket);
	}

	m_TcpSocket = INVALID_SOCKET;

	return;
}

void Connection::ResetConnection()
{
	Init();
	BindAcceptEx();
}

bool Connection::BindIOCP(const HANDLE hWorkIOCP_)
{
	auto hIOCP = CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_TcpSocket),
		hWorkIOCP_,
		(ULONG_PTR)(this),
		0);

	if (hIOCP == INVALID_HANDLE_VALUE || hIOCP != hWorkIOCP_)
	{
		return false;
	}

	m_IsConnected = true;

	return true;
}

bool Connection::BindRecv()
{
	if (!m_IsConnected)
	{
		return false;
	}

	m_RecvOverlapped.m_eOperation = eIOOperation::TCPRECV;
	m_RecvOverlapped.m_wsaBuf[0].len = MAX_SOCKBUF;
	m_RecvOverlapped.m_wsaBuf[0].buf = m_RecvBuf;

	ZeroMemory(&m_RecvOverlapped.m_overlapped, sizeof(WSAOVERLAPPED));

	m_RecvOverlapped.flags = 0;

	auto result = WSARecv(
		m_TcpSocket,
		&m_RecvOverlapped.m_wsaBuf[0],
		1, // 수신은 하나만 씀.
		NULL,
		&m_RecvOverlapped.flags,
		&m_RecvOverlapped.m_overlapped,
		NULL
	);

	if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		return false;
	}

	return true;
}

bool Connection::SendMsgTCP(PacketData* pData_)
{
	if (pData_ == nullptr || !m_IsConnected)
	{
		return false;
	}

	SpinLockGuard guard(m_TCPSpinLock);

	if (!m_TCPSendBuffer.Push(pData_))
	{
		return false;
	}

	if (m_TCPSendOverlapped.m_wsaBuf[0].len == 0) // 전송중 확인
	{
		return SendTCPIO();
	}

	return true;
}

Job* Connection::SetNewJob(Job* pJob_)
{
	Job* pRet = m_pJob;
	m_pJob = pJob_;

	return pRet;
}

bool Connection::SendTCPIO()
{
	ZeroMemory(&m_TCPSendOverlapped, sizeof(stOverlappedEx));
	
	uint32_t bufs = m_TCPSendBuffer.GetWSABUF(m_TCPSendOverlapped.m_wsaBuf, MAX_WSABUFS);
	m_TCPSendOverlapped.m_eOperation = eIOOperation::TCPSEND;
	m_TCPSendOverlapped.m_userIndex = m_ClientIndex;

	int result = WSASend(m_TcpSocket,
		m_TCPSendOverlapped.m_wsaBuf,
		bufs,
		NULL,
		0,
		(LPWSAOVERLAPPED) & (m_TCPSendOverlapped),
		NULL);

	if (result == SOCKET_ERROR && WSAGetLastError() != ERROR_IO_PENDING)
	{
		return false;
	}

	LOG_DEBUG("TCP Msg Sent To Client[", m_ClientIndex, "]");

	return true;
}

bool Connection::ReSendTCP()
{
	SpinLockGuard guard(m_TCPSpinLock);

	return SendTCPIO();
}

void Connection::TCPSendCompleted(uint32_t ioSize_)
{
	SpinLockGuard guard(m_TCPSpinLock);

	m_TCPSendBuffer.Pop(ioSize_);
	m_TCPSendOverlapped.m_wsaBuf[0].len = 0;

	if (!m_TCPSendBuffer.IsEmpty())
	{
		SendTCPIO();
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

	shutdown(m_TcpSocket, SD_BOTH);
	setsockopt(m_TcpSocket, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

	closesocket(m_TcpSocket);
	m_TcpSocket = INVALID_SOCKET;
	m_UDPAddr.sin_port = 0;

	return;
}

void Connection::SetUDPIP(const sockaddr_in& udpAddr)
{
	CopyMemory(&m_UDPAddr, &udpAddr, sizeof(sockaddr_in));

	return;
}

bool Connection::StorePartialMessage(uint32_t size_)
{
	SpinLockGuard guard(m_RecvLock);

	return m_RecvBuffer.Enqueue(m_RecvBuf, size_);
}

bool Connection::GetReqMessage(std::string_view& out_)
{
	SpinLockGuard guard(m_RecvLock);

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

bool Connection::PopRecvBuffer(uint32_t ioSize_)
{
	SpinLockGuard guard(m_RecvLock);

	return m_RecvBuffer.Pop(ioSize_);
}

uint16_t Connection::GetIndex()
{
	return m_ClientIndex;
}

bool Connection::SetSocketOpt(sockaddr_in& out_)
{
	int bufSize = MAX_SOCKBUF;
	int nRet = setsockopt(m_TcpSocket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&bufSize), sizeof(bufSize));

	if (nRet == SOCKET_ERROR)
	{
		LOG_ERR("소켓버퍼 크기 지정 실패.");
		// WSAGetLastError 확인
		return false;
	}

	int noDelay = 1;
	nRet = setsockopt(m_TcpSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));

	if (nRet == SOCKET_ERROR)
	{
		LOG_ERR("Nagle 옵션 해제 실패.");
		// WSAGetLastError 확인
		return false;
	}

	nRet = setsockopt(m_TcpSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&m_ListenSocket, (int)sizeof(SOCKET));

	if (nRet == SOCKET_ERROR)
	{
		LOG_ERR("TCP소켓과 리스닝소켓 연결 실패.");
		// WSAGetLastError 확인
		return false;
	}

	
	sockaddr_in* localAddrPtr = nullptr;
	sockaddr_in* remoteAddrPtr = nullptr;
	int localLen, remoteLen;

	GetAcceptExSockaddrs(
		m_AcceptBuf,
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		(sockaddr**)&localAddrPtr,
		&localLen,
		(sockaddr**)&remoteAddrPtr,
		&remoteLen
	);

	// remoteAddrPtr이 null이 아니면 구조체 복사
	if (remoteAddrPtr)
	{
		out_ = *remoteAddrPtr;
	}

	return true;
}

sockaddr_in& Connection::GetUDPIP()
{
	return m_UDPAddr;
}

uint64_t Connection::GetUDPToken()
{
	return m_UDPToken;
}

uint64_t Connection::SetUDPToken(uint64_t uNewToken_)
{
	uint64_t ret = m_UDPToken;
	m_UDPToken = uNewToken_;

	return ret;
}

void Connection::SetJobProcess(JobProcess eVal_)
{
	m_eJobProcess = eVal_;
	return;
}

JobProcess Connection::GetJobProcess()
{
	return m_eJobProcess;
}

bool Connection::BindAcceptEx()
{
	ZeroMemory(&m_RecvOverlapped.m_overlapped, sizeof(WSAOVERLAPPED));

	m_TcpSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (m_TcpSocket == INVALID_SOCKET)
	{
		return false;
	}

	DWORD bytes = 0;
	DWORD flags = 0;
	m_RecvOverlapped.m_wsaBuf[0].len = 0;
	m_RecvOverlapped.m_wsaBuf[0].buf = nullptr;
	m_RecvOverlapped.m_eOperation = eIOOperation::ACCEPT;

	auto result = AcceptEx(
		m_ListenSocket,
		m_TcpSocket,
		m_AcceptBuf,
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