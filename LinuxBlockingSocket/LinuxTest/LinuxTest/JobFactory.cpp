#include "JobFactory.hpp"

void JobFactory::Init()
{
	createFuncs.resize(static_cast<size_t>(ReqType::LAST) + 1);

	// -- 새로 생성된 작업객체가 있으면 추가해야함
	Register<EchoJob>(ReqType::ECHO);

}

Job* JobFactory::CreateJob(uint16_t userindex_, std::string_view& req_)
{
	if (req_.size() < sizeof(ReqHeader))
	{
		std::cerr << "JobFactory::CreateJob : Not Enough Size.\n";
		return nullptr;
	}

	ReqHeader header{};
	memcpy(&header, req_.data(), sizeof(ReqHeader));

	auto& func = createFuncs[header.reqType];

	if (!func)
	{
		std::cerr << "JobFactory::CreateJob : Invalid ReqType. REQTYPE:" <<  header.reqType << '\n';
		return nullptr;
	}

	Job* pRet = func(userindex_, header.reqNo);
	std::string_view payload(req_.data() + sizeof(ReqHeader), req_.size() - sizeof(ReqHeader));

	if (pRet == nullptr || !pRet->Parse(payload))
	{
		std::cerr << "JobFactory::CreateJob : Failed to Create Job[" << header.reqType << "].\n";
		return nullptr;
	}

	return pRet;
}

void JobFactory::DeallocateJob(Job* pJob_)
{
	if (pJob_ == nullptr)
	{
		return;
	}

	delete pJob_;

	return;
}