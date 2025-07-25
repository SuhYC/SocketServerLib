#include "Job.hpp"

std::function<InfoCode(const uint32_t, PacketData*)> Job::SendMsgFunc;

Job::Job(uint32_t userindex_, uint32_t reqNo_)
	: m_UserIndex(userindex_), m_ReqNo(reqNo_)
{

}

EchoJob::EchoJob(uint32_t userindex_, uint32_t reqNo_)
	: Job(userindex_, reqNo_)
{
	memset(&m_param, 0, sizeof(EchoParameter));
}

bool EchoJob::Parse(const std::string_view& param_)
{
	if (param_.size() > sizeof(EchoParameter))
	{
		return false;
	}

	memset(&m_param, 0, sizeof(EchoParameter));
	memcpy(&m_param, param_.data(), param_.size());

	return true;
}

InfoCode EchoJob::Execute()
{
	// L7 User객체 가져오기
	// User* pUser = m_pStDI->um->GetUser(m_UserIndex);

	// Zone 객체 가져오기
	// Zone* pZone = m_pStDI->zm->GetZone(pUser->GetZoneIndex());

	PacketData* pPacket = new PacketData();

	if (pPacket == nullptr)
	{
		std::cerr << "EchoJob::Execute : Packet nullptr. \n";
		return InfoCode::OTHER_ERR;
	}

	pPacket->Init(InfoCode::REQ_SUCCESS, m_ReqNo, m_param);

	InfoCode eRet = SendResultMsg(m_UserIndex, pPacket);

	delete pPacket;

	return eRet;
}