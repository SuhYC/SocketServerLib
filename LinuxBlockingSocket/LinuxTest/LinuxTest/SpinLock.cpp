#include "SpinLock.hpp"

SpinLock::SpinLock()
{
	flag.store(true);
}

void SpinLock::lock()
{
	while (!try_lock())
	{

	}
	return;
}

void SpinLock::unlock()
{
	flag.store(true);
	return;
}

bool SpinLock::try_lock()
{
	bool expected = true;
	return flag.compare_exchange_weak(expected, false);
}

SpinLockGuard::SpinLockGuard(SpinLock& lock_) : m_lock(lock_)
{
	m_lock.lock();
}

SpinLockGuard::~SpinLockGuard()
{
	m_lock.unlock();
}