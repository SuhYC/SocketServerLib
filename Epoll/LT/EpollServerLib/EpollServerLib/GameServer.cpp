#include "GameServer.hpp"

GameServer::GameServer(uint16_t uBindPort_)
{
	EpollServer::Start(uBindPort_);
}

GameServer::~GameServer()
{
	if (m_IsRun)
	{
		End();
	}
}

bool GameServer::Start()
{
	m_IsRun = true;

	m_JobFactory.Init();

	Job::SendMsgFunc = [this](int fd_, PacketData* pPacket_) -> InfoCode {
		bool bRet = SendMsg(fd_, pPacket_);

		if (!bRet)
		{
			return InfoCode::NOT_FINISHED;
		}
		return InfoCode::REQ_SUCCESS;
		};

	bool bRet = CreateJobThread();
	if (!bRet)
	{
		std::cerr << "GameServer::Start : Failed to Allocate JobThread.\n";
		return false;
	}

	return true;
}

void GameServer::End()
{
	m_IsRun = false;

	bool bRet = DestroyJobThread();
	if (!bRet)
	{
		std::cerr << "GameServer::End : JobThread join Failed.\n";
	}

	EpollServer::End();

	return;
}

bool GameServer::CreateJobThread()
{
	try
	{
		m_JobThread = std::thread([this]() {JobThread(); });
	}
	catch (std::system_error& e)
	{
		std::cerr << "GameServer::CreateJobThread : Failed to Allocate Thread.\n";
		return false;
	}

	return true;
}

void GameServer::JobThread()
{
	Job* pJob = nullptr;
	while (m_IsRun)
	{
		{
			SpinLockGuard guard(m_JobLock);
			if (m_JobQueue.empty())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(30));
				continue;
			}
			pJob = m_JobQueue.front();
			m_JobQueue.pop();
		}

		if (pJob == nullptr)
		{
			std::cerr << "GameServer::JobThread : Job nullptr.\n";
			continue;
		}

		InfoCode eRet = pJob->Execute();

		switch (eRet)
		{
		case InfoCode::REQ_SUCCESS:
		case InfoCode::REQ_FAILED:
			delete pJob;
			break;
		case InfoCode::NOT_FINISHED:
			m_JobQueue.push(pJob);
			break;
		default:
			delete pJob;
			std::cerr << "GameServer::JobThread : InfoCode : " << static_cast<int32_t>(eRet) << '\n';
			break;
		}
	}

	while (!m_JobQueue.empty())
	{
		pJob = m_JobQueue.front();
		m_JobQueue.pop();

		if (pJob != nullptr)
		{
			delete pJob;
		}
	}

	return;
}

bool GameServer::DestroyJobThread()
{
	try
	{
		if (m_JobThread.joinable())
		{
			m_JobThread.join();
		}
	}
	catch (std::system_error& e)
	{
		std::cerr << "GameServer::DestroyJobThread : thread join Failed.\n";
		return false;
	}
	
	return true;
}

void GameServer::OnReceive(const int fd_, std::string_view& sv_)
{
	Job* pJob = m_JobFactory.CreateJob(fd_, sv_);

	if (pJob == nullptr)
	{
		std::cerr << "GameServer::OnReceive : Failed to Allocate Job.\n";
		return;
	}

	{
		SpinLockGuard guard(m_JobLock);

		m_JobQueue.push(pJob);
	}
}

void GameServer::OnConnect(const int fd_)
{
	std::cout << "GameServer::OnConnect : Client[" << fd_ << "] Connected.\n";
	return;
}

void GameServer::OnDisconnect(const int fd_)
{
	std::cout << "GameServer::OnDisconnect : Client[" << fd_ << "] Disconnected.\n";
	return;
}