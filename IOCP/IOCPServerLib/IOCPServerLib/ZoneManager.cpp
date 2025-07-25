#include "ZoneManager.hpp"

ZoneManager::ZoneManager()
{

}

ZoneManager::~ZoneManager()
{
	for (auto& pair : m_Zones)
	{
		if (pair.second != nullptr)
		{
			delete pair.second;
		}
	}
}

Zone* ZoneManager::GetZone(uint32_t uZoneIndex_)
{
	SpinLockGuard guard(m_Lock);
	auto itr = m_Zones.find(uZoneIndex_);

	if (itr != m_Zones.end())
	{
		return itr->second;
	}

	Zone* pRet = CreateZone(uZoneIndex_);

	return pRet;
}

Zone* ZoneManager::CreateZone(uint32_t uZoneIndex_)
{
	return new Zone();
}