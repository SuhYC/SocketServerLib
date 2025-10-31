#include "MemoryPool.hpp"

MemoryPool::MemoryPool(uint64_t uSize_, uint32_t uInitBlock_) : m_dataSize(uSize_)
{
	void* block = nullptr;

	for (uint32_t i = 0; i < uInitBlock_; i++)
	{
		block = ::operator new(m_dataSize, std::nothrow);

		if (block != nullptr)
		{
			m_data.push(block);
		}
		else
		{
			LOG_ERR("Failed to Allocate Memory Block.");
		}
	}
}


MemoryPool::~MemoryPool()
{
	void* block = nullptr;

	while (m_data.try_pop(block))
	{
		if (block != nullptr)
		{
			::operator delete(block);
		}
	}
}

void* MemoryPool::Allocate()
{
	void* pRet = nullptr;
	m_data.try_pop(pRet);

	if (pRet == nullptr)
	{
		pRet = ::operator new(m_dataSize, std::nothrow);

		if (pRet == nullptr)
		{
			LOG_ERR("Failed to Allocate Memory Block.");
			return nullptr;
		}
	}

	return pRet;
}


void MemoryPool::Deallocate(void* block_)
{
	if (block_ != nullptr)
	{
		m_data.push(block_);
	}
	return;
}