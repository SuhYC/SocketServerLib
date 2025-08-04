#pragma once

#include "EpollServer.hpp"
#include "JobFactory.hpp"
#include "Job.hpp"
#include <queue>
#include <thread>

class GameServer : public EpollServer
{
public:
	GameServer(uint16_t uBindPort_);
	~GameServer();

	bool Start();
	void End();

private:
	bool CreateJobThread();
	void JobThread();
	bool DestroyJobThread();
	
	SpinLock m_JobLock;
	std::queue<Job*> m_JobQueue;
	JobFactory m_JobFactory;
	std::thread m_JobThread;

	bool m_IsRun;

	virtual void OnReceive(const int fd_, std::string_view& sv_) override;
	virtual void OnConnect(const int fd_) override;
	virtual void OnDisconnect(const int fd_) override;
};