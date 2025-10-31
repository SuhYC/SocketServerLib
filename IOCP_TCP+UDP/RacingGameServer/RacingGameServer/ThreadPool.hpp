#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <concurrent_queue.h>
#include <functional>
#include "Define.hpp"
#include "Database.hpp"

class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();

	bool Start(const uint16_t uThreadCount_);
	bool Stop();

	bool Resize(const uint16_t uThreadCount_);

	bool Enqueue(std::function<InfoCode()>&& job);

private:
	void WorkerThread(const uint16_t idx_);

	Concurrency::concurrent_queue<std::function<InfoCode()>> q;

	std::vector<std::thread> ths;

	// 의도한 스레드 수
	uint16_t m_thControll;

	std::atomic_int atm;
};

/// <summary>
/// 하나의 스레드로 작동합니다.
/// 복수의 스레드로 Concurrency::concurrent_priority_queue와 사용하면 현재 작업할 것이 없어도 try_pop을 계속 반복해야함.
/// CPU자원을 너무 먹을 것 같으니 일반우선순위큐 + 단일스레드로 동작.
/// cv 기반으로 작성하자.
/// 우선순위큐에 시간과 함께 넣어서 사용하며
/// 현재처리할 작업이 없다면 wait_until로 멈춰둘 수 있다.
/// 
/// </summary>
class TimeBasedThreadPool
{
public:
	TimeBasedThreadPool();
	~TimeBasedThreadPool();

	bool Start();
	bool Stop();

	bool Enqueue(const std::chrono::steady_clock::time_point tp_, std::function<InfoCode()>&& job_);
	bool Enqueue(const std::chrono::seconds delay_, std::function<InfoCode()>&& job_);

	void SetRetryDelay(const std::chrono::milliseconds msdelay_);

private:
	void WorkerThread();

	using JobPair = std::pair<std::chrono::steady_clock::time_point, std::function<InfoCode()>>;
	struct JobCmp
	{
		bool operator()(const JobPair& a, const JobPair& b)
		{
			return a.first > b.first;
		}
	};
	bool Requeue(JobPair&& pair_);

	std::priority_queue<JobPair, std::vector<JobPair>, JobCmp> q;

	std::thread th;

	std::condition_variable cv;
	std::mutex mu;

	std::chrono::milliseconds m_retry_delay;

	bool m_IsRun;
};