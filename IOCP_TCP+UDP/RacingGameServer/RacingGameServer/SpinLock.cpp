#include "SpinLock.hpp"

SpinLock::SpinLock()
{
	flag.store(true);
}

void SpinLock::lock()
{
	bool expected = true;
	while (!flag.compare_exchange_weak(expected, false))
	{
		expected = true;
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
	return flag.compare_exchange_strong(expected, false);
}

SpinLockGuard::SpinLockGuard(SpinLock& lock_) : m_lock(lock_), locked(true)
{
	m_lock.lock();
}

SpinLockGuard::SpinLockGuard(SpinLock& lock_, const try_to_lock_t&) : m_lock(lock_), locked(false)
{
	if (m_lock.try_lock())
	{
		locked = true;
	}
}

SpinLockGuard::~SpinLockGuard()
{
	if (locked)
	{
		m_lock.unlock();
	}
}

bool SpinLockGuard::owns_lock() const
{
	return locked;
}

void SpinLockGuard::unlock()
{
	if (locked)
	{
		m_lock.unlock();
		locked = false;
	}
	return;
}