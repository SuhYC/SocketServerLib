#include "JobFactory.hpp"

void JobFactory::Init()
{
	createFuncs.resize(static_cast<uint32_t>(ReqType::LAST) + 1);

	Register<EchoJob>(ReqType::ECHO);
}

Job* JobFactory::CreateJob(const SOCKET clientSocket_, std::string_view& sv_)
{
	if (sv_.size() < sizeof(ReqHeader))
	{
		std::cerr << "JobFactory::CreateJob : Not Enough Size.\n";
		return nullptr;
	}

	ReqHeader header{};
	CopyMemory(&header, sv_.data(), sizeof(ReqHeader));

	auto& func = createFuncs[header.reqType];

	if (!func)
	{
		std::cerr << "JobFactory::CreateJob : Invalid ReqType. REQTYPE:" << header.reqType << "\n";
		return nullptr;
	}

	Job* pRet = func(clientSocket_, header.reqNo);
	std::string_view payload(sv_.data() + sizeof(ReqHeader), sv_.size() - sizeof(ReqHeader));

	if (pRet == nullptr || !pRet->Parse(payload))
	{
		std::cerr << "JobFactory::CreateJob : Failed to Create Job[" << header.reqType << "].\n";
		return nullptr;
	}

	return pRet;
}