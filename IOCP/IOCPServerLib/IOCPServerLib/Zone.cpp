#include "Zone.hpp"

Zone::Zone()
{
	
}

Zone::~Zone()
{

}

bool Zone::Enter(uint32_t uClientIndex_, User* pUser_)
{
	SpinLockGuard guard(m_Lock);

	try
	{
		m_Users.emplace(uClientIndex_, pUser_);
	}
	catch (std::bad_alloc& e)
	{
		LOG_ERR("");
		return false;
	}
	DEFAULT_CATCH()

	return true;
}

bool Zone::Exit(uint32_t uClientIndex_)
{
	SpinLockGuard guard(m_Lock);

	return m_Users.erase(uClientIndex_);
}