#pragma once

#include "Define.hpp"
#include <Windows.h>
#include "PacketData.hpp"

struct stUDPSendContext
{
	/// <summary>
	/// 송신전 초기화
	/// </summary>
	/// <param name="pPacket_"></param>
	/// <returns></returns>
	bool Init(uint32_t uClientIndex_, sockaddr_in& addr, PacketData* pPacket_);

	stOverlappedEx m_overlapped;
	PacketData* pPacket;
};