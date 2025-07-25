#pragma once

#include <map>

#include <stdint.h>
#include "Define.hpp"
#include "Zone.hpp"

#include "SpinLock.hpp"

class ZoneManager
{
public:
	ZoneManager();
	virtual ~ZoneManager();

	Zone* GetZone(uint32_t uZoneIndex_);

protected:
	virtual Zone* CreateZone(uint32_t uZoneIndex_);

private:
	SpinLock m_Lock;
	std::map<uint32_t, Zone*> m_Zones;
};