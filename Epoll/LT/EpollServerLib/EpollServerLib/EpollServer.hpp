#pragma once

#include <sys/socket.h>
#include <sys/epoll.h>

#include <string_view>
#include <stdint.h>
#include <unordered_map>
#include <thread>

#include "PacketData.hpp"
#include "ClientContext.hpp"
#include "SpinLock.hpp"

class EpollServer
{
protected:
	EpollServer();
	virtual ~EpollServer();

	bool Start(uint16_t uBindPort_);
	bool End();

	bool SendMsg(const int fd_, PacketData* pPacket_);
private:
	bool SocketInit(uint16_t uBindPort_);
	bool CreateThreads();
	bool DestoryThreads();
	bool CreateContext(const int fd_);
	bool ReleaseContext(const int fd_);
	void ReleaseAllContexts();

	void ReadThread();

	ClientContext* GetContext(const int fd_);
	
	void DoAccept();
	void DoRecv(const int fd_);
	void Close(const int fd_);
	void ResumeSend(const int fd_);

	int m_ListenSocket;
	std::unordered_map<int, ClientContext*> Clients;

	int epoll_fd;

	std::thread m_ReadThread;
	std::thread m_WriteThread;

	SpinLock m_ClientLock;
	bool m_IsRun;

	virtual void OnReceive(const int fd_, std::string_view& sv_) = 0;
	virtual void OnConnect(const int fd_) = 0;
	virtual void OnDisconnect(const int fd_) = 0;
};