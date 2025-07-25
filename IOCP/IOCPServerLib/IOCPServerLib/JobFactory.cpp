#include "JobFactory.hpp"

void JobFactory::Init(DIStruct&& param_)
{
	m_DIStruct = param_;

	createFuncs.resize(static_cast<size_t>(ReqType::LAST) + 1);

	// -- 새로 생성된 작업객체가 있으면 추가해야함
	Register<EchoJob>(ReqType::ECHO);

}

Job* JobFactory::CreateJob(uint16_t userindex_, std::string_view& req_)
{
	if (req_.size() < sizeof(ReqHeader))
	{
		LOG_ERR("Not Enough Size.");
		return nullptr;
	}

	ReqHeader header{};
	CopyMemory(&header, req_.data(), sizeof(ReqHeader));

	auto& func = createFuncs[header.reqType];

	if (!func)
	{
		LOG_ERR("Invalid ReqType. REQTYPE:", header.reqType);
		return nullptr;
	}

	Job* pRet = func(userindex_, header.reqNo);
	std::string_view payload(req_.data() + sizeof(ReqHeader), req_.size() - sizeof(ReqHeader));

	if (pRet == nullptr || !pRet->Parse(payload))
	{
		LOG_ERR("Failed to Create Job[", header.reqType, "].");
		return nullptr;
	}

	return pRet;
}

void JobFactory::DeallocateJob(Job* pJob_)
{
	m_Pool.Deallocate(pJob_);

	return;
}