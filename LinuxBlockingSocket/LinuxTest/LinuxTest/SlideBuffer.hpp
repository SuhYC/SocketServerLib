#pragma once

#include <stdint.h>
#include <exception>
#include <iostream>
#include <string.h>

/// <summary>
/// lock-free하지 않습니다. 상호배제 필요.
/// </summary>
class SlideBuffer final
{
public:
	SlideBuffer();
	~SlideBuffer();

	/// <summary>
	/// 슬라이드버퍼에 담긴 메시지를 비웁니다.
	/// 메모리는 변경되지 않습니다.
	/// </summary>
	/// <returns></returns>
	void Clear();

	/// <summary>
	/// 슬라이드버퍼에 메모리를 할당합니다.
	/// 기존에 할당된 메모리가 있었다면 해제 후 할당하며
	/// 기존 메모리에 작성했던 데이터는 보존되지 않습니다.
	/// </summary>
	/// <param name="capacity_">신규 할당할 메모리의 크기</param>
	/// <returns>재할당 성공 여부</returns>
	bool Init(const uint32_t capacity_);

	/// <summary>
	/// 슬라이드버퍼에 데이터를 삽입합니다.
	/// 버퍼의 남은 공간보다 큰 데이터를 삽입하면 실패합니다.
	/// 데이터로 nullptr를 넘기면 실패합니다.
	/// </summary>
	/// <param name="in_">삽입할 데이터의 포인터</param>
	/// <param name="size_">삽입할 데이터의 크기</param>
	/// <returns>삽입 성공 여부</returns>
	bool Enqueue(const char* in_, const uint32_t size_);

	/// <summary>
	/// 슬라이드버퍼로부터 데이터를 복사합니다.
	/// 출력버퍼로 이동된 데이터는 슬라이드버퍼에서 삭제됩니다.
	/// </summary>
	/// <param name="out_">출력버퍼의 포인터</param>
	/// <param name="maxSize_">출력버퍼의 크기</param>
	/// <returns>복사된 데이터의 크기</returns>
	uint32_t Dequeue(char* out_, const uint32_t maxSize_);

	/// <summary>
	/// 슬라이드버퍼 내의 데이터를 특정 크기만큼 삭제합니다.
	/// 내부 데이터의 크기보다 큰 데이터를 삭제하려고 하면 실패합니다.
	/// </summary>
	/// <param name="size_">삭제할 데이터의 크기</param>
	/// <returns>삭제 성공 여부</returns>
	bool Pop(const uint32_t size_);

	/// <summary>
	/// 슬라이드버퍼의 가장 선두에 있는 4바이트를 uint32_t로 파싱하여 반환합니다.
	/// 슬라이드버퍼에 담을 데이터의 헤더로 메시지의 크기를 구분할 것이기 때문에 작성.
	/// 내부 데이터가 5바이트 미만인 경우 0을 반환합니다. (길이헤더 이하의 길이기 때문)
	/// 
	/// 수신버퍼에서 사용할 것.
	/// 송신버퍼에서는 헤더와 상관없이 보낼 수 있는 최대크기의 데이터를 송신 예약한다.
	/// </summary>
	/// <returns>첫 메시지의 크기</returns>
	uint32_t Peek();

	/// <summary>
	/// 슬라이드버퍼에 담긴 모든 데이터의 크기의 합을 반환합니다.
	/// </summary>
	/// <returns>담긴 모든 데이터의 크기 합</returns>
	uint32_t GetSize();

	bool IsEmpty();

	/// <summary>
	/// 슬라이드버퍼가 담을 수 있는 한계용량을 반환합니다.
	/// Init(const uint64_t)을 통해 한계용량을 변경할 수 있습니다. 
	/// </summary>
	/// <returns>현재의 한계용량</returns>
	uint32_t GetCapacity();

	/// <summary>
	/// 버퍼를 반환합니다.
	/// 
	/// WSABUF.buf에 연결하기위해 char*타입으로 설정하였으나
	/// 외부에서 데이터를 직접 조작하지 않도록 주의.
	/// </summary>
	/// <returns>버퍼 포인터</returns>
	char* GetBuf();

private:
	char* m_pData;
	uint32_t m_Size;
	uint32_t m_Capacity;
};