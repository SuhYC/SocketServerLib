#pragma once

#include "PacketData.hpp"

PacketData::PacketData() : m_pData(new(std::nothrow) char[PACKET_SIZE]), m_Size(0), m_Capacity(PACKET_SIZE)
{
	if (m_pData == nullptr)
	{
		std::cerr << "PacketData::Constructor : Failed to Allocate.\n";
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
	m_Size = 0;

	return;
}

void PacketData::Free()
{
	if (m_pData == nullptr)
	{
		return;
	}

	delete[] m_pData;

	m_pData = nullptr;

	return;
}

bool PacketData::Allocate(uint32_t size_)
{
	if (m_pData != nullptr)
	{
		delete[] m_pData;
	}

	m_pData = new(std::nothrow) char[size_];

	if (m_pData == nullptr)
	{
		std::cerr << "PacketData::Allocate : Failed to Allocate.\n";
		m_Capacity = 0;
		m_Size = 0;
		return false;
	}

	m_Capacity = size_;
	m_Size = 0;

	return true;
}

bool PacketData::Init(InfoCode infoCode_, uint32_t reqNo_, std::string& strData_)
{
	uint32_t size = strData_.length();
	uint32_t totalSize = size + sizeof(InfoHeader);

	if (totalSize > PACKET_SIZE)
	{
		return false;
	}

	int32_t nInfoCode = static_cast<int32_t>(infoCode_);
	m_Size = 0;

	// totalSize
	CopyMemory(m_pData + m_Size, &totalSize, sizeof(uint32_t));
	m_Size += sizeof(uint32_t);

	// infoCode
	CopyMemory(m_pData + m_Size, &nInfoCode, sizeof(int32_t));
	m_Size += sizeof(int32_t);

	// reqNo
	CopyMemory(m_pData + m_Size, &reqNo_, sizeof(uint32_t));
	m_Size += sizeof(uint32_t);

	// payload
	if (size > 0)
	{
		CopyMemory(m_pData + m_Size, &strData_[0], size);
		m_Size += size;
	}

	return true;
}

bool PacketData::Init(InfoCode infoCode_, uint32_t reqNo_, const char* pData_, uint32_t dataSize_)
{
	uint32_t totalSize = dataSize_ + sizeof(InfoHeader);

	if (totalSize > PACKET_SIZE)
	{
		return false;
	}

	int32_t nInfoCode = static_cast<int32_t>(infoCode_);
	m_Size = 0;

	// totalSize
	CopyMemory(m_pData + m_Size, &totalSize, sizeof(uint32_t));
	m_Size += sizeof(uint32_t);
	// infoCode
	CopyMemory(m_pData + m_Size, &nInfoCode, sizeof(int32_t));
	m_Size += sizeof(int32_t);

	// reqNo
	CopyMemory(m_pData + m_Size, &reqNo_, sizeof(uint32_t));
	m_Size += sizeof(uint32_t);

	// payload
	if (dataSize_ > 0)
	{
		CopyMemory(m_pData + m_Size, pData_, dataSize_);
		m_Size += dataSize_;
	}

	return true;
}

bool PacketData::Init(InfoCode infoCode_, uint32_t reqNo_)
{
	int32_t nInfoCode = static_cast<int32_t>(infoCode_);
	m_Size = 0;

	// totalSize
	uint32_t size = sizeof(InfoHeader);
	CopyMemory(m_pData + m_Size, &size, sizeof(uint32_t));
	m_Size += sizeof(uint32_t);

	// infoCode
	CopyMemory(m_pData + m_Size, &nInfoCode, sizeof(int32_t));
	m_Size += sizeof(int32_t);

	// reqNo
	CopyMemory(m_pData + m_Size, &reqNo_, sizeof(uint32_t));
	m_Size += sizeof(uint32_t);


	return true;
}

char* PacketData::GetData()
{
	return m_pData;
}

uint32_t PacketData::GetSize() const
{
	return m_Size;
}