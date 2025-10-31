#pragma once

#include "IOCPCore.hpp"
#include <vector>
#include <thread>
#include <queue>
#include <atomic>


#include <concurrent_priority_queue.h>
#include <concurrent_queue.h>

#include <sstream>

#include "ThreadPool.hpp"

class GameServer : protected IOCPCore
{
public:
	GameServer();
	virtual ~GameServer();

	bool Start(uint16_t uBindPort_, uint16_t uUdpPort_, uint32_t uMaxClient_);

	void End();

private:
	void OnTCPReceive(const uint32_t uClientIndex_) override;

	/// <summary>
	/// 일단 동기화가 필요한 패킷은 UDP를 통하면 안된다.
	/// TCP와 병렬로 실행해도 문제없는 동작을 수행할 예정. 
	/// (주로 실시간성만 중요한 위치정보 등을 전파하는데 쓸 예정.)
	/// </summary>
	/// <param name="uClientIndex_"></param>
	/// <param name="msg_"></param>
	void OnUDPReceive(const uint32_t uClientIndex_, std::string_view& msg_) override;

	void OnConnect(const uint32_t uClientIndex_, const sockaddr_in& addr_) override;
	void OnDisconnect(const uint32_t uClientIndex_) override;

	/// <summary>
	/// 각 클라이언트의 TCP요청을 처리하는 함수.
	/// 해당 함수를 ThreadPool에 넘겨 처리할 예정.
	/// 파싱 -> 처리 -> 송신 까지의 과정을 수행하다가 중단되면 이후에 이어서 할 수 있도록 구현할것.
	/// 
	/// 일단 SpinLock으로 현재 다른 스레드가 처리하고 있지는 않은지 체크.
	/// 이후 동작을 수행할 수 있는 만큼 수행한 후 어디까지 했는지 기록.
	/// SpinLock을 해제하고 반환 (이후 InfoCode를 받은 ThreadPool쪽에서 ReQueue.)
	/// </summary>
	/// <param name="uClientIndex_"></param>
	/// <returns></returns>
	InfoCode HandleReq(const uint32_t uClientIndex_);

	/// <summary>
	/// TCP 수신버퍼로부터 메시지단위로 가져와
	/// 헤더 기반으로 작업객체를 구분하여 파라미터를 파싱한 후
	/// 성공한 경우 해당 메시지를 버퍼에서 제거.
	/// 작업객체는 GameServer에 벡터로 두자 그냥.
	/// </summary>
	/// <param name="uClientIndex_"></param>
	/// <returns></returns>
	bool ParseReq(const uint32_t uClientIndex_);

	/// <summary>
	/// 벡터에 저장해둔 작업객체를 실행합니다.
	/// 작업이 완료된 경우 결과 송신이 필요하면 PacketData 버퍼를 가져와 작성한 뒤 리턴합니다.
	/// </summary>
	/// <param name="uClientIndex_"></param>
	/// <returns>중단된 경우 false, 결과에 상관없이 완료된 경우 true</returns>
	bool ExecuteReq(const uint32_t uClientIndex_);

	/// <summary>
	/// Execute 단계에서 작성한 메시지를 송신버퍼에 담습니다.
	/// 메모리 복사가 필요하고 송신버퍼가 가득찬 경우 재시도하여야 하기 때문에 분리작성합니다.
	/// </summary>
	/// <param name="uClientIndex_"></param>
	/// <returns>송신버퍼에 담는데 성공하면 true</returns>
	bool SendRes(const uint32_t uClientIndex_);

	bool SendMsgFunc(const uint32_t uClientIndex_, PacketData* pPacket_, SendProtocol eProtocol_);

	bool CreateJobThreads(const uint16_t uMaxThreads_);
	bool PushJob(const uint32_t uClientIndex_);
	void ReleaseJobThreads();

	void ReleaseRemainJobs();

	ThreadPool JobThreads;

	std::vector<std::unique_ptr<SpinLock>> ProcessLocks;
	std::vector<Job*> m_Jobs;
};