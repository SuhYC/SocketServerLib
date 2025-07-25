#pragma once

#include "PacketData.hpp"
#include "Define.hpp"
#include <concurrent_queue.h>
#include "LogManager.hpp"

class PacketPool final
{
public:
	PacketPool();
	~PacketPool();

	PacketData* Allocate();
	void Deallocate(PacketData* pPacket_);

private:
	Concurrency::concurrent_queue<PacketData*> m_FreeList;
};