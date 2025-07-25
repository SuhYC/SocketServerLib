#pragma once

#include "User.hpp"
#include <stdint.h>
#include <vector>
#include "Define.hpp"

class UserManager
{
public:
	UserManager(uint32_t uMaxClient_);
	~UserManager();

	virtual bool Init();
	virtual User* GetUser(uint32_t uUserIndex_) final;

protected:
	virtual User* CreateUser(uint32_t uUserIndex_);

private:
	std::vector<User*> m_Users;
	uint32_t m_uMaxClient;
};