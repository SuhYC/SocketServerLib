#include "GameServer.hpp"

GameServer::GameServer()
{

}

GameServer::~GameServer()
{

}

bool GameServer::Start(uint16_t uBindPort_, uint16_t uUdpPort_, uint32_t uMaxClient_)
{
	JobFactory::Instance().Init(
		[this](uint32_t uClientIndex_, PacketData* pPacket_, SendProtocol eSendPro_) -> bool {
			return SendMsgFunc(uClientIndex_, pPacket_, eSendPro_);
		},
		[this](uint32_t uClientIndex_) -> uint64_t {
			return GetUDPToken(uClientIndex_);
		}
		);

	try
	{
		ProcessLocks.resize(uMaxClient_);
		for (size_t i = 0; i < uMaxClient_; i++)
		{
			ProcessLocks[i] = std::make_unique<SpinLock>();
		}
	}
	catch (std::bad_alloc&)
	{
		LOG_ERR("Failed to Allocate SpinLocks");
		return false;
	}

	try
	{
		m_Jobs.resize(uMaxClient_);
		for (size_t i = 0; i < uMaxClient_; i++)
		{
			m_Jobs[i] = nullptr;
		}
	}
	catch (std::bad_alloc&)
	{
		LOG_ERR("Failed to Allocate Jobs Vector.");
		return false;
	}

	bool bRet = CreateJobThreads(MAX_JOBTHREAD);

	if (!bRet)
	{
		LOG_ERR("Failed to Create JobThreads.");

		return false;
	}

	return IOCPCore::Start(uBindPort_, uUdpPort_, uMaxClient_);
}

void GameServer::End()
{
	ReleaseJobThreads();

	IOCPCore::End();
}

bool GameServer::CreateJobThreads(const uint16_t uMaxThreads_)
{
	return JobThreads.Start(uMaxThreads_);
}


bool GameServer::PushJob(const uint32_t uClientIndex_)
{
	std::function<InfoCode()> func = [this, uClientIndex_]() -> InfoCode
		{
			return HandleReq(uClientIndex_);
		};

	return JobThreads.Enqueue(std::move(func));
}

void GameServer::ReleaseJobThreads()
{
	JobThreads.Stop();
	return;
}

void GameServer::OnTCPReceive(const uint32_t uClientIndex_)
{
	if (!PushJob(uClientIndex_))
	{
		LOG_ERR("Failed to Push Job.");
	}
}

void GameServer::OnUDPReceive(const uint32_t uClientIndex_, std::string_view& msg_)
{
	// 수신한 내용을 작업객체로 파싱해서 큐잉한다.
	Job* pJob = JobFactory::Instance().CreateUDPJob(uClientIndex_, msg_);

	if (pJob == nullptr)
	{
		// 파싱 실패 시 그냥 버린다.
		return;
	}

	std::function<InfoCode()> func = [this, uClientIndex_, pJob]() -> InfoCode
		{
			// 파싱은 람다식 넘기기 전에 작업객체로 만들었기 때문에 넘어간다.

			if (pJob->Execute())
			{
				pJob->SendRes();
			}

			JobFactory::Instance().DeallocateJob(pJob);

			return InfoCode::REQ_SUCCESS; // 처리에 실패해도 유실된걸로 치고 넘긴다. 신뢰성이 필요한 요청을 하지 말것.
		};

	JobThreads.Enqueue(std::move(func));

	return;
}

void GameServer::OnConnect(const uint32_t uClientIndex_, const sockaddr_in& addr_)
{
	uint32_t ip = addr_.sin_addr.S_un.S_addr;
	
	char ipStr[INET_ADDRSTRLEN] = {}; // IPv4용 버퍼
	InetNtopA(AF_INET, &addr_.sin_addr, ipStr, INET_ADDRSTRLEN);

	LOG_DEBUG("Client[", uClientIndex_, "] Connected. ip : ", ipStr);

	return;
}

void GameServer::OnDisconnect(const uint32_t uClientIndex_)
{
	LOG_DEBUG("Client[", uClientIndex_, "] Disconnected.");

	return;
}

