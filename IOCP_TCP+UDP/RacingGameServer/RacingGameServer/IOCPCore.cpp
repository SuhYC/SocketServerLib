#include "IOCPCore.hpp"

IOCPCore::IOCPCore() : m_IsRun(false), m_IOCPHandle(INVALID_HANDLE_VALUE), m_ListenSocket(INVALID_SOCKET), m_UDPRecvBuf({}), m_UDPSocket(INVALID_SOCKET)
{
	WSADATA wsaData;

	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (nRet != 0)
	{
		LOG_ERR("WSAStartup Failed.");
	}
}

IOCPCore::~IOCPCore()
{
	WSACleanup();
}

bool IOCPCore::SendMsgTCP(const uint32_t uClientIndex_, PacketData* pPacket_)
{
	Connection* pConnection = GetConnection(uClientIndex_);

	if (pConnection == nullptr)
	{
		return false;
	}

	return pConnection->SendMsgTCP(pPacket_);
}

bool IOCPCore::SendMsgUDP(const uint32_t uClientIndex_, PacketData* pPacket_)
{
	if (pPacket_ == nullptr)
	{
		return false;
	}

	Connection* pConnection = GetConnection(uClientIndex_);

	if (pConnection == nullptr)
	{
		return false;
	}

	// UDP 송신 CompletionKey는 연결객체가 아니다. UDPSendContext의 포인터를 CK로 넘기자.

	// 락 안씀.
	// 임계영역이긴 하나 중요한 데이터를 송신할 것도 아니고, 누락되면 안되는 데이터도 아니다.
	sockaddr_in& clntaddr = pConnection->GetUDPIP();

	stUDPSendContext* pSendContext = m_UDPSendContextPool.Allocate();

	if (pSendContext == nullptr)
	{
		LOG_ERR("UDPSendctx Allocate Failed.");
		return false;
	}

	pSendContext->Init(uClientIndex_, clntaddr, pPacket_);

	// UDP IP 매핑 안된 상태
	if (pSendContext->m_overlapped.clientAddr.sin_port == 0)
	{
		LOG_DEBUG("IP Not Mapped.");

		m_UDPSendContextPool.Deallocate(pSendContext);
		return false;
	}

	int nRet = WSASendTo(m_UDPSocket, pSendContext->m_overlapped.m_wsaBuf, 1, NULL, 0, (SOCKADDR*)&pSendContext->m_overlapped.clientAddr,
		pSendContext->m_overlapped.addrlen, &pSendContext->m_overlapped.m_overlapped, NULL);

	if (nRet == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		// 어쨋든 송신실패했으니 바로 반환한다.
		m_UDPSendContextPool.Deallocate(pSendContext);

		return false;
	}

	return true;
}

bool IOCPCore::Start(uint16_t uBindPort_, uint16_t uUdpPort_, uint32_t uMaxClient_)
{
	m_IsRun = true;

	bool bRet = InitIOCP(uBindPort_);

	if (!bRet)
	{
		return false;
	}

	bRet = InitUDP(uUdpPort_);

	if (!bRet)
	{
		return false;
	}

	bRet = CreateConnections(uMaxClient_);

	if (!bRet)
	{
		return false;
	}

	bRet = BindUDPRecv();

	if (!bRet)
	{
		return false;
	}

	bRet = CreateWorkerThreads();

	if (!bRet)
	{
		return false;
	}

	LOG_DEBUG("서버 시작...");

	return true;
}

bool IOCPCore::End()
{
	m_IsRun = false;

	PostEndToWorkerThreads();

	bool bRet = DestroyWorkerThreads();

	ReleaseConnections();

	CloseHandle(m_IOCPHandle);

	if (!bRet)
	{
		return false;
	}

	return true;
}

bool IOCPCore::GetReqMsg(uint32_t uClientIndex_, std::string_view& sv_)
{
	Connection* pConnection = GetConnection(uClientIndex_);

	if (pConnection == nullptr)
	{
		LOG_ERR("Invalid Connection. Conn NO. : ", uClientIndex_);
		return false;
	}

	return pConnection->GetReqMessage(sv_);
}

bool IOCPCore::PopReqMsg(uint32_t uClientIndex_, std::string_view& sv_)
{
	Connection* pConnection = GetConnection(uClientIndex_);

	return pConnection->PopRecvBuffer(sv_.size());
}

