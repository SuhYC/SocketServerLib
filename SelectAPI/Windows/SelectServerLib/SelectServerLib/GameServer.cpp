#include "GameServer.hpp"


GameServer::GameServer(uint16_t uBindPort_) : m_IsRun(false)
{
	bool bRet = SelectServer::Start(uBindPort_);

	if (!bRet)
	{
		std::cerr << "GameServer::Constructor : Failed to Init.\n";
	}
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

	Job::SendMsgFunc = [this](const SOCKET clientSocket_, PacketData* pPacket_) -> InfoCode {return SendResultMsg(clientSocket_, pPacket_); };
	m_JobFactory.Init();

	bool bRet = CreateJobThread();

	if (!bRet)
	{
		std::cerr << "GameServer::Start : Failed to Create JobThread.\n";
		return false;
	}
}

bool GameServer::End()
{
	m_IsRun = false;

	bool bRet = DestroyJobThread();

	if (!bRet)
	{
		std::cerr << "GameServer::End : Failed to Join JobThread.\n";
		return false;
	}

	return SelectServer::End();
}

bool GameServer::CreateJobThread()
{
	try
	{
		m_JobThread = std::thread([this]() {JobThread(); });
	}
	catch (std::system_error& e)
	{
		std::cerr << "GameServer::CreateJobThread : Failed to Create thread.\n";
		return false;
	}

	return true;
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
		std::cerr << "GameServer::DestroyJobThread : Failed to Join.\n";
		return false;
	}

	return true;
}

InfoCode GameServer::SendResultMsg(const SOCKET clientSocket_, PacketData* pPacket_)
{
	bool bRet = SendMsg(clientSocket_, pPacket_);

	if (!bRet)
	{
		return InfoCode::NOT_FINISHED;
	}

	return InfoCode::REQ_SUCCESS;
}

void GameServer::JobThread()
{
	Job* pJob = nullptr;

	while (m_IsRun)
	{
		bool bRet = m_JobQueue.try_pop(pJob);

		if (!bRet)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
			continue;
		}

		if (pJob == nullptr)
		{
			std::cerr << "GameServer::JobThread : Job ptr nullptr.\n";
			continue;
		}

		InfoCode eRet = pJob->Execute();

		switch (eRet)
		{
		case InfoCode::REQ_SUCCESS:
		case InfoCode::REQ_FAILED:
			// -- Done.
			delete pJob;
			break;
		case InfoCode::NOT_FINISHED:
			m_JobQueue.push(pJob);
			break;
		default:
			std::cerr << "GameServer::JobThread : err" << static_cast<int>(eRet) << "\n";
			delete pJob;
			break;
		}

	}
}

void GameServer::OnReceive(SOCKET clientSocket_)
{
	std::string_view sv;

	while (GetReqMsg(clientSocket_, sv))
	{
		Job* pJob = m_JobFactory.CreateJob(clientSocket_, sv);

		if (pJob == nullptr)
		{
			std::cerr << "GameServer::OnReceive : Failed to Create Job.\n";
			return;
		}

		m_JobQueue.push(pJob);

		bool bRet = PopMsg(clientSocket_, sv);

		if (!bRet)
		{
			std::cerr << "GameServer::OnReceive : Failed to Pop Msg.\n";
			return;
		}
	}
	return;
}

void GameServer::OnConnect(SOCKET clientSocket_)
{
	std::cout << "SOCKET[" << clientSocket_ << "] Connected.\n";
	return;
}

void GameServer::OnDisconnect(SOCKET clientSocket_)
{
	std::cout << "SOCKET[" << clientSocket_ << "] Disconnected.\n";
	return;
}