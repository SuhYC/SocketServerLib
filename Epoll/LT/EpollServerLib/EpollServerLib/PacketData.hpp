#pragma once

#include "Define.hpp"

#include <stdint.h>
#include <string>
#include <new>
#include <iostream>
#include <cstring>


class PacketData
{
public:
	PacketData();
	~PacketData();

	/// <summary>
	/// 버퍼에 적힌 데이터를 초기화합니다.
	/// (버퍼를 조작하지는 않음)
	/// </summary>
	void Clear();

	/// <summary>
	/// 할당된 버퍼 메모리를 해제합니다.
	/// </summary>
	void Free();

	/// <summary>
	/// 버퍼를 재할당합니다.
	/// 기존 버퍼가 있었다면 해제하며 데이터는 제거됩니다.
	/// </summary>
	/// <param name="size_"></param>
	/// <returns></returns>
	bool Allocate(uint32_t size_ = PACKET_SIZE);

	/// <summary>
	/// 객체 단위 직렬화가 가능한 경우(동적메모리x) 컴파일되며,
	/// 버퍼에 데이터를 복사합니다.
	/// 네트워크IO에 사용할 수 있도록 길이헤더가 부착됩니다.
	/// 
	/// POD객체에 사용해주세요.
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <param name="objData_"></param>
	/// <returns></returns>
	template <typename T>
	typename std::enable_if<std::is_trivially_copyable<T>::value&& std::is_standard_layout<T>::value, bool>::type
		Init(InfoCode infoCode_, uint32_t reqNo_, T& objData_)
	{
		uint32_t size = sizeof(T);
		uint32_t totalSize = size + sizeof(InfoHeader);
		if (totalSize > PACKET_SIZE)
		{
			return false;
		}

		int32_t nInfoCode = static_cast<int32_t>(infoCode_);
		m_Size = 0;

		// totalSize
		memcpy(m_pData + m_Size, &totalSize, sizeof(uint32_t));
		m_Size += sizeof(uint32_t);

		// infoCode
		memcpy(m_pData + m_Size, &nInfoCode, sizeof(int32_t));
		m_Size += sizeof(int32_t);

		// reqNo
		memcpy(m_pData + m_Size, &reqNo_, sizeof(uint32_t));
		m_Size += sizeof(uint32_t);

		// payload
		if (size > 0)
		{
			memcpy(m_pData + m_Size, &objData_, sizeof(T));
			m_Size += sizeof(T);
		}

		return true;
	}

	/// <summary>
	/// 버퍼에 데이터를 복사합니다
	/// 네트워크IO에 사용할 수 있도록
	/// 길이헤더가 부착됩니다.
	/// </summary>
	/// <param name="strData_"></param>
	/// <returns></returns>
	bool Init(InfoCode infoCode_, uint32_t reqNo_, std::string& strData_);

	/// <summary>
	/// 버퍼에 데이터를 복사합니다
	/// 네트워크IO에 사용할 수 있도록
	/// 길이헤더가 부착됩니다.
	/// 
	/// dataSize_가 pData_의 크기를 넘지 않도록 주의.
	/// </summary>
	/// <param name="infoCode_"></param>
	/// <param name="reqNo_"></param>
	/// <param name="pData_"></param>
	/// <param name="dataSize_"></param>
	/// <returns></returns>
	bool Init(InfoCode infoCode_, uint32_t reqNo_, const char* pData_, uint32_t dataSize_);

	/// <summary>
	/// 페이로드 없이 응답코드와 요청번호만 복사합니다.
	/// </summary>
	/// <param name="infoCode_"></param>
	/// <param name="reqNo_"></param>
	/// <returns></returns>
	bool Init(InfoCode infoCode_, uint32_t reqNo_);

	/// <summary>
	/// 버퍼 반환
	/// </summary>
	/// <returns>버퍼의 포인터</returns>
	char* GetData();

	/// <summary>
	/// 버퍼에 저장된 데이터의 길이 반환
	/// </summary>
	/// <returns>저장된 데이터의 길이</returns>
	uint32_t GetSize() const;

private:
	char* m_pData;
	uint32_t m_Size;
	uint32_t m_Capacity;
};