uint64_t IOCPCore::GetUDPToken(uint32_t uClientIndex_)
{
	Connection* pConnection = GetConnection(uClientIndex_);

	if (pConnection == nullptr)
	{
		LOG_ERR("연결객체 매핑 오류 : ", uClientIndex_);
		return 0;
	}

	uint64_t uRet = pConnection->GetUDPToken();

	if (uRet != 0) // 기존에 토큰 있었음
	{
		return uRet;
	}

	clock_t timestamp = clock();

	uRet = ((uint64_t)uClientIndex_ << 32) | (uint64_t)timestamp;

	SpinLockGuard guard(m_UDPTokenLock);

	pConnection->SetUDPToken(uRet);
	m_UDPTokens.emplace(uRet, uClientIndex_); // map<uint64_t, uint32_t>

	return uRet;
}

uint64_t IOCPCore::RenewUDPToken(uint32_t uClientIndex_)
{
	clock_t timestamp = clock();

	uint64_t uRet = ((uint64_t)uClientIndex_ << 32) | (uint64_t)timestamp;

	Connection* pConnection = GetConnection(uClientIndex_);

	if (pConnection == nullptr)
	{
		LOG_ERR("연결객체 매핑 오류 : ", uClientIndex_);
		return 0;
	}

	SpinLockGuard guard(m_UDPTokenLock);

	uint64_t oldToken = pConnection->SetUDPToken(uRet);

	if (oldToken != 0)
	{
		m_UDPTokens.erase(oldToken);
	}

	m_UDPTokens.emplace(uRet, uClientIndex_); // map<uint64_t, uint32_t>

	return uRet;
}

void IOCPCore::SetJobProcess(uint32_t uClientIndex_, JobProcess eVal_)
{
	Connection* pConnection = GetConnection(uClientIndex_);

	if (pConnection == nullptr)
	{
		return;
	}

	pConnection->SetJobProcess(eVal_);

	return;
}

JobProcess IOCPCore::GetJobProcess(uint32_t uClientIndex_)
{
	Connection* pConnection = GetConnection(uClientIndex_);

	if (pConnection == nullptr)
	{
		LOG_ERR("Connection Object Not Found.");
		return JobProcess::PARSING;
	}

	return pConnection->GetJobProcess();

}

Job* IOCPCore::SetNewJob(uint32_t uClientIndex_, Job* pJob_)
{
	Connection* pConnection = GetConnection(uClientIndex_);

	if (pConnection == nullptr)
	{
		return nullptr;
	}

	return pConnection->SetNewJob(pJob_);
}

void IOCPCore::ClearUDPToken(uint32_t uClientIndex_)
{
	Connection* pConnection = GetConnection(uClientIndex_);

	if (pConnection == nullptr)
	{
		LOG_ERR("연결객체 매핑 오류 : ", uClientIndex_);
		return;
	}

	SpinLockGuard guard(m_UDPTokenLock);

	uint64_t token = pConnection->SetUDPToken(0);

	if (token != 0)
	{
		m_UDPTokens.erase(token);
	}

	return;
}

bool IOCPCore::InitIOCP(uint16_t uBindPort_)
{
	m_ListenSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (m_ListenSocket == INVALID_SOCKET)
	{
		return false;
	}

	SOCKADDR_IN stServerAddr{};
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(uBindPort_);
	stServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	int nRet = bind(m_ListenSocket, reinterpret_cast<SOCKADDR*>(&stServerAddr), sizeof(SOCKADDR_IN));
	if (nRet != 0)
	{
		return false;
	}

	nRet = listen(m_ListenSocket, 5);
	if (nRet != 0)
	{
		return false;
	}

	m_IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, MAX_WORKTHREAD);

	if (m_IOCPHandle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	auto hIOCPHandle = CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_ListenSocket), m_IOCPHandle, 0, 0);

	if (m_IOCPHandle != hIOCPHandle)
	{
		return false;
	}

	return true;
}

bool IOCPCore::InitUDP(uint16_t uUdpPort_)
{
	m_UDPSocket = WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (m_UDPSocket == INVALID_SOCKET)
	{
		return false;
	}

	SOCKADDR_IN stServerAddr{};
	stServerAddr.sin_family = AF_INET;
	stServerAddr.sin_port = htons(uUdpPort_);
	stServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	int nRet = bind(m_UDPSocket, reinterpret_cast<SOCKADDR*>(&stServerAddr), sizeof(SOCKADDR_IN));
	if (nRet == SOCKET_ERROR)
	{
		return false;
	}

	auto hIOCPHandle = CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_UDPSocket), m_IOCPHandle, 0, 0);

	if (m_IOCPHandle != hIOCPHandle)
	{
		return false;
	}

	return true;
}

