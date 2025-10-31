#pragma once

#include <vector>
#include <thread>
#include <string_view>
#include <map>
#include <queue>

#include "Define.hpp"
#include "Connection.hpp"
#include "ObjectPool.hpp"
#include "UDPSendContext.hpp"

// 공식API로 제공되지 않음.
constexpr ULONG STATUS_CANCELLED = 0xC0000120;


/// <summary>
/// 1. TCP Socket <-> UDP IP 간의 매칭
/// - 일단 TCP로 UDP토큰을 발행, server -TCP-> client
/// - 클라이언트는 UDP로 토큰을 포함하여 메시지 발신, client -UDP-> server
/// - 서버는 토큰을 확인하여 기존에 발행한 토큰과 일치하면 연결객체에 UDP IP를 매핑.
/// 
/// </summary>

class IOCPCore
{
protected:
	IOCPCore();
	virtual ~IOCPCore();

	bool SendMsgTCP(const uint32_t uClientIndex_, PacketData* pPacket_);
	bool SendMsgUDP(const uint32_t uClientIndex_, PacketData* pPacket_);
	bool Start(uint16_t uBindPort_, uint16_t uUdpPort_, uint32_t uMaxClient_);
	bool End();

	bool GetReqMsg(uint32_t uClientIndex_, std::string_view& sv_);
	bool PopReqMsg(uint32_t uClientIndex_, std::string_view& sv_);

	/// <summary>
	/// 기존 토큰이 있다면 가져오고, 없다면 발행하여 반환한다.
	/// </summary>
	/// <param name="uClientIndex_"></param>
	/// <returns>신규생성된 토큰</returns>
	uint64_t GetUDPToken(uint32_t uClientIndex_);

	/// <summary>
	/// 토큰 갱신이 필요한 시점에 호출.
	/// 신규 토큰을 발행하고, 기존 토큰은 있다면 제거한다.
	/// </summary>
	/// <param name="uClientIndex_"></param>
	/// <returns></returns>
	uint64_t RenewUDPToken(uint32_t uClientIndex_);

	/// <summary>
	/// 해당 클라이언트의 TCP수신 처리 단계를 설정합니다.
	/// </summary>
	/// <param name="uClientIndex_"></param>
	/// <param name="eVal_"></param>
	void SetJobProcess(uint32_t uClientIndex_, JobProcess eVal_);

	/// <summary>
	/// 해당 클라이언트의 TCP수신 처리 단계를 가져옵니다.
	/// </summary>
	/// <param name="uClientIndex_"></param>
	/// <returns></returns>
	JobProcess GetJobProcess(uint32_t uClientIndex_);

	Job* SetNewJob(uint32_t uClientIndex_, Job* pJob_);

private:
	bool InitIOCP(uint16_t uBindPort_);
	bool InitUDP(uint16_t uUdpPort_);

	bool CreateWorkerThreads();
	bool DestroyWorkerThreads();
	void PostEndToWorkerThreads();
	bool CreateConnections(uint32_t uMaxClient_);

	bool BindUDPRecv();
	bool UDPResend(stUDPSendContext* pOverlapped_);

	void ReleaseConnections();

	Connection* GetConnection(uint32_t uClientIndex_);

	void WorkerThread();

	/// <summary>
	/// 연결이 끊긴 경우 처리할 것들 집합.
	/// </summary>
	/// <param name="uClientIndex_"></param>
	void DoDisconnect(uint32_t uClientIndex_, bool bBroadCast_ = false);
	/// <summary>
	/// 연결객체에 저장된 토큰을 조회하여 남아있는 토큰이 있다면 정리.
	/// </summary>
	/// <param name="uClientIndex_"></param>
	void ClearUDPToken(uint32_t uClientIndex_);

	void DoAccept(const stOverlappedEx* const pOverlapped_);
	void DoTCPRecv(const stOverlappedEx* const pOverlapped_, const DWORD ioSize_);
	void DoUDPRecv(const stOverlappedEx* const pOverlapped_, const DWORD ioSize_);
	void DoTCPSend(const stOverlappedEx* const pOverlapped_, const DWORD ioSize_);
	void DoUDPSend(stUDPSendContext* pOverlapped_);

	virtual void OnTCPReceive(const uint32_t uClientIndex_) = 0;
	virtual void OnUDPReceive(const uint32_t uClientIndex_, std::string_view& msg_) = 0;

	virtual void OnConnect(const uint32_t uClientIndex_, const sockaddr_in& addr_) = 0;
	virtual void OnDisconnect(const uint32_t uClientIndex_) = 0;

	bool m_IsRun;

	std::vector<Connection*> m_Connections;
	SOCKET m_ListenSocket;
	SOCKET m_UDPSocket;
	
	HANDLE m_IOCPHandle;

	stOverlappedEx m_UDPRecvOverlapped;
	ObjectPool<stUDPSendContext> m_UDPSendContextPool;

	char m_UDPRecvBuf[MAX_SOCKBUF];
	std::map<uint64_t, uint32_t> m_UDPTokens;

	std::vector<std::thread> m_WorkerThreads;

	SpinLock m_UDPTokenLock;
};