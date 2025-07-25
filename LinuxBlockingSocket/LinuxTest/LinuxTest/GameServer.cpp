#include "GameServer.hpp"

GameServer::GameServer(uint16_t nBindPort_) : m_IsRun(true)
{
	bool bRet = SocketServer::Run(nBindPort_);

	if (bRet == false)
	{
		std::cerr << "GameServer::Constructor : Failed to Init SocketServer.\n";
	}
}

GameServer::~GameServer()
{
	SocketServer::Stop();
}

bool GameServer::Start()
{
	m_IsRun = true;

	m_JobFactory.Init();
	Job::SendMsgFunc = [this](const uint32_t uClientIndex_, PacketData* pPacket_) -> InfoCode { return SendResultMsg(uClientIndex_, pPacket_); };

	bool bRet = CreateJobThread();

	if (!bRet)
	{
		std::cerr << "GameServer::Start : Failed to Create JobThread\n";
		return false;
	}

	return true;
}

bool GameServer::End()
{
	m_IsRun = false;

	bool bRet = DestroyJobThread();

	if (!bRet)
	{
		std::cerr << "GameServer::End : Failed to Join JobThread\n";
		return false;
	}
	return true;
}

bool GameServer::CreateJobThread()
{
	try
	{
		m_JobThread = std::thread([this]() {JobThread(); });
	}
	catch (std::system_error& e)
	{
		std::cerr << "GameServer::CreateJobThread : Failed to Create Thread.\n";
		return false;
	}
	catch (...)
	{
		std::cerr << "GameServer::CreateJobThread : Some Err Occured\n";
		return false;
	}

	std::cout << "GameServer::CreateJobThread : Created Job Thread.\n";

	return true;
}

void GameServer::JobThread()
{
	Job* pJob = nullptr;
	while (m_IsRun)
	{
		{
			SpinLockGuard guard(m_Lock);

			if (m_JobQueue.empty())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}
			pJob = m_JobQueue.front();
			m_JobQueue.pop();
		}

		InfoCode eRet = pJob->Execute();

		switch (eRet)
		{
		case InfoCode::REQ_SUCCESS:
		case InfoCode::REQ_FAILED:
			// -- Done.
			m_JobFactory.DeallocateJob(pJob);
			break;
		case InfoCode::NOT_FINISHED:
			// -- Re-Queueing.
			m_JobQueue.push(pJob);
			break;
		default:
			std::cerr << "GameServer::JobThread : Undefined InfoCode : [" << static_cast<int32_t>(eRet) << "]\n";
			m_JobFactory.DeallocateJob(pJob);
			break;
		}
	}
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
		std::cerr << "GameServer::DestroyJobThread : " << e.what() << '\n';
		return false;
	}

	return true;
}

void GameServer::OnReceive(int nClientIndex_, std::string_view& sv_)
{
	Job* pJob = m_JobFactory.CreateJob(nClientIndex_, sv_);

	if (pJob == nullptr)
	{
		std::cerr << "GameServer::OnReceive : Failed to Get Memory Block.\n";
		return;
	}

	m_JobQueue.push(pJob);
	return;
}

void GameServer::OnConnect(int nClientIndex_)
{
	std::cout << "GameServer::OnConnect : Socket[" << nClientIndex_ << "] Connected\n";
	return;
}

void GameServer::OnDisconnect(int nClientIndex_)
{
	std::cout << "GameServer::OnDisConnect : Socket[" << nClientIndex_ << "] Disconnected\n";
	return;
}