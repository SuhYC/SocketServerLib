#pragma once

#include <atomic>

/// <summary>
/// 당연히 중복호출로 인한 데드락은 방지책을 만들지 않았다.
/// </summary>
class SpinLock final
{
public:
	SpinLock();

	void lock();

	void unlock();

private:
	bool try_lock();

	std::atomic_bool flag;
};

class SpinLockGuard final
{
public:
	SpinLockGuard() = delete;
	SpinLockGuard(SpinLock& lock_);

	~SpinLockGuard();

	SpinLockGuard(const SpinLockGuard& other_) = delete;
	SpinLockGuard(SpinLockGuard&& rhs_) = delete;

	SpinLockGuard& operator=(const SpinLockGuard& other_) = delete;
	SpinLockGuard& operator=(SpinLockGuard&& rhs_) = delete;

private:
	SpinLock& m_lock;
};