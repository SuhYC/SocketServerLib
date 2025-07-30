#include "SelectServer.hpp"


SelectServer::SelectServer() : m_IsRun(false), m_ListenSocket(INVALID_SOCKET), reads({}), writes({})
{

}

SelectServer::~SelectServer()
{
	WSACleanup();
}

bool SelectServer::Start(uint16_t uBindPort_)
{
	m_IsRun = true;

	bool bRet = SocketInit(uBindPort_);

	if (!bRet)
	{
		std::cerr << "SelectServer::Start : Socket Init Failed.\n";
		return false;
	}
	
	std::cout << "소켓 초기화 성공.\n";

	bRet = CreateThread();

	if (!bRet)
	{
		std::cerr << "SelectServer::Start : Failed to Create Thread.\n";
		return false;
	}

	std::cout << "스레드 생성 성공.\n";

	return true;
}

bool SelectServer::End()
{
	m_IsRun = false;

	DestroyThread();

	ReleaseAllContexts();

	closesocket(m_ListenSocket);
	m_ListenSocket = INVALID_SOCKET;

	return true;
}

bool SelectServer::SendMsg(const SOCKET clientSocket_, PacketData* pPacket_)
{
	ClientContext* pCC = GetContext(clientSocket_);

	if (pCC == nullptr)
	{
		std::cerr << "SelectServer::SendMsg : Context nullptr.\n";
		return false;
	}

	SendStatus eRet = pCC->SendMsg(pPacket_);
	if (eRet == SendStatus::WHOLE_SUCCESS)
	{
		return true;
	}
	else if (eRet == SendStatus::PARTIAL_SUCCESS)
	{
		// 재송신 필요
		FD_SET(clientSocket_, &writes);
		return true;
	}
	else
	{
		// 버퍼에 못담았음.
		return false;
	}
}

bool SelectServer::GetReqMsg(const SOCKET clientSocket_, std::string_view& out_)
{
	ClientContext* pCC = GetContext(clientSocket_);

	if (pCC == nullptr)
	{
		return false;
	}

	return pCC->GetReqMsg(out_);
}

bool SelectServer::PopMsg(const SOCKET clientSocket_, std::string_view& sv_)
{
	ClientContext* pCC = GetContext(clientSocket_);

	if (pCC == nullptr)
	{
		return false;
	}

	return pCC->PopMsg(sv_);
}

bool SelectServer::SocketInit(uint16_t uBindPort_)
{
	WSADATA wsaData;
	SOCKADDR_IN servAdr;

	ZeroMemory(&servAdr, sizeof(servAdr));

	int nRet = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (nRet != 0)
	{
		std::cerr << "SelectServer::SocketInit : Failed to WSAStartup.\n";
		return false;
	}

	m_ListenSocket = socket(PF_INET, SOCK_STREAM, 0);

	if (m_ListenSocket == INVALID_SOCKET)
	{
		std::cerr << "SelectServer::SocketInit : Failed to Allocate listen Socket.\n";
		return false;
	}

	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(uBindPort_);

	nRet = bind(m_ListenSocket, reinterpret_cast<sockaddr*>(&servAdr), sizeof(servAdr));

	if (nRet == SOCKET_ERROR)
	{
		std::cerr << "SelectServer::SocketInit : Failed to bind.\n";
		return false;
	}

	nRet = listen(m_ListenSocket, 5);

	if (nRet == SOCKET_ERROR)
	{
		std::cerr << "SelectServer::SocketInit : Failed to listen.\n";
		return false;
	}

	FD_ZERO(&reads);
	FD_ZERO(&writes);

	FD_SET(m_ListenSocket, &reads);

	return true;
}

bool SelectServer::CreateThread()
{
	try
	{
		m_ReadThread = std::thread([this]() {ReadThread(); });
		m_WriteThread = std::thread([this]() {WriteThread(); });
	}
	catch (std::system_error& e)
	{
		std::cerr << "SelectServer::CreateThread : Failed to Allocate Thread." << e.what() << '\n';
		return false;
	}


	return true;
}

bool SelectServer::DestroyThread()
{
	if (m_ReadThread.joinable())
	{
		m_ReadThread.join();
	}

	if (m_WriteThread.joinable())
	{
		m_WriteThread.join();
	}

	return true;
}

bool SelectServer::CreateContext(const SOCKET clientSocket_)
{
	if (clientSocket_ == INVALID_SOCKET)
	{
		return false;
	}

	SpinLockGuard guard(m_ClientLock);

	ClientContext* pCC = new(std::nothrow) ClientContext(clientSocket_);

	if (pCC == nullptr)
	{
		std::cerr << "SelectServer::CreateContext : Failed to Allocate ClientContext.\n";

		return false;
	}

	auto pair = Clients.emplace(clientSocket_, pCC);

	if (!(pair.second))
	{
		delete pCC;
		std::cerr << "SelectServer::CreateContext : Key Already Exist.\n";
		return false;
	}

	return true;
}

