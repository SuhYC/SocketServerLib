#include "GameServer.hpp"

GameServer::GameServer(uint32_t uBindPort_, uint32_t uMaxClient_) :m_IsRun(true), m_UserManager(uMaxClient_), m_PacketPool()
{
	bool bRet = Start(uBindPort_, uMaxClient_);

	m_IsRun = bRet;

	if (!bRet)
	{
		LOG_ERR("Init Failed");
		return;
	}
}

GameServer::~GameServer()
{

}

bool GameServer::Run()
{
	m_IsRun = true;

	m_JobFactory.Init({ &m_UserManager,&m_ZoneManager, &m_PacketPool });
	Job::SendMsgFunc = [this](const uint32_t uClientIndex_, PacketData* pPacket_) -> InfoCode { return SendResultMsg(uClientIndex_, pPacket_); };

	bool bRet = CreateJobThreads();

	if (!bRet)
	{
		LOG_ERR("Failed to Create JobThreads.");
		return false;
	}

	return true;
}

bool GameServer::End()
{
	m_IsRun = false;

	bool bRet = DestroyJobThreads();

	if (!bRet)
	{
		LOG_ERR("Failed to Join JobThreads.");
		return false;
	}

	return true;
}

void GameServer::OnReceive(const uint32_t uClientIndex_)
{
	std::string_view sv;

	while (GetReqMsg(uClientIndex_, sv))
	{
		Job* pJob = m_JobFactory.CreateJob(uClientIndex_, sv);

		if (pJob == nullptr)
		{
			LOG_ERR("Failed to Get Memory Block.");

			bool bRet = PopReqMsg(uClientIndex_, sv);

			if (!bRet)
			{
				LOG_ERR("Failed to Pop RecvBuffer.");
			}

			continue;
		}

		m_JobQueue.push(pJob);

		bool bRet = PopReqMsg(uClientIndex_, sv);

		if (!bRet)
		{
			LOG_ERR("Failed to Pop RecvBuffer.");
		}
	}

	return;
}

void GameServer::OnConnect(const uint32_t uClientIndex_, const uint32_t ip_)
{
	LOG_DEBUG("Socket[", uClientIndex_, "] Connected.");
}

void GameServer::OnDisconnect(const uint32_t uClientIndex_)
{
	LOG_DEBUG("Socket[", uClientIndex_, "] Disconnected.");
}

bool GameServer::CreateJobThreads()
{
	try
	{
		for (int i = 0; i < MAX_JOBTHREAD; i++)
		{
			m_JobThreads.emplace_back([this]() {JobThread(); });
		}
	}
	catch (std::system_error& e)
	{
		LOG_ERR("Failed to Create Job Threads.");
		return false;
	}
	DEFAULT_CATCH()

	LOG_DEBUG("Created JobThread");

	return true;
}

bool GameServer::DestroyJobThreads()
{
	try
	{
		for (int i = 0; i < m_JobThreads.size(); i++)
		{
			if (m_JobThreads[i].joinable())
			{
				m_JobThreads[i].join();
			}
		}
	}
	catch (std::system_error& e)
	{
		LOG_ERR(e.what());
		return false;
	}
	DEFAULT_CATCH()

	return true;
}

void GameServer::JobThread()
{
	Job* pJob = nullptr;
	while (m_IsRun)
	{
		if (!m_JobQueue.try_pop(pJob))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
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
			LOG_ERR("Undefined InfoCode : [", static_cast<int32_t>(eRet), "]");
			m_JobFactory.DeallocateJob(pJob);
			break;
		}
	}
}