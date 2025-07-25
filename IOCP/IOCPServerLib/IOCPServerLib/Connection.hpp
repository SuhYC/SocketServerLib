#pragma once

#include "Define.hpp"

#include <WinSock2.h>
#include <MSWSock.h> // AcceptEx()
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib") // acceptEx()

#include <string>

#include "SlideBuffer.hpp"
#include "PacketData.hpp"
#include "SpinLock.hpp"

class Connection final
{
public:
	Connection(const SOCKET listenSocket_, const int index);
	~Connection();

	void Init();

	void ResetConnection();

	bool BindIOCP(const HANDLE hWorkIOCP_);

	bool BindRecv();

	/// <summary>
	/// 송신버퍼에 데이터를 담고 송신가능하면 SendIO를 호출해 송신예약합니다.
	/// </summary>
	/// <param name="pData_">송신할 메시지 패킷</param>
	/// <returns></returns>
	bool SendMsg(PacketData* pData_);

	/// <summary>
	/// 송신예약작업을 진행합니다.
	/// 송신버퍼에서 송신할 수 있는 크기를 기록하여 송신예약을 걸고,
	/// WSASend를 호출합니다.
	/// </summary>
	/// <returns></returns>
	bool SendIO();

	bool ReSend();

	/// <summary>
	/// CP로부터 송신완료 이벤트를 받았을 때 호출하면 됩니다.
	/// 송신버퍼에서 송신이 완료된 부분을 제거한 뒤, 아직 남아있는 데이터가 있다면 이어서 송신합니다.
	/// </summary>
	void SendCompleted();

	void Close(bool bIsForce_ = false);

	bool GetIP(uint32_t& out_);

	char* RecvBuffer() { return mRecvBuf; }
	unsigned short GetIndex() { return m_ClientIndex; }

	/// <summary>
	/// 수신한 메시지가 적절한 크기를 가지는지 확인하기 전에 기존에 수신한 메시지와 병합합니다.
	/// m_RecvBuffer에 수신된 메시지들이 병합되어 저장되며,
	/// Connection::GetReqMessage(char*,uint32_t)를 통해 재단된 메시지를 반환받을 수 있습니다.
	/// </summary>
	/// <param name="str_"></param>
	/// <param name="size_"></param>
	/// <returns></returns>
	bool StorePartialMessage(char* str_, uint32_t size_);

	/// <summary>
	/// 저장된 데이터가 없거나,
	/// 헤더에 표현된 메시지의 길이보다 짧은 데이터만 들어있는 경우
	/// 0을 반환.
	/// 
	/// 충분한 데이터가 있는 경우 out_에 메시지를 복사하고
	/// 복사된 길이를 반환.
	/// </summary>
	/// <param name="out_">메시지를 반환받을 버퍼 주소</param>
	/// <returns>복사된 메시지의 길이</returns>
	bool GetReqMessage(std::string_view& sv_);

	bool PopRecvBuffer(std::string_view& sv_);

private:
	bool BindAcceptEx();

	SOCKET m_ListenSocket;
	SOCKET m_ClientSocket;

	std::atomic<bool> m_IsConnected;
	unsigned short m_ClientIndex;

	SpinLock m_SpinLock;

	char mAcceptBuf[64];

	stOverlappedEx m_RecvOverlapped;
	char mRecvBuf[MAX_SOCKBUF];

	SlideBuffer m_SendBuffer;
	SlideBuffer m_RecvBuffer;

	stOverlappedEx m_SendOverlapped;
};