bool IOCPCore::CreateWorkerThreads()
{
	try
	{
		for (int i = 0; i < MAX_WORKTHREAD; i++)
		{
			m_WorkerThreads.emplace_back([this]() {WorkerThread(); });
		}
	}
	catch (const std::system_error&)
	{
		return false;
	}
	DEFAULT_CATCH()

		return true;
}

void IOCPCore::PostEndToWorkerThreads()
{
	m_IsRun = false;

	for (int i = 0; i < m_WorkerThreads.size(); i++)
	{
		PostQueuedCompletionStatus(m_IOCPHandle, 0, 0, nullptr);
	}

	return;
}

bool IOCPCore::DestroyWorkerThreads()
{
	m_IsRun = false;

	try
	{
		for (int i = 0; i < m_WorkerThreads.size(); i++)
		{
			std::thread& t = m_WorkerThreads[i];

			if (t.joinable())
			{
				t.join();
			}
		}
	}
	catch (const std::system_error&)
	{

		return false;
	}
	DEFAULT_CATCH()

		return true;
}

bool IOCPCore::CreateConnections(uint32_t uMaxClient_)
{
	try
	{
		m_Connections.clear();
		m_Connections.resize(uMaxClient_);

		for (uint32_t i = 0; i < uMaxClient_; i++)
		{
			Connection* pConnection = new(std::nothrow) Connection(m_ListenSocket, i);

			if (pConnection == nullptr)
			{
				LOG_ERR("할당 실패");
				break;
			}

			m_Connections[i] = pConnection;
			m_Connections[i]->ResetConnection();
		}
	}
	catch (const std::bad_alloc&)
	{
		LOG_ERR("할당 실패");
		return false;
	}
	DEFAULT_CATCH()

		return true;
}

bool IOCPCore::BindUDPRecv()
{
	ZeroMemory(&m_UDPRecvOverlapped, sizeof(m_UDPRecvOverlapped));
	ZeroMemory(&m_UDPRecvBuf, MAX_SOCKBUF);
	m_UDPRecvOverlapped.m_eOperation = eIOOperation::UDPRECV;
	m_UDPRecvOverlapped.m_wsaBuf[0].buf = m_UDPRecvBuf;
	m_UDPRecvOverlapped.m_wsaBuf[0].len = MAX_UDP_DATAGRAM_SIZE;

	m_UDPRecvOverlapped.flags = 0;

	m_UDPRecvOverlapped.addrlen = sizeof(m_UDPRecvOverlapped.clientAddr);

	int nRet = WSARecvFrom(m_UDPSocket, m_UDPRecvOverlapped.m_wsaBuf, 1, NULL, &m_UDPRecvOverlapped.flags,
		(SOCKADDR*)&m_UDPRecvOverlapped.clientAddr, &m_UDPRecvOverlapped.addrlen, (LPWSAOVERLAPPED)&m_UDPRecvOverlapped, NULL);

	if (nRet == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		LOG_ERR("UDP Recv Bind Failed.");

		return false;
	}

	return true;
}

bool IOCPCore::UDPResend(stUDPSendContext* pOverlapped_)
{
	if (pOverlapped_ == nullptr)
	{
		return false;
	}

	ZeroMemory(pOverlapped_, sizeof(WSAOVERLAPPED));

	int nRet = WSASendTo(m_UDPSocket, pOverlapped_->m_overlapped.m_wsaBuf, 1, NULL, 0, (SOCKADDR*)&pOverlapped_->m_overlapped.addrlen,
		sizeof(sockaddr_in), &pOverlapped_->m_overlapped.m_overlapped, NULL);

	if (nRet == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		// 어쨋든 송신실패했으니 바로 반환한다.
		PacketPool::Instance().Deallocate(pOverlapped_->pPacket);
		m_UDPSendContextPool.Deallocate(pOverlapped_);

		return false;
	}

	return true;
}

void IOCPCore::ReleaseConnections()
{
	for (int i = 0; i < m_Connections.size(); i++)
	{
		Connection* pConnection = m_Connections[i];

		if (pConnection != nullptr)
		{
			delete pConnection;
		}
	}

	return;
}

Connection* IOCPCore::GetConnection(uint32_t uClientIndex_)
{
	if (m_Connections.size() > uClientIndex_)
	{
		return m_Connections[uClientIndex_];
	}

	return nullptr;
}

