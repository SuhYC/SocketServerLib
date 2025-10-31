#include "PacketPool.hpp"

PacketPool::PacketPool()
{
	PacketData* pPacket = nullptr;

	for (int i = 0; i < PACKETCOUNT; i++)
	{
		pPacket = new(std::nothrow) PacketData();
		if (pPacket == nullptr)
		{
			continue;
		}

		if (!pPacket->Allocate(PACKET_SIZE))
		{
			delete pPacket;
			continue;
		}
		m_FreeList.push(pPacket);
	}
}

PacketPool::~PacketPool()
{
	PacketData* pPacket = nullptr;

	while (m_FreeList.try_pop(pPacket))
	{
		if (pPacket != nullptr)
		{
			delete pPacket;
		}
	}
}

PacketPool& PacketPool::Instance()
{
	static PacketPool instance;
	return instance;
}

PacketData* PacketPool::Allocate()
{
	PacketData* pRet = nullptr;

	if (!m_FreeList.try_pop(pRet))
	{
		pRet = new(std::nothrow) PacketData();

		if (pRet == nullptr)
		{
			return nullptr;
		}

		if (!pRet->Allocate(PACKET_SIZE))
		{
			delete pRet;
			return nullptr;
		}
	}
	return pRet;
}

void PacketPool::Deallocate(PacketData* pPacket_)
{
	if (pPacket_ == nullptr)
	{
		return;
	}

	pPacket_->Clear();

	m_FreeList.push(pPacket_);
	return;
}