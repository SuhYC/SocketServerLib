#include "JobFactory.hpp"

void JobFactory::Init()
{
	createFuncs.resize(static_cast<uint32_t>(ReqType::LAST) + 1);

	Register<EchoJob>(ReqType::ECHO);
}

Job* JobFactory::CreateJob(const int fd_, std::string_view& sv_)
{
	if (sv_.size() < sizeof(ReqHeader))
	{
		std::cerr << "JobFactory::CreateJob : Not Enough Size.\n";
		return nullptr;
	}

	ReqHeader header{};
	memcpy(&header, sv_.data(), sizeof(ReqHeader));

	if (header.reqType > static_cast<int32_t>(ReqType::LAST) || header.reqType < 0)
	{
		std::cerr << "JobFactory::CreateJob : Invalid ReqType. REQTYPE:" << header.reqType << "\n";
		return nullptr;
	}

	auto& func = createFuncs[header.reqType];

	if (!func)
	{
		std::cerr << "JobFactory::CreateJob : Invalid ReqType. REQTYPE:" << header.reqType << "\n";
		return nullptr;
	}

	Job* pRet = func(fd_, header.reqNo);
	std::string_view payload(sv_.data() + sizeof(ReqHeader), sv_.size() - sizeof(ReqHeader));

	if (pRet == nullptr || !pRet->Parse(payload))
	{
		std::cerr << "JobFactory::CreateJob : Failed to Create Job[" << header.reqType << "].\n";
		return nullptr;
	}

	return pRet;
}