InfoCode GameServer::HandleReq(const uint32_t uClientIndex_)
{
	if (uClientIndex_ >= ProcessLocks.size())
	{
		return InfoCode::OTHER_ERR;
	}

	SpinLockGuard guard(*ProcessLocks[uClientIndex_], try_to_lock);

	if (!guard.owns_lock())
	{
		return InfoCode::NOT_MY_TURN;
	}

	JobProcess eRet = GetJobProcess(uClientIndex_);

	while (true)
	{
		// 중단한 작업이 있다면 해당 위치부터 아래로 전부 실행하기 위해 break; 생략
		switch (eRet)
		{
		case JobProcess::PARSING:
			if (!ParseReq(uClientIndex_))
			{
				SetJobProcess(uClientIndex_, JobProcess::PARSING);
				return InfoCode::REQ_FAILED; // 현재 작업할 요청 없음.
			}
			[[fallthrough]];
		case JobProcess::EXECUTE:
			if (!ExecuteReq(uClientIndex_))
			{
				SetJobProcess(uClientIndex_, JobProcess::EXECUTE);
				return InfoCode::NOT_FINISHED;
			}
			[[fallthrough]];
		case JobProcess::SEND:
			if (!SendRes(uClientIndex_))
			{
				SetJobProcess(uClientIndex_, JobProcess::SEND);
				return InfoCode::NOT_FINISHED;
			}

			// 초기상태로 가기 위해 초기화 후 break; -> 반복
			eRet = JobProcess::PARSING;
			break;
		default:
			LOG_ERR("Undefined Enum : ", static_cast<int32_t>(eRet));
			return InfoCode::OTHER_ERR;
		}
	}
}

bool GameServer::ParseReq(const uint32_t uClientIndex_)
{
	if (uClientIndex_ >= m_Jobs.size())
	{
		LOG_ERR("Invalid Index : ", uClientIndex_);
		return false;
	}

	std::string_view sv;
	if (!GetReqMsg(uClientIndex_, sv))
	{
		return false;
	}

	Job* pJob = JobFactory::Instance().CreateJob(uClientIndex_, sv);

	if (pJob == nullptr)
	{
		LOG_ERR("Failed to Get Job Object.");
		return false;
	}

	if (m_Jobs[uClientIndex_] != nullptr)
	{
		JobFactory::Instance().DeallocateJob(m_Jobs[uClientIndex_]);
	}

	m_Jobs[uClientIndex_] = pJob;

	PopReqMsg(uClientIndex_, sv);

	return true;
}

bool GameServer::ExecuteReq(const uint32_t uClientIndex_)
{
	if (uClientIndex_ >= m_Jobs.size())
	{
		LOG_ERR("Invalid Index : ", uClientIndex_);
		return false;
	}

	if (m_Jobs[uClientIndex_] == nullptr)
	{
		LOG_ERR("No Job Object on Vector.");
		return false;
	}

	if (!m_Jobs[uClientIndex_]->Execute())
	{
		return false;
	}

	return true;
}

bool GameServer::SendRes(const uint32_t uClientIndex_)
{
	if (uClientIndex_ >= m_Jobs.size())
	{
		LOG_ERR("Invalid Index : ", uClientIndex_);
		return false;
	}

	if (m_Jobs[uClientIndex_] == nullptr)
	{
		LOG_ERR("No Job Object on Vector.");
		return false;
	}

	if (!m_Jobs[uClientIndex_]->SendRes())
	{
		return false;
	}

	JobFactory::Instance().DeallocateJob(m_Jobs[uClientIndex_]);
	m_Jobs[uClientIndex_] = nullptr;

	return true;
}

bool GameServer::SendMsgFunc(const uint32_t uClientIndex_, PacketData* pPacket_, SendProtocol eProtocol_)
{
	if (pPacket_ == nullptr)
	{
		return false;
	}

	if (eProtocol_ == SendProtocol::TCP)
	{
		return SendMsgTCP(uClientIndex_, pPacket_);
	}
	else if (eProtocol_ == SendProtocol::UDP)
	{
		return SendMsgUDP(uClientIndex_, pPacket_);
	}

	LOG_ERR("Invalid SendProtocol.", static_cast<int32_t>(eProtocol_));

	return false;
}

void GameServer::ReleaseRemainJobs()
{
	for (Job* pJob : m_Jobs)
	{
		if (pJob != nullptr)
		{
			pJob->~Job();

			// 메모리풀을 통해 할당된 객체임. 이후에 다른 경로로 생성될 경우 주의.
			::operator delete(pJob);
		}
	}
}