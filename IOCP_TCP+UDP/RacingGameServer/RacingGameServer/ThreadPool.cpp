#include "ThreadPool.hpp"

// ----- ThreadPool (lock-free queue + multi-thread)

ThreadPool::ThreadPool() : atm(0), m_thControll(0)
{

}

ThreadPool::~ThreadPool()
{
	Stop();
}

bool ThreadPool::Start(const uint16_t uThreadCount_)
{
	if (uThreadCount_ == 0)
	{
		LOG_ERR("Can't Start with ZERO Thread.");
		return false;
	}

	return Resize(uThreadCount_);
}

bool ThreadPool::Stop()
{
	if (m_thControll == 0)
	{
		return true;
	}

	return Resize(0);
}

bool ThreadPool::Resize(const uint16_t uThreadCount_)
{
	if (m_thControll == uThreadCount_)
	{
		return true;
	}

	uint16_t prev = m_thControll;

	if (prev < uThreadCount_)
	{
		try
		{
			m_thControll = uThreadCount_;

			ths.reserve(uThreadCount_);

			for (uint16_t i = prev; i < uThreadCount_; i++)
			{
				ths.emplace_back([this, i]() {WorkerThread(i); });
			}
		}
		catch (std::bad_alloc&)
		{
			m_thControll = prev;

			LOG_ERR("Failed to Allocate New Threads - bad_alloc");

			return false;
		}
		catch (std::system_error&)
		{
			m_thControll = prev;

			LOG_ERR("Failed to Allocate New Threads - system_error");

			for (uint16_t i = prev; i < ths.size(); i++)
			{
				if (ths[i].joinable())
				{
					ths[i].join();
				}
			}

			return false;
		}

		return true;
	}

	try
	{
		atm.store(m_thControll);
		m_thControll = uThreadCount_;

		std::atomic_notify_all(&atm);

		for (uint16_t i = uThreadCount_; i < prev; i++)
		{
			if (ths[i].joinable())
			{
				ths[i].join();
			}
		}

		ths.resize(uThreadCount_);
	}
	catch (std::out_of_range&)
	{
		LOG_ERR("Do Not Call Resize On Multiple Thread.");

		return false;
	}
	

	return true;
}

bool ThreadPool::Enqueue(std::function<InfoCode()>&& job)
{
	try
	{
		q.push(std::move(job));
	}
	catch (std::bad_alloc&)
	{
		LOG_ERR("Failed to Enqueue Job - bad_alloc.");
		return false;
	}

	atm++;
	std::atomic_notify_one(&atm);

	return true;
}

void ThreadPool::WorkerThread(const uint16_t idx_)
{
	Database::Instance().Connect();

	std::function<InfoCode()> func;
	while (idx_ < m_thControll)
	{
		if (!q.try_pop(func))
		{
			std::atomic_wait(&atm, 0);
			continue;
		}

		atm--;

		if (func)
		{
			InfoCode eRet = func();

			if (eRet == InfoCode::NOT_FINISHED)
			{
				Enqueue(std::move(func));
			}
		}
	}

	Database::Instance().Release();
}

// ----- Time Based ThreadPool (Using PQ + Single Thread)

TimeBasedThreadPool::TimeBasedThreadPool() : m_IsRun(false), m_retry_delay(0)
{

}

TimeBasedThreadPool::~TimeBasedThreadPool()
{
	Stop();
}

bool TimeBasedThreadPool::Start()
{
	if (m_IsRun)
	{
		return false;
	}

	m_IsRun = true;

	try
	{
		th = std::thread([this]() {WorkerThread(); });
	}
	catch (std::system_error&)
	{
		LOG_ERR("Failed to Allocate Thread.");
		m_IsRun = false;
		return false;
	}

	return true;
}

bool TimeBasedThreadPool::Stop()
{
	m_IsRun = false;

	cv.notify_all();

	if (th.joinable())
	{
		th.join();
	}

	return true;
}


bool TimeBasedThreadPool::Enqueue(const std::chrono::steady_clock::time_point tp_, std::function<InfoCode()>&& job_)
{
	std::unique_lock<std::mutex> lock(mu);

	try
	{
		q.emplace(tp_, std::move(job_));
	}
	catch (std::bad_alloc&)
	{
		LOG_ERR("Failed to Enqueue Job - bad_alloc.");
		return false;
	}

	lock.unlock();

	cv.notify_one();

	return true;
}

bool TimeBasedThreadPool::Enqueue(const std::chrono::seconds delay_, std::function<InfoCode()>&& job_)
{
	std::chrono::steady_clock::time_point tp_ = std::chrono::steady_clock::now() + delay_;

	return Enqueue(tp_, std::move(job_));
}

void TimeBasedThreadPool::SetRetryDelay(std::chrono::milliseconds msdelay_)
{
	m_retry_delay = msdelay_;
	return;
}

bool TimeBasedThreadPool::Requeue(JobPair&& pair_)
{
	std::unique_lock<std::mutex> lock(mu);

	try
	{
		q.push(std::move(pair_));
	}
	catch (std::bad_alloc&)
	{
		LOG_ERR("Failed to Enqueue Job - bad_alloc.");
		return false;
	}

	lock.unlock();

	cv.notify_one();

	return true;
}

void TimeBasedThreadPool::WorkerThread()
{
	Database::Instance().Connect();

	JobPair pair;
	while (m_IsRun)
	{
		std::unique_lock<std::mutex> lock(mu);
		cv.wait(lock, [this] { return !q.empty() || !m_IsRun; });

		if (!m_IsRun)
		{
			break;
		}

		auto top = q.top().first;
		auto now = std::chrono::steady_clock::now();

		if (top > now)
		{
			cv.wait_until(lock, top, [this] {
				return (!q.empty() && q.top().first <= std::chrono::steady_clock::now()) || !m_IsRun; 
				});
			continue;
		}

		pair = q.top();
		q.pop();

		lock.unlock();

		if (pair.second)
		{
			InfoCode eRet = pair.second();

			if (eRet == InfoCode::NOT_FINISHED)
			{
				// pair의 시간지점을 재설정해서 넣는것도 가능.
				pair.first += m_retry_delay;
				
				Requeue(std::move(pair));
			}
		}
	}

	Database::Instance().Release();
}