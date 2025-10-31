#pragma once

#include "Define.hpp"
#include <concurrent_queue.h>
#include <stdint.h>
#include "LogManager.hpp"

template<typename T>
class ObjectPool
{
public:
	ObjectPool()
	{

	}

	ObjectPool(uint32_t uMaxData_)
	{
		for (uint32_t i = 0; i < uMaxData_; i++)
		{
			T* pData = new(std::nothrow) T();
			if (pData != nullptr)
			{
				m_Data.push(pData);
			}
			else
			{
				LOG_ERR("Failed to Allocate");
			}
		}
	}
	~ObjectPool()
	{
		T* pRet{};

		while (m_Data.try_pop(pRet))
		{
			if (pRet != nullptr)
			{
				delete pRet;
			}
		}
	}

	T* Allocate()
	{
		T* pRet{};

		if (m_Data.try_pop(pRet))
		{
			return pRet;
		}

		pRet = new(std::nothrow) T();

		if (pRet == nullptr)
		{
			LOG_ERR("Failed to Allocate ");
		}

		return pRet;
	}
	void Deallocate(T* data_)
	{
		m_Data.push(data_);

		return;
	}

private:
	Concurrency::concurrent_queue<T*> m_Data;
};