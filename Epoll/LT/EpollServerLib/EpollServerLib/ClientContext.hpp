#pragma once

#include <stdint.h>
#include <string_view>

#include <fcntl.h> // non-Blocking
#include <netinet/tcp.h> // nodelay
#include <sys/socket.h>
#include <unistd.h> // fd
#include <netinet/in.h> // IPPROTO_TCP
#include <sys/epoll.h>

#include "PacketData.hpp"
#include "SlideBuffer.hpp"
#include "SpinLock.hpp"

class ClientContext
{
public:
	ClientContext();
	~ClientContext();

	bool Init(const int fd_);
	epoll_event& GetEpollEvent();
	void Close();

	bool StorePartialMsg(const char* pMsg_, uint32_t ioSize_);
	bool GetReqMsg(std::string_view& out_);
	bool PopMsg(std::string_view& sv_);

	SendStatus SendMsg(PacketData* pPacket_);
	SendStatus ResumeSend();
private:
	int m_Socket;

	SlideBuffer recvBuffer;
	SlideBuffer sendBuffer;

	epoll_event ep_ev;

	SpinLock m_SendLock;
};