void IOCPCore::WorkerThread()
{
	constexpr int32_t MAX_EVENTS = 64;
	OVERLAPPED_ENTRY entries[MAX_EVENTS];
	ULONG numEntries = 0;

	Connection* pConnection = nullptr;
	stOverlappedEx* pOverlapped = nullptr;

	while (m_IsRun)
	{
		BOOL result = GetQueuedCompletionStatusEx(
			m_IOCPHandle,
			entries,
			MAX_EVENTS,
			&numEntries,
			INFINITE,
			FALSE);

		if (result)
		{
			for (ULONG i = 0; i < numEntries; i++)
			{
				OVERLAPPED_ENTRY& entry = entries[i];

				pConnection = reinterpret_cast<Connection*>(entry.lpCompletionKey);
				pOverlapped = reinterpret_cast<stOverlappedEx*>(entry.lpOverlapped);
				DWORD iosize = entry.dwNumberOfBytesTransferred;

				if (pOverlapped == nullptr)
				{
					// PQCS 함수로 종료요청을 보낸 경우임
					std::thread::id tid = std::this_thread::get_id();
					size_t id_no = std::hash<std::thread::id>{}(tid);
					LOG_DEBUG("ThreadID[", id_no, "] terminated.");
					break;
				}

				if (iosize == 0 && pOverlapped->m_eOperation != eIOOperation::ACCEPT)
				{
					// 정상 연결 해제.
					DoDisconnect(pConnection->GetIndex(), true);

					continue;
				}

				
				// 예외처리 내가봐도 어지럽다.. 언젠가 리팩토링 해야할듯..
				if (entry.Internal != 0)
				{
					switch (entry.Internal)
					{
					case STATUS_CANCELLED:
						// 연결 해제 등으로 I/O가 캔슬됨
						if (pOverlapped->m_eOperation == eIOOperation::UDPSEND)
						{
							// 자원 정리 목적.
							DoUDPSend(reinterpret_cast<stUDPSendContext*>(pOverlapped));
						}
						continue;
					case ERROR_NETNAME_DELETED:
					case ERROR_CONNECTION_ABORTED:
						// 연결 해제해도 됨
						DoDisconnect(pConnection->GetIndex(), true);
						if (pOverlapped->m_eOperation == eIOOperation::UDPSEND)
						{
							// 자원 정리 목적.
							DoUDPSend(reinterpret_cast<stUDPSendContext*>(pOverlapped));
						}
						break;
					case ERROR_MORE_DATA:
						LOG_ERR("Not Enough Buffer On IO : ", static_cast<int32_t>(pOverlapped->m_eOperation));
						if (pOverlapped->m_eOperation == eIOOperation::UDPSEND)
						{
							// 자원 정리 목적.
							DoUDPSend(reinterpret_cast<stUDPSendContext*>(pOverlapped));
						}
						
						continue;
					case ERROR_NOT_ENOUGH_MEMORY:
						// 커널에서 메모리부족으로 실패. 재요청 필요
					case WSAEWOULDBLOCK:
						// 드라이버 문제나 네트워크 스택 문제. 재요청 필요
					case WSAEINTR:
						// 시스템 인터럽트로 인한 실패. 재요청 필요
						if (!m_IsRun)
						{
							return;
						}

						switch (pOverlapped->m_eOperation)
						{
						case eIOOperation::TCPSEND:
							pConnection->ReSendTCP();
							break;
						case eIOOperation::UDPSEND:
							UDPResend(reinterpret_cast<stUDPSendContext*>(pOverlapped));
							break;
						case eIOOperation::TCPRECV:
							pConnection->BindRecv();
							break;
						case eIOOperation::UDPRECV:
							// -- !!!!! ---
							// 아직 서버 가동중이니 다시 바인드해야함.
							BindUDPRecv();

							break;
						case eIOOperation::ACCEPT:
							DoDisconnect(pConnection->GetIndex());
							break;
						default:
							LOG_ERR("Undefined IO Operation : ", static_cast<int32_t>(pOverlapped->m_eOperation));
							break;
						}
						break;
					default:
						// 뭔진 모르겠으나 로그 찍고 연결해제해
						LOG_ERR(entry.Internal);
						LOG_ERR("Socket[", pConnection->GetIndex(), "] : 알 수 없는 에러. CODE", entry.Internal);
						
						DoDisconnect(pConnection->GetIndex());
						break;
					}
				}

				switch (pOverlapped->m_eOperation)
				{
				case eIOOperation::ACCEPT:
					DoAccept(pOverlapped);
					break;
				case eIOOperation::TCPRECV:
					DoTCPRecv(pOverlapped, iosize);
					break;
				case eIOOperation::UDPRECV:
					DoUDPRecv(pOverlapped, iosize);
					break;
				case eIOOperation::TCPSEND:
					DoTCPSend(pOverlapped, iosize);
					break;
				case eIOOperation::UDPSEND:
					DoUDPSend(reinterpret_cast<stUDPSendContext*>(pOverlapped));
					break;
				default:
					// io enum 예상 못한게 나왔는데?
					LOG_ERR("Invalid IOOperation Enum. ENUM VALUE : ", static_cast<int32_t>(pOverlapped->m_eOperation));
					break;
				}
			}
		}
	}

	return;
}

