#include "PacketData.hpp"

PacketData::PacketData() : m_pData(nullptr), m_uCapacity(0), m_uSize(0)
{
	m_pData = new(std::nothrow) char[MAX_PACKET_SIZE];

	if (m_pData != nullptr)
	{
		m_uCapacity = MAX_PACKET_SIZE;
	}
}

PacketData::~PacketData()
{
	if (m_pData != nullptr)
	{
		delete[] m_pData;
	}
}

void PacketData::Clear()
{
	m_uSize = 0;
}

void PacketData::Free()
{
	if (m_pData != nullptr)
	{
		delete[] m_pData;
		m_pData = nullptr;
	}

	return;
}

char* PacketData::GetData()
{
	return m_pData;
}

uint32_t PacketData::GetSize() const
{
	return m_uSize;
}
