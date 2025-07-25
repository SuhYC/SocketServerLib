#include "UserManager.hpp"

UserManager::UserManager(uint32_t uMaxClient_) : m_uMaxClient(uMaxClient_)
{
}

UserManager::~UserManager()
{
	for (auto* pUser : m_Users)
	{
		if (pUser != nullptr)
		{
			delete pUser;
		}
	}
}

bool UserManager::Init()
{
	try
	{
		m_Users.reserve(m_uMaxClient);
		for (int i = 0; i < m_uMaxClient; i++)
		{
			User* pUser = CreateUser(i);
			m_Users.push_back(pUser);
		}
	}
	catch (std::bad_alloc&)
	{
		LOG_ERR("Users Vector Init Failed.");
		return false;
	}
	DEFAULT_CATCH()

	return true;
}

User* UserManager::GetUser(uint32_t uUserIndex_)
{
	if (uUserIndex_ >= m_uMaxClient)
	{
		LOG_ERR("Invalid User Index.");
		return nullptr;
	}

	return m_Users[uUserIndex_];
}

User* UserManager::CreateUser(uint32_t uUserIndex_)
{
	return new User(uUserIndex_);
}