void IOCPCore::DoDisconnect(uint32_t uClientIndex_, bool bBroadcast_)
{
	Connection* pConnection = GetConnection(uClientIndex_);

	if (pConnection == nullptr)
	{
		LOG_ERR("연결객체 매핑 오류");
		return;
	}

	ClearUDPToken(uClientIndex_);
	pConnection->Close();
	pConnection->ResetConnection();

	if (bBroadcast_)
	{
		OnDisconnect(uClientIndex_);
	}

	return;
}

void IOCPCore::DoAccept(const stOverlappedEx* const pOverlapped_)
{
	Connection* pConnection = GetConnection(pOverlapped_->m_userIndex);
	bool bRet = pConnection->BindIOCP(m_IOCPHandle);

	if (!bRet)
	{
		DoDisconnect(pConnection->GetIndex());
		return;
	}

	bRet = pConnection->BindRecv();

	if (!bRet)
	{
		DoDisconnect(pConnection->GetIndex());
		return;
	}

	sockaddr_in addr{};
	bRet = pConnection->SetSocketOpt(addr);

	if (!bRet)
	{
		// ip 추출 실패
	}

	OnConnect(pOverlapped_->m_userIndex, addr);

	return;
}

void IOCPCore::DoTCPRecv(const stOverlappedEx* const pOverlapped_, const DWORD ioSize_)
{
	Connection* pConnection = GetConnection(pOverlapped_->m_userIndex);

	LOG_DEBUG("Recved ", ioSize_, "bytes.");

	if (pConnection == nullptr)
	{
		LOG_ERR("Connection Missing");
		return;
	}

	pConnection->StorePartialMessage(ioSize_);

	OnTCPReceive(pOverlapped_->m_userIndex);

	bool bRet = pConnection->BindRecv();

	if (!bRet)
	{
		DoDisconnect(pConnection->GetIndex(), true);
		LOG_ERR("Failed to Reserve Recv");
		return;
	}

	return;
}

void IOCPCore::DoUDPRecv(const stOverlappedEx* const pOverlapped_, const DWORD ioSize_)
{
	LOG_DEBUG("[", pOverlapped_->m_userIndex, "] ", ioSize_ , "bytes Recved.");

	m_UDPRecvBuf[ioSize_] = NULL;

	// 토큰 확인
	UDPHeader header;
	CopyMemory(&header, m_UDPRecvBuf, sizeof(header));

	uint32_t uClientIndex = 0;

	//LOG_DEBUG("port : ", pOverlapped_->clientAddr.sin_port);
	
	{
		SpinLockGuard guard(m_UDPTokenLock);

		// 토큰 확인되지 않으면 그냥 드랍
		auto itr = m_UDPTokens.find(header.token);
		if (itr == m_UDPTokens.end())
		{
			LOG_DEBUG("Re.token : ", header.token);

			BindUDPRecv();
			return;
		}

		uClientIndex = itr->second;
	}
	
	// 토큰에 맞는 커넥션인덱스 가져와서 주소갱신하고 OnUDPReceive에 넘김
	Connection* pConnection = GetConnection(uClientIndex);
	if (pConnection == nullptr)
	{
		LOG_ERR("연결객체 매핑 오류");
		BindUDPRecv();
		return;
	}

	pConnection->SetUDPIP(pOverlapped_->clientAddr);
	std::string_view msg(m_UDPRecvBuf, ioSize_);
	OnUDPReceive(uClientIndex, msg);
	
	// 다시 UDPRecv 바인드하고 종료
	
	BindUDPRecv();

	return;
}

void IOCPCore::DoTCPSend(const stOverlappedEx* const pOverlapped_, DWORD ioSize_)
{
	Connection* pConnection = GetConnection(pOverlapped_->m_userIndex);

	if (pConnection == nullptr)
	{
		return;
	}

	pConnection->TCPSendCompleted(ioSize_);
	return;
}

void IOCPCore::DoUDPSend(stUDPSendContext* pOverlapped_)
{
	// 리소스만 반환하고 종료

	PacketPool::Instance().Deallocate(pOverlapped_->pPacket);

	m_UDPSendContextPool.Deallocate(pOverlapped_);

	return;
}