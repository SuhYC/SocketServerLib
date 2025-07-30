#pragma once

#include "SelectServer.hpp"
#include "Define.hpp"
#include "JobFactory.hpp"
#include <concurrent_queue.h>
#include <thread>

class GameServer : public SelectServer
{
public:
	GameServer(uint16_t uBindPort_);
	~GameServer();

	bool Start();
	bool End();
private:
	bool CreateJobThread();
	bool DestroyJobThread();
	
	InfoCode SendResultMsg(const SOCKET clientSocket_, PacketData* pPacket_);
	void JobThread();

	bool m_IsRun;

	std::thread m_JobThread;

	JobFactory m_JobFactory;
	Concurrency::concurrent_queue<Job*> m_JobQueue;

	virtual void OnReceive(SOCKET clientSocket_) override;
	virtual void OnConnect(SOCKET clientSocket_) override;
	virtual void OnDisconnect(SOCKET clientSocket_) override;
};