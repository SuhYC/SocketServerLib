#pragma once

#include "Job.hpp"
#include "concurrent_queue.h"
#include "LogManager.hpp"
#include <typeinfo>

/// <summary>
/// 선언한 모든 타입의 Job 파생클래스를 담을 것.
/// 해당 union을 바탕으로 가장 크기가 큰 Job을 확인하는 로직.
/// </summary>
union Jobs
{
	// ----- 작업이 추가될 때마다 추가해주어야함
	EchoJob j1;
};

const uint32_t MAX_JOB_SIZE = sizeof(Jobs);
const uint32_t MEMORY_POOL_SIZE = 10;

class JobMemoryPool final
{
public:
	JobMemoryPool();
	~JobMemoryPool();

	template<typename T>
	typename std::enable_if<std::is_base_of<Job, T>::value, Job*>::type
	Allocate(uint16_t userindex_, uint32_t reqNo_, DIStruct& stDI_)
	{
		if (sizeof(T) > MAX_JOB_SIZE)
		{
			LOG_ERR("Check union Jobs. JobType : ", typeid(T).name());
			return nullptr;
		}

		void* memory = nullptr;

		if (!FreeList.try_pop(memory))
		{
			memory = ::operator new(MAX_JOB_SIZE, std::nothrow);
			if (memory == nullptr)
			{
				LOG_ERR("Not Enough Mem.");
				return nullptr;
			}
		}

		Job* pRet = new (memory) T(userindex_, reqNo_, &stDI_);

		return pRet;
	}

	void Deallocate(Job* job_);

private:
	Concurrency::concurrent_queue<void*> FreeList;

};