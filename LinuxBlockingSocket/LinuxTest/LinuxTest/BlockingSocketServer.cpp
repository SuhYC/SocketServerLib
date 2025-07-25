#include "BlockingSocketServer.hpp"

bool SocketServer::Run(uint16_t nBindPort_)
{
	m_IsRun = true;
	if (!Init(nBindPort_))
	{
		return false;
	}

	CreateAcceptThread();

	return true;
}

void SocketServer::Stop()
{
	m_IsRun = false;

	DestroyThreads();
}

bool SocketServer::SendMsg(int nClientIndex_, PacketData* pPacket_)
{
	if (pPacket_ == nullptr)
	{
		return false;
	}

	ssize_t totalSent = 0;
	
	// OS 커널의 송신버퍼에 다 못담는 경우도 있다고는 함. 그래서 루프.
	while (totalSent < pPacket_->GetSize())
	{
		int nRet = write(nClientIndex_, pPacket_->GetData() + totalSent, pPacket_->GetSize() - totalSent);

		if (nRet < 0)
		{
			int err = errno;
			std::cerr << "SocketServer::SendMsg : errcode : " << err << strerror(err) << '\n';
			return false;
		}

		std::cout << "SocketServer::SendMsg : " << nRet << "bytes Sent.\n";
		totalSent += nRet;
	}

	return true;
}

bool SocketServer::Init(uint16_t nBindPort_)
{
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	if (serv_sock == -1)
	{
		std::cerr << "SocketServer::Init : Failed to Create Server Socket.\n";
		return false;
	}

	sockaddr_in serv_addr{};

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(nBindPort_);

	int nRet = bind(serv_sock, reinterpret_cast<sockaddr*>(&serv_addr), sizeof(serv_addr));

	if (nRet == -1)
	{
		std::cerr << "SocketServer::Init : Failed to Bind Socket.\n";
		return false;
	}

	nRet = listen(serv_sock, 5);

	if (nRet == -1)
	{
		std::cerr << "SocketServer::Init : Failed to Listen.\n";
		return false;
	}

	return true;
}

void SocketServer::CreateClientThread(int clnt_sock_)
{
	ClientContext* pCC = new(std::nothrow) ClientContext(clnt_sock_);

	if (pCC == nullptr)
	{
		std::cerr << "SocketServer::AcceptThread : Not enough Mem On Allocate ClientContext.\n";
		close(clnt_sock_);
		return;
	}

	bool bRet = pCC->Init([this, pCC, clnt_sock_]() {ClientThread(clnt_sock_, pCC->GetAliveRef()); });

	if (bRet == false)
	{
		std::cerr << "SocketServer::CreateClientThread : Failed to Init ClientContext.\n";
		return;
	}

	m_Clients.push_back(pCC);

	return;
}

void SocketServer::ClientThread(int nClientIndex_, bool& isAlive_)
{
	SlideBuffer slideBuf;
	if (!slideBuf.Init(MAX_BUF_SIZE))
	{
		std::cerr << "SocketServer::ClientThread : Cant Allocate Buffer.\n";
		return;
	}

	char buf[1024]{};

	OnConnect(nClientIndex_);

	while (m_IsRun)
	{
		int nRet = read(nClientIndex_, buf, 1024);

		if (nRet > 0)
		{
			slideBuf.Enqueue(buf, nRet);

			while (slideBuf.Peek() <= slideBuf.GetSize() && slideBuf.GetSize() > 0)
			{
				std::string_view sv(slideBuf.GetBuf(), slideBuf.Peek());

				OnReceive(nClientIndex_, sv);

				slideBuf.Pop(sv.size());
			}
			continue;
		}

		if (nRet == 0)
		{
			OnDisconnect(nClientIndex_);
			break;
		}

		if (nRet < 0)
		{
			int err = errno;

			std::cerr << "SocketServer::ClientThread : Err on Recv : " << err << strerror(err) << '\n';

			continue;
		}
	}

	if (close(nClientIndex_) == -1)
	{
		int err = errno;
		std::cerr << "SocketServer::ClientThread : Err on Close : " << err << strerror(err) << '\n';
	}

	RequestToCleanUpThreads();

	return;
}

void SocketServer::CreateAcceptThread()
{
	m_AcceptThread = std::thread([this]() {AcceptThread(); });

	return;
}

void SocketServer::AcceptThread()
{
	sockaddr_in clnt_addr{};
	socklen_t addr_size = sizeof(clnt_addr);
	int clnt_sock;

	while (m_IsRun)
	{
		CleanUpThreads();

		clnt_sock = accept(serv_sock, reinterpret_cast<sockaddr*>(&clnt_addr), &addr_size);

		if (clnt_sock == -1)
		{
			int err = errno;

			switch (err)
			{
			case EAGAIN:
				// 블로킹 소켓이긴 한데 만약 논블로킹이라면 현재 연결할 대기중인 클라이언트 없음.
			case EINTR:
				// 인터럽트 발생해서 잠시 블로킹이 해제됨 (다시 시도하면 됨)
			case ECONNABORTED:
				// 연결하려고 대기큐에서 클라이언트를 가져왔는데 해당 클라이언트가 이미 종료됨
				continue;
			default:
				std::cerr << "SocketServer::AcceptThread : Failed to Accept.\n";
				std::cerr << "SocketServer::AcceptThread : errno : " << err << ", err string : " << strerror(err) << '\n';
				return;
			}
		}

		CreateClientThread(clnt_sock);
	}
}

void SocketServer::RequestToCleanUpThreads()
{
	++reqCnt;
	return;
}

void SocketServer::CleanUpThreads()
{
	if (reqCnt < THREAD_CLEAN_REQ_CNT)
	{
		return;
	}

	reqCnt.store(0);

	for (ClientContext* pCC : m_Clients)
	{
		if (pCC == nullptr)
		{
			std::cerr << "SocketServer::CleanUpThreads : nullptr on m_Clients.\n";
			continue;
		}

		pCC->check();
	}

	m_Clients.erase(
		std::remove_if(m_Clients.begin(), m_Clients.end(), [](ClientContext*& ctx) { // nullptr거나 isTargeted면 정리.
			if (ctx == nullptr)
			{
				return true;
			}
			if (ctx->GetStatus())
			{
				delete ctx;
				ctx = nullptr;
				return true;
			}
			return false;
		}),
		m_Clients.end()
	);
}


void SocketServer::DestroyThreads()
{
	if (m_AcceptThread.joinable())
	{
		m_AcceptThread.join();
	}

	for (ClientContext* pCC : m_Clients)
	{
		if (pCC == nullptr)
		{
			continue;
		}

		if (pCC->th.joinable())
		{
			pCC->th.join();
		}

		delete pCC;
	}

	return;
}