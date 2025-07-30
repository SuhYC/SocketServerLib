#pragma once

#include "Define.hpp"

#include <stdint.h>
#include <new>
#include <unordered_map>
#include <thread>
#include "ClientContext.hpp"
#include "SpinLock.hpp"

class SelectServer
{
protected:
	SelectServer();
	~SelectServer();

	bool Start(uint16_t uBindPort_);
	bool End();

	bool SendMsg(const SOCKET clientSocket_, PacketData* pPacket_);

	bool GetReqMsg(const SOCKET clientSocket_, std::string_view& out_);
	bool PopMsg(const SOCKET clientSocket_, std::string_view& sv_);
private:
	bool SocketInit(uint16_t uBindPort_);
	bool CreateThread();
	bool DestroyThread();
	bool CreateContext(const SOCKET clientSocket_);
	bool ReleaseContext(SOCKET clientSocket_);
	void ReleaseAllContexts();

	void ReadThread();
	void WriteThread();

	ClientContext* GetContext(const SOCKET clientSocket_);

	void DoAccept();
	void DoRecv(const SOCKET clientSocket_);
	void ResumeSend(const SOCKET clientSocket_);

	SOCKET m_ListenSocket;
	std::unordered_map<SOCKET, ClientContext*> Clients;

	fd_set reads, writes;

	std::thread m_ReadThread;
	std::thread m_WriteThread;

	SpinLock m_ClientLock;

	bool m_IsRun;

	virtual void OnReceive(SOCKET clientSocket_) = 0;
	virtual void OnConnect(SOCKET clientSocket_) = 0;
	virtual void OnDisconnect(SOCKET clientSocket_) = 0;
};