#include "IOCPServer.hpp"

IOCPServer::IOCPServer() : m_IsRun(false), m_IOCPHandle(INVALID_HANDLE_VALUE), m_ListenSocket(INVALID_SOCKET)
{
	WSADATA wsaData;

	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (nRet != 0)
	{
		LOG_ERR("WSAStartup Failed.");
	}
}

IOCPServer::~IOCPServer()
{
	End();

	ReleaseConnections();

	WSACleanup();
}

bool IOCPServer::SendMsg(const uint32_t uClientIndex_, PacketData* pPacket_)
{
	Connection* pConnection = GetConnection(uClientIndex_);

	if (pConnection == nullptr)
	{
		return false;
	}

	return pConnection->SendMsg(pPacket_);
}

bool IOCPServer::Start(uint32_t uBindPort_, uint32_t uMaxClient_)
{
	m_IsRun = true;

	bool bRet = InitIOCP(uBindPort_);

	if (!bRet)
	{
		return false;
	}

	bRet = CreateConnections(uMaxClient_);

	if (!bRet)
	{
		return false;
	}

	bRet = CreateWorkerThreads();

	if (!bRet)
	{
		return false;
	}

	return true;
}

bool IOCPServer::End()
{
	PostEndToWorkerThreads();

	bool bRet = DestroyWorkerThreads();

	CloseHandle(m_IOCPHandle);

	if (!bRet)
	{

		return false;
	}

	return true;
}

bool IOCPServer::GetReqMsg(uint32_t uClientIndex_, std::string_view& sv_)
{
	Connection* pConnection = GetConnection(uClientIndex_);

	if (pConnection == nullptr)
	{
		LOG_ERR("Invalid Connection. Conn NO. : ", uClientIndex_);
		return false;
	}

	return pConnection->GetReqMessage(sv_);
}

bool IOCPServer::PopReqMsg(uint32_t uClientIndex_, std::string_view& sv_)
{
	Connection* pConnection = GetConnection(uClientIndex_);

	return pConnection->PopRecvBuffer(sv_);
}

bool IOCPServer::InitIOCP(uint32_t uBindPort_)
{
	m_ListenSocket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);

	if (m_ListenSocket == INVALID_SOCKET)
	{
		return false;
	}

	SOCKADDR_IN stServerAddr;
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

bool IOCPServer::CreateWorkerThreads()
{
	try
	{
		for (int i = 0; i < MAX_WORKTHREAD; i++)
		{
			m_WorkerThreads.emplace_back([this]() {WorkerThread(); });
		}
	}
	catch (const std::system_error& e)
	{
		return false;
	}
	DEFAULT_CATCH()

	return true;
}

void IOCPServer::PostEndToWorkerThreads()
{
	m_IsRun = false;

	for (int i = 0; i < m_WorkerThreads.size(); i++)
	{
		PostQueuedCompletionStatus(m_IOCPHandle, 0, 0, nullptr);
	}

	return;
}

bool IOCPServer::DestroyWorkerThreads()
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
	catch (const std::system_error& e)
	{

		return false;
	}
	DEFAULT_CATCH()

	return true;
}

bool IOCPServer::CreateConnections(uint32_t uMaxClient_)
{
	try
	{
		m_Connections.clear();
		m_Connections.resize(uMaxClient_);

		for (int i = 0; i < uMaxClient_; i++)
		{
			Connection* pConnection = new Connection(m_ListenSocket, i);

			if (pConnection == nullptr)
			{
				LOG_ERR("할당 실패");
				break;
			}

			m_Connections[i] = pConnection;
			m_Connections[i]->ResetConnection();
		}
	}
	catch (const std::bad_alloc& e)
	{
		return false;
	}
	DEFAULT_CATCH()

	return true;
}

void IOCPServer::ReleaseConnections()
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

Connection* IOCPServer::GetConnection(uint32_t uClientIndex_)
{
	if (m_Connections.size() > uClientIndex_)
	{
		return m_Connections[uClientIndex_];
	}

	return nullptr;
}

