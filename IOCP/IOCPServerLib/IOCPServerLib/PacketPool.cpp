#include "PacketPool.hpp"

PacketPool::PacketPool()
{
	for (int i = 0; i < INIT_PACKET_COUNT; i++)
	{
		PacketData* pPacket = new(std::nothrow) PacketData();

		if (pPacket == nullptr)
		{
			LOG_ERR("Failed to Allocate Packet Block. ", i, "Blocks Created.");
			break;
		}
		m_FreeList.push(pPacket);
	}
}

PacketPool::~PacketPool()
{
	PacketData* pPacket = nullptr;
	while (!m_FreeList.try_pop(pPacket))
	{
		delete pPacket;
	}
}

PacketData* PacketPool::Allocate()
{
	PacketData* pRet = nullptr;
	if (!m_FreeList.try_pop(pRet))
	{
		pRet = new(std::nothrow) PacketData();

		if (pRet == nullptr)
		{
			LOG_ERR("Not Enough Mem.");
		}
		return pRet;
	}
	return pRet;
}

void PacketPool::Deallocate(PacketData* pPacket_)
{
	pPacket_->Clear();

	m_FreeList.push(pPacket_);

	return;
}