#include "EpollServer.hpp"

EpollServer::EpollServer() : m_IsRun(false), epoll_fd(-1), m_ListenSocket(-1)
{

}

EpollServer::~EpollServer()
{
	if (m_IsRun)
	{
		End();
	}
}


bool EpollServer::Start(uint16_t uBindPort_)
{
	m_IsRun = true;

	bool bRet = SocketInit(uBindPort_);

	if (!bRet)
	{
		std::cerr << "EpollServer::Start : SocketInit Failed\n";
		return false;
	}

	bRet = CreateThreads();
	if (!bRet)
	{
		std::cerr << "EpollServer::Start : CreateThreads Failed\n";
		return false;
	}
	
	return true;
}

bool EpollServer::End()
{
	m_IsRun = false;

	ReleaseAllContexts();

	bool bRet = DestoryThreads();
	if (!bRet)
	{
		std::cerr << "EpollServer::End : DestroyThreads Failed\n";
	}

	close(m_ListenSocket);
	close(epoll_fd);
	return true;
}


bool EpollServer::SendMsg(const int fd_, PacketData* pPacket_)
{
	ClientContext* pCC = GetContext(fd_);

	if (pCC == nullptr)
	{
		std::cerr << "EpollServer::SendMsg : ClientContext nullptr.\n";
		return false;
	}

	SendStatus eRet = pCC->SendMsg(pPacket_);

	switch (eRet)
	{
	case SendStatus::PARTIAL_SUCCESS:
	{
		epoll_event& event = pCC->GetEpollEvent();
		event.events = EPOLLIN | EPOLLOUT;
		int nRet = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd_, &event); // 송신도 감지
		if (nRet == -1)
		{
			std::cerr << "EpollServer::SendMsg : Failed to Changer fd Mode.\n";
			return false;
		}
	}
		[[fallthrough]];
	case SendStatus::WHOLE_SUCCESS:
		return true;
	case SendStatus::FAILED:
		return false;
	default:
		std::cerr << "EpollServer::SendMsg : Undefined Status.\n";
		return false;
	}
}

bool EpollServer::SocketInit(uint16_t uBindPort_)
{
	m_ListenSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (m_ListenSocket == -1)
	{
		std::cerr << "EpollServer::SocketInit : Failed to Allcate ListenSocket.\n";
		return false;
	}

	struct sockaddr_in serv_adr {};
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(uBindPort_);

	int nRet = bind(m_ListenSocket, reinterpret_cast<sockaddr*>(&serv_adr), sizeof(serv_adr));
	if (nRet == -1)
	{
		std::cerr << "EpollServer::SocketInit : Failed to bind.\n";
		return false;
	}

	nRet = listen(m_ListenSocket, 5);
	if (nRet == -1)
	{
		std::cerr << "EpollServer::SocketInit : Failed to Listen\n";
		return false;
	}

	epoll_fd = epoll_create(EPOLL_SIZE);
	if (nRet < 0)
	{
		std::cerr << "EpollServer::SocketInit : Failed to Allocate Epoll fd.\n";
		return false;
	}

	epoll_event event{};
	event.events = EPOLLIN;
	event.data.fd = m_ListenSocket;
	nRet = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, m_ListenSocket, &event);
	if (nRet == -1)
	{
		std::cerr << "EpollServer::SocketInit : Failed to Register Epoll fd.\n";
		return false;
	}

	return true;
}

bool EpollServer::CreateThreads()
{
	try
	{
		m_ReadThread = std::thread([this]() {ReadThread(); });
	}
	catch (std::system_error& e)
	{
		std::cerr << "EpollServer::CreateThread : Failed to Create Threads.\n";
		return false;
	}

	return true;
}

bool EpollServer::DestoryThreads()
{
	try
	{
		if (m_ReadThread.joinable())
		{
			m_ReadThread.join();
		}
	}
	catch (std::system_error& e)
	{
		std::cerr << "EpollServer::DestroyThreads : Failed to Join Threads.\n";
		return false;
	}

	return true;
}

bool EpollServer::CreateContext(const int fd_)
{
	if (fd_ < 0)
	{
		return false;
	}

	ClientContext* pCC = new(std::nothrow) ClientContext();

	if (pCC == nullptr)
	{
		std::cerr << "EpollServer::CreateContext : Failed to Allocate ClientContext.\n";
		return false;
	}

	pCC->Init(fd_);

	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_, &(pCC->GetEpollEvent()));

	SpinLockGuard guard(m_ClientLock);

	auto pair = Clients.emplace(fd_, pCC);

	if (pair.second == false)
	{
		std::cerr << "EpollServer::CreateContext : Duplicated fd.\n";
		return false;
	}

	return true;
}

