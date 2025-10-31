#include "Job.hpp"

Job::Job(DIStruct* stDI_) : m_stDI(stDI_), m_pPacket(nullptr), m_ClientIndex(0), m_ReqNo(0)
{

}

Job::~Job()
{
	if (m_pPacket != nullptr)
	{
		delete m_pPacket;
	}
}

JobFactory::JobFactory() : m_pool(sizeof(Jobs), MAX_MEMORY_BLOCKS)
{
	try
	{
		createFuncs.resize(static_cast<size_t>(ReqType::LAST) + 1);
	}
	catch (std::bad_alloc&)
	{
		LOG_ERR("Failed to Allocate Vector.");
	}

	Register<EchoJob>(ReqType::ECHO);
	Register<ReqTokenJob>(ReqType::REQ_TOKEN);
	Register<UDPEchoJob>(ReqType::UDPECHO);
}

JobFactory& JobFactory::Instance()
{
	static JobFactory instance;

	return instance;
}

bool JobFactory::Init(SENDFUNC sendFunc_, TOKENFUNC tokenFunc_)
{
	m_DIStruct.sendFunc = sendFunc_;
	m_DIStruct.getTokenFunc = tokenFunc_;

	return true;
}

Job* JobFactory::CreateJob(uint32_t uUserIndex_, std::string_view& sv)
{
	if (sv.size() < HEADER_SIZE)
	{
		LOG_ERR("Too short. msg size : ", sv.size());

		return nullptr;
	}
	else if (sv.size() > sizeof(ReqMessage))
	{
		LOG_ERR("Too Long. msg size : ", sv.size());

		return nullptr;
	}

	ReqMessage reqMsg{};

	CopyMemory(&reqMsg, sv.data(), sv.size());

	if (reqMsg.header.reqType > static_cast<int32_t>(ReqType::LAST))
	{
		LOG_ERR("Wrong Req Type : ", reqMsg.header.reqType);
		return nullptr;
	}

	auto& func = createFuncs[reqMsg.header.reqType];

	if (!func)
	{
		LOG_ERR("Can't Find CreateJob Func : ", reqMsg.header.reqType);
		return nullptr;
	}

	Job* pRet = func();

	std::string_view payload(reqMsg.payload, sv.size() - sizeof(ReqHeader));

	if (pRet == nullptr || !pRet->Parse(payload))
	{
		LOG_ERR("[",reqMsg.header.reqType, "] Failed to Parse.");

		if (pRet != nullptr)
		{
			DeallocateJob(pRet);
		}

		return nullptr;
	}

	pRet->m_ClientIndex = uUserIndex_;
	pRet->m_ReqNo = reqMsg.header.reqNo;

	return pRet;
}

Job* JobFactory::CreateUDPJob(uint32_t uUserIndex_, std::string_view& sv_)
{
	if (sv_.size() < HEADER_SIZE)
	{
		LOG_ERR("Too short. msg size : ", sv_.size());

		return nullptr;
	}
	else if (sv_.size() > sizeof(ReqMessage))
	{
		LOG_ERR("Too Long. msg size : ", sv_.size());

		return nullptr;
	}

	UDPReqMessage reqMsg{};

	CopyMemory(&reqMsg, sv_.data(), sv_.size());

	if (reqMsg.header.reqType > static_cast<int32_t>(ReqType::LAST))
	{
		LOG_ERR("Wrong Req Type : ", reqMsg.header.reqType);
		return nullptr;
	}

	auto& func = createFuncs[reqMsg.header.reqType];

	if (!func)
	{
		LOG_ERR("Can't Find CreateJob Func : ", reqMsg.header.reqType);
		return nullptr;
	}

	Job* pRet = func();

	std::string_view payload(reqMsg.payload, reqMsg.payloadSize);

	if (pRet == nullptr || !pRet->Parse(payload))
	{
		LOG_ERR("[", reqMsg.header.reqType, "] Failed to Parse.");

		if (pRet != nullptr)
		{
			DeallocateJob(pRet);
		}

		return nullptr;
	}

	pRet->m_ClientIndex = uUserIndex_;

	return pRet;
}

