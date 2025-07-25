#pragma once

#include "BlockingSocketServer.hpp"
#include <thread>
#include <queue>
#include "JobFactory.hpp"
#include "SpinLock.hpp"
#include <string_view>

class GameServer : public SocketServer
{
public:
	GameServer(uint16_t nBindPort_);
	~GameServer();

	bool Start();
	bool End();
private:
	bool CreateJobThread();
	void JobThread();
	bool DestroyJobThread();


	bool m_IsRun;

	std::queue<Job*> m_JobQueue;
	std::thread m_JobThread;
	JobFactory m_JobFactory;
	SpinLock m_Lock;

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

	virtual void OnReceive(int nClientIndex_, std::string_view& sv_);
	virtual void OnConnect(int nClientIndex_);
	virtual void OnDisconnect(int nClientIndex_);
};