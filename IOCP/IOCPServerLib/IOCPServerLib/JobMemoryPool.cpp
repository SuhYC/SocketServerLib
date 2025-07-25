#include "JobMemoryPool.hpp"

JobMemoryPool::JobMemoryPool()
{
	void* block = nullptr;

	for (int i = 0; i < MEMORY_POOL_SIZE; i++)
	{
		block = ::operator new(MAX_JOB_SIZE, std::nothrow);
		if (block != nullptr)
		{
			FreeList.push(block);
		}
	}
}

JobMemoryPool::~JobMemoryPool()
{
	void* block = nullptr;
	while (!FreeList.empty())
	{
		if (FreeList.try_pop(block))
		{
			if (block != nullptr)
			{
				::operator delete(block);
			}
		}
	}
}

void JobMemoryPool::Deallocate(Job* job_)
{
	if (job_ == nullptr)
	{
		return;
	}

	job_->~Job();

	FreeList.push(job_);
	return;
}