void IOCPServer::WorkerThread()
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
					pConnection->ResetConnection();
					OnDisconnect(pConnection->GetIndex());

					continue;
				}

				if (entry.Internal != 0)
				{
					switch (entry.Internal)
					{
					case STATUS_CANCELLED:
						// 연결 해제 등으로 I/O가 캔슬됨
						continue;
					case ERROR_NETNAME_DELETED:
					case ERROR_CONNECTION_ABORTED:
						// 연결 해제해도 됨
						pConnection->Close();
						pConnection->ResetConnection();
						OnDisconnect(pConnection->GetIndex());
						break;
					//case ERROR_MORE_DATA: -- TCP에서는 혼잡제어로 인해 발생하지 않음
					case ERROR_NOT_ENOUGH_MEMORY:
						// 커널에서 메모리부족으로 실패. 재요청 필요
					case WSAEWOULDBLOCK:
						// 드라이버 문제나 네트워크 스택 문제. 재요청 필요
					case WSAEINTR:
						// 시스템 인터럽트로 인한 실패. 재요청 필요
						if (pOverlapped->m_eOperation == eIOOperation::SEND)
						{
							pConnection->ReSend();
						}
						else if (pOverlapped->m_eOperation == eIOOperation::RECV)
						{
							pConnection->BindRecv();
						}
						else if (pOverlapped->m_eOperation == eIOOperation::ACCEPT)
						{
							pConnection->Close();
							pConnection->ResetConnection();
						}
						break;
					default:
						// 뭔진 모르겠으나 로그 찍고 연결해제해
						LOG_ERR(entry.Internal);
						LOG_ERR("Socket[", pConnection->GetIndex(), "] : 알 수 없는 에러. CODE", entry.Internal);
						pConnection->Close();
						pConnection->ResetConnection();
						break;
					}
				}

				switch (pOverlapped->m_eOperation)
				{
				case eIOOperation::ACCEPT:
					DoAccept(pOverlapped);
					break;
				case eIOOperation::RECV:
					DoRecv(pOverlapped, iosize);
					break;
				case eIOOperation::SEND:
					DoSend(pOverlapped);
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

void IOCPServer::DoAccept(const stOverlappedEx* pOverlapped_)
{
	Connection* pConnection = GetConnection(pOverlapped_->m_userIndex);
	bool bRet = pConnection->BindIOCP(m_IOCPHandle);

	if (!bRet)
	{
		pConnection->Close();
		pConnection->ResetConnection();
		return;
	}

	bRet = pConnection->BindRecv();

	if (!bRet)
	{
		pConnection->Close();
		pConnection->ResetConnection();
		return;
	}

	uint32_t ip;
	bRet = pConnection->GetIP(ip);

	if (!bRet)
	{
		// ip 추출 실패
		// ip 파싱이 필요할지는 모르겠어서 아직 ip파싱함수 호출은 안했음. 그냥 4바이트 정수로 받는상태 
	}

	OnConnect(pOverlapped_->m_userIndex, ip);

	return;
}

void IOCPServer::DoRecv(const stOverlappedEx* pOverlapped_, const DWORD ioSize_)
{
	Connection* pConnection = GetConnection(pOverlapped_->m_userIndex);

	LOG_DEBUG("Recved ", ioSize_, "bytes.");

	if (pConnection == nullptr)
	{
		LOG_ERR("Connection Missing");
		return;
	}

	pConnection->StorePartialMessage(pConnection->RecvBuffer(), ioSize_);

	OnReceive(pOverlapped_->m_userIndex);

	bool bRet = pConnection->BindRecv();

	if (!bRet)
	{
		pConnection->Close();
		pConnection->ResetConnection();
		OnDisconnect(pOverlapped_->m_userIndex);
		LOG_ERR("Failed to Reserve Recv");
		return;
	}

	return;
}

void IOCPServer::DoSend(const stOverlappedEx* pOverlapped_)
{
	Connection* pConnection = GetConnection(pOverlapped_->m_userIndex);

	if (pConnection == nullptr)
	{
		return;
	}

	pConnection->SendCompleted();
	return;
}