bool SelectServer::ReleaseContext(SOCKET clientSocket_)
{
	if (clientSocket_ == INVALID_SOCKET)
	{
		return false;
	}

	SpinLockGuard guard(m_ClientLock);

	auto itr = Clients.find(clientSocket_);

	if (itr == Clients.end())
	{
		std::cerr << "SelectServer::ReleaseContext : Key Not Found.";
		return false;
	}

	ClientContext* pCC = itr->second;

	Clients.erase(itr);

	if (pCC == nullptr)
	{
		closesocket(clientSocket_);
		return false;
	}

	pCC->Close();
	delete pCC;
	return true;
}

void SelectServer::ReleaseAllContexts()
{
	SpinLockGuard guard(m_ClientLock);

	for (auto& pair : Clients)
	{
		ClientContext* pCC = pair.second;

		if (pCC == nullptr)
		{
			if (pair.first != INVALID_SOCKET)
			{
				closesocket(pair.first);
			}
			continue;
		}

		pCC->Close();
		delete pCC;
	}

	Clients.clear();

	return;
}

ClientContext* SelectServer::GetContext(const SOCKET clientSocket_)
{
	SpinLockGuard guard(m_ClientLock);

	auto itr = Clients.find(clientSocket_);

	if (itr == Clients.end())
	{
		return nullptr;
	}

	return itr->second;
}

void SelectServer::ReadThread()
{
	fd_set cpyReads;
	TIMEVAL timeout{};

	// None-Blocking
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	
	sockaddr_in clntAdr{};
	int fdNum = 0, i = 0, adrSize = 0, strLen = 0;

	SOCKET clntSocket = INVALID_SOCKET;

	constexpr uint32_t MAX_BUF_SIZE = 1024;

	char recvbuf[MAX_BUF_SIZE]{};

	while (m_IsRun)
	{
		cpyReads = reads;

		fdNum = select(0, &cpyReads, 0, 0, &timeout);
		if (fdNum == SOCKET_ERROR)
		{
			int err = WSAGetLastError();

			std::cerr << "SelectServer::ReadThread : err : " << err << "\n";

			break;
		}

		if (fdNum == 0)
		{
			continue;
		}

		for (i = 0; i < reads.fd_count; i++)
		{
			if (!FD_ISSET(reads.fd_array[i], &cpyReads))
			{
				continue;
			}

			// Accept
			if (reads.fd_array[i] == m_ListenSocket)
			{
				DoAccept();
			}
			// Recv
			else
			{
				DoRecv(reads.fd_array[i]);
			}
		}
	}
}

void SelectServer::WriteThread()
{
	fd_set cpyWrites;

	TIMEVAL timeout{0,0};

	int fdNum = 0, i = 0;

	while (m_IsRun)
	{
		if (writes.fd_count < 1)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
			continue;
		}

		cpyWrites = writes;

		fdNum = select(0, nullptr, &cpyWrites, 0, &timeout);

		if (fdNum == 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
			continue;
		}

		if (fdNum == SOCKET_ERROR)
		{
			int err = WSAGetLastError();

			std::cerr << "SelectServer::WriteThread : err : " << err << "\n";
			break;
		}

		for (i = 0; i < writes.fd_count; i++)
		{
			if (!FD_ISSET(writes.fd_array[i], &cpyWrites))
			{
				continue;
			}

			ResumeSend(writes.fd_array[i]);
		}
	}
}

void SelectServer::DoAccept()
{
	sockaddr_in clntAdr{};
	int adrSize = sizeof(clntAdr);

	SOCKET clntSocket = accept(m_ListenSocket, reinterpret_cast<sockaddr*>(&clntAdr), &adrSize);

	if (clntSocket == INVALID_SOCKET)
	{
		int err = WSAGetLastError();

		if (err != WSAEWOULDBLOCK)
		{
			std::cerr << "SelectServer::ReadThread : err : " << err << "\n";
		}

		return;
	}

	FD_SET(clntSocket, &reads);

	CreateContext(clntSocket);

	OnConnect(clntSocket);

	return;
}

void SelectServer::DoRecv(const SOCKET clientSocket_)
{
	constexpr uint32_t MAX_BUF_SIZE = 1024;

	char recvbuf[MAX_BUF_SIZE]{};

	int strLen = recv(clientSocket_, recvbuf, MAX_BUF_SIZE - 1, 0);

	if (strLen == 0)
	{
		OnDisconnect(clientSocket_);
		ReleaseContext(clientSocket_);
		FD_CLR(clientSocket_, &reads);

		return;
	}

	ClientContext* pCC = GetContext(clientSocket_);

	if (pCC == nullptr)
	{
		std::cerr << "SelectServer::DoRecv : Context nullptr.\n";
		return;
	}

	pCC->StorePartialMsg(recvbuf, strLen);

	OnReceive(clientSocket_);

	return;
}

void SelectServer::ResumeSend(const SOCKET clientSocket_)
{
	ClientContext* pCC = GetContext(clientSocket_);

	if (pCC == nullptr)
	{
		std::cerr << "SelectServer::ResumeSend : Context nullptr.\n";
		return;
	}

	SendStatus eRet = pCC->ResumeSend();

	if (eRet == SendStatus::WHOLE_SUCCESS)
	{
		FD_CLR(clientSocket_, &writes);
	}

	return;
}