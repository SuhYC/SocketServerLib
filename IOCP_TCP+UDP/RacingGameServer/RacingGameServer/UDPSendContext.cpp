#include "UDPSendContext.hpp"

bool stUDPSendContext::Init(uint32_t uClientIndex_, sockaddr_in& addr, PacketData* pPacket_)
{
	if (pPacket_ == nullptr)
	{
		return false;
	}
	pPacket = pPacket_;

	ZeroMemory(&m_overlapped, sizeof(m_overlapped));

	m_overlapped.m_eOperation = eIOOperation::UDPSEND;
	m_overlapped.m_userIndex = uClientIndex_;
	m_overlapped.m_wsaBuf->buf = pPacket_->GetData();
	m_overlapped.m_wsaBuf->len = pPacket_->GetSize();
	
	CopyMemory(&m_overlapped.clientAddr, &addr, sizeof(sockaddr_in));
	m_overlapped.addrlen = sizeof(m_overlapped.clientAddr);
}
