#pragma once

#include <concurrent_queue.h>
#include <stdint.h>

#include "Define.hpp"

class MemoryPool
{
public:
	MemoryPool() = delete;
	MemoryPool(uint64_t uSize_, uint32_t uInitBlock_);
	~MemoryPool();

	void* Allocate();

	void Deallocate(void* block_);

private:
	Concurrency::concurrent_queue<void*> m_data;
	const uint64_t m_dataSize;
};