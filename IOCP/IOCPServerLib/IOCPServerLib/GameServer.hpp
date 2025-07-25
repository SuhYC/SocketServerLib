#pragma once

#include "Define.hpp"
#include "IOCPServer.hpp"
#include "Job.hpp"
#include "LogManager.hpp"
#include "JobFactory.hpp"
#include <concurrent_queue.h>
#include <optional>
#include <string_view>
#include "PacketPool.hpp"
#include "PacketData.hpp"

class GameServer final : public IOCPServer
{
public:
	GameServer(uint32_t uBindPort_, uint32_t uMaxClient_);
	~GameServer();

	bool Run();
	bool End();

private:
	void OnReceive(const uint32_t uClientIndex_) override;
	void OnConnect(const uint32_t uClientIndex_, const uint32_t ip_) override;
	void OnDisconnect(const uint32_t uClientIndex_) override;

	bool CreateJobThreads();
	bool DestroyJobThreads();

	void JobThread();

	bool m_IsRun;

	UserManager m_UserManager;
	ZoneManager m_ZoneManager;
	PacketPool m_PacketPool;

	JobFactory m_JobFactory;

	Concurrency::concurrent_queue<Job*> m_JobQueue;
	std::vector<std::thread> m_JobThreads;

	InfoCode SendResultMsg(const uint32_t uClientIndex_, PacketData* pPacket_)
	{
		bool bRet = SendMsg(uClientIndex_, pPacket_);

		if (!bRet)
		{
			// -- 버퍼가 가득찬 상태라 송신요청이 실패함. (Re-Queueing 필요)
			return InfoCode::NOT_FINISHED;
		}

		return InfoCode::REQ_SUCCESS;
	}
};