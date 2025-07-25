#pragma once

#include <atomic>

/// <summary>
/// �翬�� �ߺ�ȣ��� ���� ������� ����å�� ������ �ʾҴ�.
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