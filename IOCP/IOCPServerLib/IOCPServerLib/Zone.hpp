#pragma once

#include <map>
#include "User.hpp"
#include "SpinLock.hpp"
#include <stdint.h>
#include "Define.hpp"
#include "LogManager.hpp"

class Zone
{
public:
	Zone();
	virtual ~Zone();

	virtual bool Enter(uint32_t uClientIndex_, User* pUser_);
	virtual bool Exit(uint32_t uClientIndex_);

protected:
	std::map<uint32_t, User*> m_Users;

	SpinLock m_Lock;
};