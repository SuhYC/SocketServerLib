#pragma once

#include <sys/socket.h>
#include <stdint.h>
#include <vector>
#include <string_view>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <thread>
#include <atomic>
#include <algorithm>

#include "PacketData.hpp"
#include "SlideBuffer.hpp"
#include "ClientContext.hpp"
#include "SpinLock.hpp"

// ��Ŷ ���� �� ������ ���� �� �����̵� ������ ũ��.
constexpr uint32_t MAX_BUF_SIZE = 131072;

// ���Ϳ��� �����ؾ��� Ŭ���̾�Ʈ ���� �����尡 �ش� ������ �������� ���Ϳ��� �����ϴ� ���� ����. (join�� 
constexpr uint32_t THREAD_CLEAN_REQ_CNT = 10;

class SocketServer
{
protected:
	bool Run(uint16_t nBindPort_);
	void Stop();

	bool SendMsg(int nClientIndex_, PacketData* pPacket_);

private:
	bool Init(uint16_t nBindPort_);
	void CreateClientThread(int nClientIndex_);
	void ClientThread(int nClientIndex_, bool& isAlive_);
	void CreateAcceptThread();
	void AcceptThread();
	void RequestToCleanUpThreads();
	void CleanUpThreads();

	void DestroyThreads();

	bool m_IsRun;
	int serv_sock;
	std::vector<ClientContext*> m_Clients;
	std::thread m_AcceptThread;
	SpinLock m_Lock;
	std::atomic<uint32_t> reqCnt;

	virtual void OnReceive(int nClientIndex_, std::string_view& sv) = 0;
	virtual void OnDisconnect(int nClientIndex_) = 0;
	virtual void OnConnect(int nClientIndex_) = 0;
};