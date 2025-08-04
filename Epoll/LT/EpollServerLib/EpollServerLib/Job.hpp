#pragma once

#include "Define.hpp"
#include <string_view>
#include <functional>
#include "PacketData.hpp"

class Job
{
public:
	Job(const int fd_, uint32_t uReqNo);
	virtual ~Job() = default;

	virtual bool Parse(const std::string_view& sv_) = 0;
	virtual InfoCode Execute() = 0;

	static std::function<InfoCode(const int fd_, PacketData* pPacket_)> SendMsgFunc;

protected:
	int m_Socket;
	uint32_t m_ReqNo;

	InfoCode SendResultMsg(const int fd_, PacketData* pPacket_)
	{
		if (!SendMsgFunc)
		{
			std::cerr << "Job::SendResultMsg : funcptr nullptr.\n";
			return InfoCode::NULLPTR_ON_FUNCPTR;
		}

		if (pPacket_ == nullptr)
		{
			std::cerr << "Job::SendResultMsg : Packet nullptr.\n";
			return InfoCode::OTHER_ERR;
		}

		return SendMsgFunc(fd_, pPacket_);
	}
};

class EchoJob : public Job
{
public:
	EchoJob(const int fd_, uint32_t uReqNo_);

	bool Parse(const std::string_view& sv_) override;
	InfoCode Execute() override;

private:
	EchoParameter m_param;
};