bool EpollServer::ReleaseContext(const int fd_)
{
	SpinLockGuard guard(m_ClientLock);

	auto itr = Clients.find(fd_);

	if (itr == Clients.end())
	{
		return false;
	}

	ClientContext* pCC = itr->second;

	Clients.erase(itr);

	if (pCC != nullptr)
	{
		delete pCC;
	}

	return true;
}

void EpollServer::ReleaseAllContexts()
{
	SpinLockGuard guard(m_ClientLock);

	for (auto& pair : Clients)
	{
		ClientContext* pCC = pair.second;

		if (pCC != nullptr)
		{
			delete pCC;
		}
	}

	Clients.clear();
	return;
}

void EpollServer::ReadThread()
{
	epoll_event events[EPOLL_EVENT_SIZE]{};
	int events_cnt = 0;

	while (m_IsRun)
	{
		events_cnt = epoll_wait(epoll_fd, events, EPOLL_EVENT_SIZE, -1);

		for (int i = 0; i < events_cnt; i++)
		{
			if (events[i].data.fd == m_ListenSocket)
			{
				// 새로운 연결
				DoAccept();
				continue;
			}

			uint32_t ev = events[i].events;

			if (ev & EPOLLHUP || ev & EPOLLERR)
			{
				// 연결끊김 혹은 에러
				Close(events[i].data.fd);
				continue;
			}

			if (ev & EPOLLOUT)
			{
				// 송신가능상태
				ResumeSend(events[i].data.fd);
			}

			if (ev & EPOLLIN)
			{
				// 수신가능상태
				DoRecv(events[i].data.fd);
			}
		}
	}
}

ClientContext* EpollServer::GetContext(const int fd_)
{
	SpinLockGuard guard(m_ClientLock);

	auto itr = Clients.find(fd_);
	if (itr == Clients.end())
	{
		return nullptr;
	}

	return itr->second;
}


void EpollServer::DoAccept()
{
	struct sockaddr_in clnt_Adr{};
	socklen_t adr_size = sizeof(clnt_Adr);

	int clientSock = accept(m_ListenSocket, reinterpret_cast<sockaddr*>(&clnt_Adr), &adr_size);

	if (clientSock == -1)
	{
		std::cerr << "EpollServer::DoAccept : Failed to Accept.\n";
		return;
	}

	bool bRet = CreateContext(clientSock);

	if (!bRet)
	{
		std::cerr << "EpollServer::DoAccept : Failed to CreateContext.\n";
		return;
	}

	OnConnect(clientSock);
	return;
}

void EpollServer::DoRecv(const int fd_)
{
	char buf[MAX_RECV_SLIDEBUFFER_SIZE]{};

	int ioSize = read(fd_, buf, MAX_RECV_SLIDEBUFFER_SIZE - 1);

	if (ioSize == 0)
	{
		// 연결 해제
		Close(fd_);
		return;
	}

	ClientContext* pCC = GetContext(fd_);
	if (pCC == nullptr)
	{
		std::cerr << "EpollServer::DoRecv : ClientContext nullptr.\n";
		return;
	}

	bool bRet = pCC->StorePartialMsg(buf, ioSize);

	if (!bRet)
	{
		std::cerr << "EpollServer::DoRecv : Failed to Store Msg.\n";
		return;
	}

	std::string_view sv;

	while (pCC->GetReqMsg(sv))
	{
		OnReceive(fd_, sv);

		pCC->PopMsg(sv);
	}

	return;
}

void EpollServer::Close(const int fd_)
{
	OnDisconnect(fd_);

	int nRet = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd_, NULL);

	if (nRet == -1)
	{
		std::cerr << "EpollServer::Close : epoll_ctl Failed.\n";
	}

	ClientContext* pCC = GetContext(fd_);

	if (pCC == nullptr)
	{
		std::cerr << "EpollServer::Close : ClientContext nullptr.\n";
		return;
	}

	pCC->Close();

	ReleaseContext(fd_);

	return;
}

void EpollServer::ResumeSend(const int fd_)
{
	ClientContext* pCC = GetContext(fd_);

	if (pCC == nullptr)
	{
		std::cerr << "EpollServer::SendMsg : ClientContext nullptr.\n";
		return;
	}

	SendStatus eRet = pCC->ResumeSend();

	switch (eRet)
	{
	case SendStatus::WHOLE_SUCCESS:
	{
		epoll_event& event = pCC->GetEpollEvent();
		event.events = EPOLLIN;
		int nRet = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd_, &event); // 송신 감지 해제
		if (nRet == -1)
		{
			std::cerr << "EpollServer::SendMsg : Failed to Changer fd Mode.\n";
			return;
		}
	}
		[[fallthrough]];
	case SendStatus::PARTIAL_SUCCESS:
		[[fallthrough]];
	case SendStatus::FAILED:
		return;
	default:
		std::cerr << "EpollServer::SendMsg : Undefined Status.\n";
		return;
	}
}
