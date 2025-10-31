#pragma once

#include "PacketData.hpp"
#include "PacketPool.hpp"

/// <summary>
/// Scatter / Gather를 위해 작성한 클래스.
/// 일단 PacketData를 받아 원형큐의 형태로 저장하고,
/// 요청하면 내부에 있는 PacketData들의 정보를 WSABUF[]의 형태로 저장한다.
/// 반환될 out파라미터는 WSABUF*와 uint32_t bufcnt의 형태로 들어온다.
/// </summary>
class SendBuffer
{
public:

	SendBuffer();
	~SendBuffer();

	/// <summary>
	/// 원형큐를 재할당합니다.
	/// 내부데이터는 유지되지 않으며
	/// 큐에 있던 버퍼는 버퍼풀로 반환됩니다.
	/// </summary>
	/// <param name="capacity_"></param>
	/// <returns></returns>
	bool Allocate(uint32_t capacity_);

	/// <summary>
	/// 새로운 메시지가 담긴 버퍼를 원형큐에 삽입합니다.
	/// </summary>
	/// <param name="pPacket_"></param>
	/// <returns></returns>
	bool Push(PacketData* pPacket_);

	/// <summary>
	/// 완료된 IO크기에 맞춰 데이터를 제거합니다.
	/// 완전히 송신 완료된 버퍼는 버퍼풀에 반환하고,
	/// 부분송신된 버퍼는 송신된 부분을 제거합니다.
	/// </summary>
	/// <param name="ioSize_"></param>
	void Pop(uint32_t ioSize_);

	/// <summary>
	/// 원형큐를 순회하며 각 버퍼의 정보를 WSABUF배열에 복사합니다.
	/// 반환값은 반환된 WSABUF배열에 기록된 버퍼의 수입니다.
	/// </summary>
	/// <param name="out_">반환될 WSABUF배열</param>
	/// <param name="bufcnt_">반환될 WSABUF배열의 최대크기</param>
	/// <returns>반환된 WSABUF배열에 기록한 버퍼의 수</returns>
	uint32_t GetWSABUF(WSABUF* out_, uint32_t bufcnt_) const;

	bool IsEmpty() const;

	bool IsFull() const;

private:
	void IncrementReadOffset();
	void IncrementWriteOffset();

	PacketData** m_Data;
	uint32_t m_ReadOffset;
	uint32_t m_WriteOffset;
	uint32_t m_Capacity;
};