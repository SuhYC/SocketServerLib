#include "SlideBuffer.hpp"

SlideBuffer::SlideBuffer() : m_pData(nullptr), m_Size(0), m_Capacity(0)
{

}

SlideBuffer::~SlideBuffer()
{
	if (m_pData != nullptr)
	{
		delete[] m_pData;
	}
}

void SlideBuffer::Clear()
{
	m_Size = 0;
}

bool SlideBuffer::Init(const uint32_t capacity_)
{
	if (m_pData != nullptr)
	{
		delete[] m_pData;
	}

	// �����ʹ� �ʱ�ȭ�ȴ�.
	m_Size = 0;

	try
	{
		m_pData = new char[capacity_];
		m_Capacity = capacity_;
	}
	catch (std::bad_alloc&)
	{
		std::cerr << "SlideBuffer::Init : �޸� �Ҵ� ����";
		m_pData = nullptr;
		m_Capacity = 0;
		return false;
	}

	return true;
}

bool SlideBuffer::Enqueue(const char* in_, const uint32_t size_)
{
	if (in_ == nullptr)
	{
		return false;
	}

	if (size_ + m_Size > m_Capacity)
	{
		return false;
	}

	memcpy(m_pData + m_Size, in_, size_);
	m_Size += size_;

	return true;
}

uint32_t SlideBuffer::Dequeue(char* out_, const uint32_t maxSize_)
{
	if (m_Size == 0)
	{
		return 0;
	}

	// �ϴ� ���ۿ� ��� ��� �����͸� ��ȯ �õ�
	uint32_t deqSize = m_Size;

	// ��¹����� ũ�Ⱑ �� �۴ٸ� ��¹����� ũ�⸸ŭ �õ�
	if (deqSize > maxSize_)
	{
		deqSize = maxSize_;
	}

	memcpy(out_, m_pData, deqSize);
	memcpy(m_pData, m_pData + deqSize, m_Size - deqSize);
	m_Size -= deqSize;

	return deqSize;
}

bool SlideBuffer::Pop(const uint32_t size_)
{
	if (size_ > m_Size)
	{
		return false;
	}

	memcpy(m_pData, m_pData + size_, m_Size - size_);
	m_Size -= size_;

	return true;
}

uint32_t SlideBuffer::Peek() const
{
	if (m_Size < 5 || m_Capacity < 5)
	{
		return 0;
	}

	uint32_t ret;

	memcpy(&ret, m_pData, sizeof(ret));

	// ret���� ª�� �޽����� ����ִ°� �Ǵ��ؾ��ұ�? - �ϴ� �������� �޽����� �־��ٰ� �Ǵ��ϰ� �������� �ʴ´�.

	return ret;
}

uint32_t SlideBuffer::GetSize() const
{
	return m_Size;
}

bool SlideBuffer::IsEmpty() const
{
	return m_Size == 0;
}

uint32_t SlideBuffer::GetCapacity() const
{
	return m_Capacity;
}

char* SlideBuffer::GetBuf()
{
	return m_pData;
}