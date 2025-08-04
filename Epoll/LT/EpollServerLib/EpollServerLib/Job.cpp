#include "Job.hpp"

std::function<InfoCode(const int fd_, PacketData* pPacket_)> Job::SendMsgFunc;

Job::Job(const int fd_, uint32_t uReqNo_) : m_Socket(fd_), m_ReqNo(uReqNo_)
{

}

EchoJob::EchoJob(const int fd_, uint32_t uReqNo_) : Job(fd_, uReqNo_)
{

}

bool EchoJob::Parse(const std::string_view& sv_)
{
	if (sv_.size() != sizeof(EchoParameter))
	{
		return false;
	}

	memcpy(&m_param, sv_.data(), sv_.size());

	return true;
}

InfoCode EchoJob::Execute()
{
	PacketData* pPacket = new PacketData();

	pPacket->Init(InfoCode::REQ_SUCCESS, m_ReqNo, m_param);

	InfoCode eRet = SendResultMsg(m_Socket, pPacket);

	delete pPacket;

	return eRet;
}