void JobFactory::DeallocateJob(Job* pJob_)
{
	if (pJob_ == nullptr)
	{
		return;
	}

	pJob_->~Job();

	m_pool.Deallocate(reinterpret_cast<void*>(pJob_));
	return;
}

EchoJob::EchoJob(DIStruct* stDI_) : Job(stDI_), m_param({})
{

}

bool EchoJob::Parse(std::string_view& sv_)
{
	if (sv_.size() > sizeof(m_param))
	{
		LOG_ERR("Too Long.");

		return false;
	}

	CopyMemory(&m_param, sv_.data(), sv_.size());

	return true;
}

bool EchoJob::Execute()
{
	PacketData* pPacket = PacketPool::Instance().Allocate();

	if (pPacket == nullptr)
	{
		LOG_ERR("Failed to Allocate Packet from Pool.");
		return false;
	}

	if (!pPacket->Init(InfoCode::REQ_SUCCESS, m_ReqNo, m_param))
	{
		LOG_DEBUG("1");
		PacketPool::Instance().Deallocate(pPacket);
		return false;
	}

	if (m_pPacket != nullptr)
	{
		LOG_DEBUG("2");
		PacketPool::Instance().Deallocate(m_pPacket);
	}

	m_pPacket = pPacket;

	return true;
}

bool EchoJob::SendRes()
{
	if (m_stDI->sendFunc(m_ClientIndex, m_pPacket, SendProtocol::TCP))
	{
		m_pPacket = nullptr;
		return true;
	}

	return false;
}

ReqTokenJob::ReqTokenJob(DIStruct* stDI_) : Job(stDI_)
{

}

bool ReqTokenJob::Parse(std::string_view& sv_)
{
	return true;
}

bool ReqTokenJob::Execute()
{
	uint64_t token = m_stDI->getTokenFunc(m_ClientIndex);

	PacketData* pPacket = PacketPool::Instance().Allocate();

	if (pPacket == nullptr)
	{
		return false;
	}

	ReqTokenRes res{ token };

	if (!pPacket->Init(InfoCode::REQ_SUCCESS, m_ReqNo, res))
	{
		PacketPool::Instance().Deallocate(pPacket);
		return false;
	}

	if (m_pPacket != nullptr)
	{
		PacketPool::Instance().Deallocate(m_pPacket);
	}

	m_pPacket = pPacket;

	return true;
}

bool ReqTokenJob::SendRes()
{
	if (m_stDI->sendFunc(m_ClientIndex, m_pPacket, SendProtocol::TCP))
	{
		m_pPacket = nullptr;
		return true;
	}

	return false;
}

UDPEchoJob::UDPEchoJob(DIStruct* stDI_) : Job(stDI_), m_param({})
{

}

bool UDPEchoJob::Parse(std::string_view& sv_)
{
	if (sv_.size() > sizeof(m_param))
	{
		LOG_ERR("Too Big.");
		return false;
	}

	ZeroMemory(&m_param, sizeof(m_param));

	CopyMemory(&m_param, sv_.data(), sv_.size());

	return true;
}

bool UDPEchoJob::Execute()
{
	PacketData* pPacket = PacketPool::Instance().Allocate();

	if (pPacket == nullptr)
	{
		return false;
	}

	if (!pPacket->Init(InfoCode::REQ_SUCCESS, m_ReqNo, m_param))
	{
		PacketPool::Instance().Deallocate(pPacket);

		return false;
	}

	if (m_pPacket != nullptr)
	{
		PacketPool::Instance().Deallocate(m_pPacket);
	}

	m_pPacket = pPacket;

	return true;
}

bool UDPEchoJob::SendRes()
{
	if (m_stDI->sendFunc(m_ClientIndex, m_pPacket, SendProtocol::UDP))
	{
		m_pPacket = nullptr;
		return true;
	}

	return false;
}