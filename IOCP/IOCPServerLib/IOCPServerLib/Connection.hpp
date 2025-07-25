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
	/// �۽Ź��ۿ� �����͸� ��� �۽Ű����ϸ� SendIO�� ȣ���� �۽ſ����մϴ�.
	/// </summary>
	/// <param name="pData_">�۽��� �޽��� ��Ŷ</param>
	/// <returns></returns>
	bool SendMsg(PacketData* pData_);

	/// <summary>
	/// �۽ſ����۾��� �����մϴ�.
	/// �۽Ź��ۿ��� �۽��� �� �ִ� ũ�⸦ ����Ͽ� �۽ſ����� �ɰ�,
	/// WSASend�� ȣ���մϴ�.
	/// </summary>
	/// <returns></returns>
	bool SendIO();

	bool ReSend();

	/// <summary>
	/// CP�κ��� �۽ſϷ� �̺�Ʈ�� �޾��� �� ȣ���ϸ� �˴ϴ�.
	/// �۽Ź��ۿ��� �۽��� �Ϸ�� �κ��� ������ ��, ���� �����ִ� �����Ͱ� �ִٸ� �̾ �۽��մϴ�.
	/// </summary>
	void SendCompleted();

	void Close(bool bIsForce_ = false);

	bool GetIP(uint32_t& out_);

	char* RecvBuffer() { return mRecvBuf; }
	unsigned short GetIndex() { return m_ClientIndex; }

	/// <summary>
	/// ������ �޽����� ������ ũ�⸦ �������� Ȯ���ϱ� ���� ������ ������ �޽����� �����մϴ�.
	/// m_RecvBuffer�� ���ŵ� �޽������� ���յǾ� ����Ǹ�,
	/// Connection::GetReqMessage(char*,uint32_t)�� ���� ��ܵ� �޽����� ��ȯ���� �� �ֽ��ϴ�.
	/// </summary>
	/// <param name="str_"></param>
	/// <param name="size_"></param>
	/// <returns></returns>
	bool StorePartialMessage(char* str_, uint32_t size_);

	/// <summary>
	/// ����� �����Ͱ� ���ų�,
	/// ����� ǥ���� �޽����� ���̺��� ª�� �����͸� ����ִ� ���
	/// 0�� ��ȯ.
	/// 
	/// ����� �����Ͱ� �ִ� ��� out_�� �޽����� �����ϰ�
	/// ����� ���̸� ��ȯ.
	/// </summary>
	/// <param name="out_">�޽����� ��ȯ���� ���� �ּ�</param>
	/// <returns>����� �޽����� ����</returns>
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