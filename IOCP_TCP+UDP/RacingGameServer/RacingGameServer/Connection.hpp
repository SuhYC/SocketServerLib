#pragma once

#include "Define.hpp"
#include <string_view>
#include "PacketData.hpp"
#include "Job.hpp"
#include "SlideBuffer.hpp"
#include "SendBuffer.hpp"
#include "SpinLock.hpp"


/*
* to do.
* UDP 송신에 지속적으로 주소갱신해줄것. (IOCP코어 쪽 작업)
* 
*/

class Connection final
{
public:
	Connection(const SOCKET listenSocket_, const int index_);
	~Connection();

	void Init();

	void ResetConnection();

	bool BindIOCP(const HANDLE hWorkIOCP_);

	/// <summary>
	/// TCP수신을 바인드한다.
	/// UDP수신은 중앙에서 처리할거니 상관없다.
	/// </summary>
	/// <returns></returns>
	bool BindRecv();

	/// <summary>
	/// TCP송신을 요청한다.
	/// </summary>
	/// <param name="pData_"></param>
	/// <returns></returns>
	bool SendMsgTCP(PacketData* pData_);

	/// <summary>
	/// 문제가 생겨 같은 송신을 재시도해야할 때
	/// </summary>
	/// <returns></returns>
	bool ReSendTCP();

	/// <summary>
	/// 완료된 TCP송신에 대해 버퍼를 갱신하고 이어서 송신한다.
	/// </summary>
	/// <param name="ioSize_"></param>
	void TCPSendCompleted(uint32_t ioSize_);


	/// <summary>
	/// 적절한 토큰으로 식별된 UDP소켓을 클라이언트와 연결한다.
	/// </summary>
	/// <param name="udpAddr"></param>
	void SetUDPIP(const sockaddr_in& udpAddr);

	/// <summary>
	/// 수신한 TCP메시지를 버퍼에 병합한다.
	/// </summary>
	/// <param name="str_"></param>
	/// <param name="size_"></param>
	/// <returns></returns>
	bool StorePartialMessage(uint32_t size_);

	/// <summary>
	/// 병합된 메시지를 파싱하기 위해 메시지단위로 잘라 반환한다.
	/// </summary>
	/// <param name="sv_"></param>
	/// <returns></returns>
	bool GetReqMessage(std::string_view& out_);

	/// <summary>
	/// 처리가 끝난 수신 메시지를 버퍼에서 제거한다.
	/// </summary>
	/// <param name="ioSize_"></param>
	/// <returns></returns>
	bool PopRecvBuffer(uint32_t ioSize_);

	void Close(bool bIsForce_ = false);

	uint16_t GetIndex();

	/// <summary>
	/// for TCPsocket, setsockopt + getpeername
	/// </summary>
	/// <param name="out_"></param>
	/// <returns></returns>
	bool SetSocketOpt(sockaddr_in& out_);

	/// <summary>
	/// for UDPsocket
	/// </summary>
	/// <returns></returns>
	sockaddr_in& GetUDPIP();

	/// <summary>
	/// 현재 설정된 UDP 매핑 토큰을 반환.
	/// </summary>
	/// <returns></returns>
	uint64_t GetUDPToken();

	/// <summary>
	/// UDP매핑 토큰을 교환.
	/// 반환값이 0이 아닌 경우 : 이미 토큰이 있었으므로 기존 토큰을 제거해주어야함.
	/// </summary>
	/// <param name="uNewToken_"></param>
	/// <returns></returns>
	uint64_t SetUDPToken(uint64_t uNewToken_);

	void SetJobProcess(JobProcess eVal_);

	JobProcess GetJobProcess();

	/// <summary>
	/// 새로운 작업객체를 저장하고
	/// 기존 작업객체를 반환한다.
	/// </summary>
	/// <param name="pJob_"></param>
	/// <returns></returns>
	Job* SetNewJob(Job* pJob_);

private:
	bool SendTCPIO();

	bool BindAcceptEx();

	sockaddr_in m_UDPAddr;
	
	SOCKET m_ListenSocket;
	SOCKET m_TcpSocket;

	char m_AcceptBuf[128];
	char m_RecvBuf[MAX_SOCKBUF];

	stOverlappedEx m_RecvOverlapped;
	stOverlappedEx m_TCPSendOverlapped;

	SlideBuffer m_RecvBuffer;
	SendBuffer m_TCPSendBuffer;

	SpinLock m_TCPSpinLock;
	SpinLock m_RecvLock;

	bool m_IsConnected;
	uint16_t m_ClientIndex;
	uint64_t m_UDPToken;
	
	Job* m_pJob;
	JobProcess m_eJobProcess;

};