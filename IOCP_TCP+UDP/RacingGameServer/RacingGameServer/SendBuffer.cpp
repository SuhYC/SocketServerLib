#include "SendBuffer.hpp"

SendBuffer::SendBuffer() : m_Capacity(0), m_ReadOffset(0), m_WriteOffset(0), m_Data(nullptr)
{

}

SendBuffer::~SendBuffer()
{
	if (m_Data != nullptr)
	{
		if (!IsEmpty())
		{
			for (uint32_t ptr = m_ReadOffset; ptr != m_WriteOffset; ptr = (ptr + 1) % m_Capacity)
			{
				if (m_Data[ptr] != nullptr)
				{
					delete m_Data[ptr];
					m_Data[ptr] = nullptr;
				}
			}
		}

		delete[] m_Data;
	}
}

bool SendBuffer::Allocate(uint32_t capacity_)
{
	if (capacity_ == 0)
	{
		return false;
	}

	if (capacity_ == m_Capacity)
	{
		return true;
	}

	PacketData** pNewData = new(std::nothrow) PacketData*[capacity_];

	if (pNewData == nullptr)
	{
		return false;
	}

	for (uint32_t ptr = 0; ptr < capacity_; ptr++)
	{
		pNewData[ptr] = nullptr;
	}

	// 누수 방지 (기존 데이터 안살리고 폐기.)
	if (m_Capacity != 0 && !IsEmpty())
	{
		for (uint32_t ptr = m_ReadOffset; ptr != m_WriteOffset; ptr = (ptr + 1) % m_Capacity)
		{
			if (m_Data[ptr] != nullptr)
			{
				delete m_Data[ptr];
				m_Data[ptr] = nullptr;
			}
		}
	}

	m_Data = pNewData;
	m_Capacity = capacity_;

	return true;
}

bool SendBuffer::Push(PacketData* pPacket_)
{
	if (IsFull() || pPacket_ == nullptr)
	{
		return false;
	}

	m_Data[m_WriteOffset] = pPacket_;
	
	IncrementWriteOffset();

	return true;
}

void SendBuffer::Pop(uint32_t ioSize_)
{
	uint32_t local = ioSize_;

	while (local > 0)
	{
		if (m_Data[m_ReadOffset] == nullptr)
		{
			LOG_ERR("송신큐에 데이터가 없습니다. 송신오류. 남은 바이트 : ", local);
			return;
		}

		uint32_t size = m_Data[m_ReadOffset]->GetSize();

		if (local >= size)
		{
			local -= size;

			PacketPool::Instance().Deallocate(m_Data[m_ReadOffset]);
			m_Data[m_ReadOffset] = nullptr;

			IncrementReadOffset();

			continue;
		}

		m_Data[m_ReadOffset]->Pop(local);
		break;
	}

	return;
}

uint32_t SendBuffer::GetWSABUF(WSABUF* out_, uint32_t bufcnt_) const
{
	if (bufcnt_ == 0 || IsEmpty() || out_ == nullptr)
	{
		return 0;
	}

	uint32_t uRet = 0;
	uint32_t uPacketSize = 0;

	for (uint32_t ptr = m_ReadOffset; ptr != m_WriteOffset; ptr = (ptr + 1) % m_Capacity)
	{
		if (uRet >= bufcnt_ || uPacketSize + m_Data[ptr]->GetSize() > MAX_TCP_SEGMENT_SIZE)
		{
			break;
		}

		out_[uRet].buf = m_Data[ptr]->GetData();
		out_[uRet].len = m_Data[ptr]->GetSize();
		uRet++;
		uPacketSize += m_Data[ptr]->GetSize();
	}

	return uRet;
}

bool SendBuffer::IsEmpty() const
{
	return m_ReadOffset == m_WriteOffset;
}

bool SendBuffer::IsFull() const
{
	return (m_WriteOffset + 1) % m_Capacity == m_ReadOffset;
}

void SendBuffer::IncrementReadOffset()
{
	m_ReadOffset = (m_ReadOffset + 1) % m_Capacity;
	return;
}

void SendBuffer::IncrementWriteOffset()
{
	m_WriteOffset = (m_WriteOffset + 1) % m_Capacity;
}