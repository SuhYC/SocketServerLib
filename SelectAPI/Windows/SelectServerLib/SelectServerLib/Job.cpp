#include "Job.hpp"

std::function<InfoCode(const SOCKET clientSocket_, PacketData* pPacket_)> Job::SendMsgFunc;

Job::Job(const SOCKET clientSocket_, uint32_t uReqNo_) : m_Socket(clientSocket_), m_ReqNo(uReqNo_)
{

}

EchoJob::EchoJob(const SOCKET clientSocket_, uint32_t uReqNo_) : Job(clientSocket_, uReqNo_)
{

}

bool EchoJob::Parse(const std::string_view& sv_)
{
	if (sv_.size() != sizeof(EchoParameter))
	{
		return false;
	}

	CopyMemory(&m_param, sv_.data(), sv_.size());

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