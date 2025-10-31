#pragma once

#include "PacketData.hpp"
#include "Define.hpp"
#include <concurrent_queue.h>
#include "LogManager.hpp"


/*
* Singleton 객체다.
* 다른 싱글턴 객체의 소멸자에서 해당 객체에 접근하지 말것.
* 어차피 이 객체 또한 소멸자에서 동적객체들을 모두 해제하므로
* 각자의 소멸자에서 해제하고 끝내는게 좋다.
* (전역객체들의 소멸 순서는 예측하기 어렵다.
* 초기화된 순서의 역순으로 정리되지만,
* 헤더 관계가 복잡하게 얽혀있는 경우
* 순서를 추적하기 어렵다.)
*/

const uint32_t PACKETCOUNT = 100;

class PacketPool final
{
public:
	~PacketPool();

	static PacketPool& Instance();

	/// <summary>
	/// 패킷버퍼의 포인터를 반환합니다.
	/// 큐에 남은 패킷이 없는 경우 할당을 시도하여
	/// 실패하면 nullptr를 반환합니다.
	/// </summary>
	/// <returns></returns>
	PacketData* Allocate();

	/// <summary>
	/// 사용이 끝난 패킷 버퍼를 큐에 넣습니다.
	/// </summary>
	/// <param name="pPacket_"></param>
	void Deallocate(PacketData* pPacket_);

private:
	PacketPool();

	Concurrency::concurrent_queue<PacketData*> m_FreeList;
};