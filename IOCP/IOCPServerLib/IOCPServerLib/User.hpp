#pragma once

#include "stdint.h"


class User
{
public:
	User(uint32_t uUserIndex_);
	virtual ~User();

	virtual void Init();

	virtual void Clear();

	void SetZoneIndex(uint32_t uZoneIndex_);
	uint32_t GetZoneIndex();

private:
	uint32_t m_Index;
	uint32_t m_ZoneIndex;
};