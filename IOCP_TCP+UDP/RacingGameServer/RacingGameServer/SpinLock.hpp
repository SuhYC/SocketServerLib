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

	/// <summary>
	/// 1회 시도를 위한 exchange strong 버전.
	/// spurious failure 방지를 위해 strong으로 시도한다.
	/// 단, 이 경우 weak에 비해 오버헤드가 크므로 lock버전에서는 get_lock을 쓰자.
	/// </summary>
	/// <returns></returns>
	bool try_lock();

private:
	std::atomic_bool flag;
};

struct try_to_lock_t { explicit try_to_lock_t() = default; };
constexpr try_to_lock_t try_to_lock{};

/// <summary>
/// std::lock_guard처럼 생성자와 동시에 잠금. 소멸자로 해제.
/// </summary>
class SpinLockGuard final
{
public:
	SpinLockGuard() = delete;
	SpinLockGuard(SpinLock& lock_);
	SpinLockGuard(SpinLock& lock_, const try_to_lock_t&);

	~SpinLockGuard();

	SpinLockGuard(const SpinLockGuard& other_) = delete;
	SpinLockGuard(SpinLockGuard&& rhs_) = delete;

	SpinLockGuard& operator=(const SpinLockGuard& other_) = delete;
	SpinLockGuard& operator=(SpinLockGuard&& rhs_) = delete;

	bool owns_lock() const;

	void unlock();
private:
	SpinLock& m_lock;
	bool locked;
};