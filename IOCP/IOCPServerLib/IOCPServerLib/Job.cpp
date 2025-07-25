#include "Job.hpp"

std::function<InfoCode(const uint32_t, PacketData*)> Job::SendMsgFunc;

Job::Job(uint32_t userindex_, uint32_t reqNo_, DIStruct* pStDI_)
	: m_UserIndex(userindex_), m_ReqNo(reqNo_), m_pStDI(pStDI_)
{

}

EchoJob::EchoJob(uint32_t userindex_, uint32_t reqNo_, DIStruct* pStDI_) 
	: Job(userindex_, reqNo_, pStDI_)
{
	ZeroMemory(&m_param, sizeof(EchoParameter));
}

bool EchoJob::Parse(const std::string_view& param_)
{
	if (param_.size() > sizeof(EchoParameter))
	{
		return false;
	}

	ZeroMemory(&m_param, sizeof(EchoParameter));
	CopyMemory(&m_param, param_.data(), param_.size());

	return true;
}

InfoCode EchoJob::Execute()
{
	// L7 User객체 가져오기
	// User* pUser = m_pStDI->um->GetUser(m_UserIndex);

	// Zone 객체 가져오기
	// Zone* pZone = m_pStDI->zm->GetZone(pUser->GetZoneIndex());

	if (m_pStDI == nullptr || m_pStDI->pp == nullptr)
	{
		LOG_ERR("Cant Find PacketPool.");
		return InfoCode::OTHER_ERR;
	}

	PacketData* pPacket = m_pStDI->pp->Allocate();

	if (pPacket == nullptr)
	{
		LOG_ERR("Packet nullptr.");
		return InfoCode::OTHER_ERR;
	}

	pPacket->Init(InfoCode::REQ_SUCCESS, m_ReqNo, m_param);

	InfoCode eRet = SendResultMsg(m_UserIndex, pPacket);

	m_pStDI->pp->Deallocate(pPacket);

	return eRet;
}