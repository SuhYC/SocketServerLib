#pragma once

#include <type_traits>
#include <stdint.h>
#include <cstring>
#include <new>

constexpr uint32_t MAX_PACKET_SIZE = 4096;

enum class InfoCode
{
	REQ_SUCCESS,
	REQ_FAILED,
	NOT_FINISHED, // 서버에서 아직 처리되지 않은 작업에 대해 다시 큐잉. 클라이언트로 전송하지 않음.
	NULLPTR_ON_FUNCPTR,
	OTHER_ERR, // 요청의 적합성과 별개로 임의의 오류 발생. (서버문제)
};

struct InfoHeader
{
	uint32_t msgSize;
	int32_t resCode;
	uint32_t reqNo;
};

struct ReqHeader
{
	uint32_t msgSize;
	int32_t reqType;
	uint32_t reqNo;
};

const uint32_t MAX_ECHO_SIZE = 80;

struct EchoParameter
{
	uint32_t PayloadSize;
	char Payload[MAX_ECHO_SIZE];
};

class PacketData
{
public:
	PacketData();
	~PacketData();
	
	void Clear();
	void Free();

	template <typename T>
	typename std::enable_if<std::is_trivially_copyable<T>::value&& std::is_standard_layout<T>::value, bool>::type
	Init(InfoCode infoCode_, uint32_t reqNo_, T& objData_)
	{
		uint32_t size = sizeof(T);
		uint32_t totalSize = size + sizeof(InfoHeader);
		if (totalSize > MAX_PACKET_SIZE)
		{
			return false;
		}

		int32_t nInfoCode = static_cast<int32_t>(infoCode_);
		m_uSize = 0;

		// totalSize
		memcpy(m_pData + m_uSize, &totalSize, sizeof(uint32_t));
		m_uSize += sizeof(uint32_t);

		// infoCode
		memcpy(m_pData + m_uSize, &nInfoCode, sizeof(int32_t));
		m_uSize += sizeof(int32_t);

		// reqNo
		memcpy(m_pData + m_uSize, &reqNo_, sizeof(uint32_t));
		m_uSize += sizeof(uint32_t);

		// payload
		if (size > 0)
		{
			memcpy(m_pData + m_uSize, &objData_, sizeof(T));
			m_uSize += sizeof(T);
		}

		return true;
	}

	char* GetData();
	uint32_t GetSize() const;

private:
	char* m_pData;
	uint32_t m_uSize;
	uint32_t m_uCapacity;
};