#pragma once

#include <vector>
#include <thread>
#include "Connection.hpp"
#include <string_view>

// 공식API로 제공되지 않음.
constexpr ULONG STATUS_CANCELLED = 0xC0000120;

class IOCPServer
{
protected:
	IOCPServer();
	virtual ~IOCPServer();

	bool SendMsg(const uint32_t uClientIndex_, PacketData* pPacket_);
	bool Start(uint32_t uBindPort_, uint32_t uMaxClient_);
	bool End();

	bool GetReqMsg(uint32_t uClientIndex_, std::string_view& sv_);
	bool PopReqMsg(uint32_t uClientIndex_, std::string_view& sv_);

private:
	bool InitIOCP(uint32_t uBindPort_);

	bool CreateWorkerThreads();
	bool DestroyWorkerThreads();
	void PostEndToWorkerThreads();
	bool CreateConnections(uint32_t uMaxClient_);
	void ReleaseConnections();

	Connection* GetConnection(uint32_t uClientIndex_);

	void WorkerThread();

	void DoAccept(const stOverlappedEx* pOverlapped_);
	void DoRecv(const stOverlappedEx* pOverlapped_, const DWORD ioSize_);
	void DoSend(const stOverlappedEx* pOverlapped_);

	virtual void OnReceive(const uint32_t uClientIndex_) = 0;
	virtual void OnConnect(const uint32_t uClientIndex_, const uint32_t ip_) = 0;
	virtual void OnDisconnect(const uint32_t uClientIndex_) = 0;

	bool m_IsRun;

	std::vector<Connection*> m_Connections;
	SOCKET m_ListenSocket;
	HANDLE m_IOCPHandle;

	std::vector<std::thread> m_WorkerThreads;
};