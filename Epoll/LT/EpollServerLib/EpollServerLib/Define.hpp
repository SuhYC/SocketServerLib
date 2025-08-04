#pragma once

#include <stdint.h>


// -- EpollServer
constexpr uint32_t EPOLL_SIZE = 50; // 2.6.8 버전 이상의 리눅스에서는 큰 의미 없음
constexpr uint32_t EPOLL_EVENT_SIZE = 64;

// -- Connection
constexpr uint32_t MAX_SEND_SLIDEBUFFER_SIZE = 1024 * 32;
constexpr uint32_t MAX_RECV_SLIDEBUFFER_SIZE = 1024;

constexpr uint32_t PACKET_SIZE = 4096;

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

enum class ReqType
{
	ECHO,
	LAST = ECHO
};

enum class SendStatus
{
	WHOLE_SUCCESS, // L7버퍼의 내용을 모두 송신하는데 성공
	PARTIAL_SUCCESS, // L7버퍼에 아직 전송하지 못한 데이터가 남은 채 성공
	FAILED // L7버퍼에 담지도 못하고 실패
};

constexpr uint32_t MAX_ECHO_SIZE = 80;

struct EchoParameter
{
	uint32_t PayloadSize;
	char Payload[MAX_ECHO_SIZE];
};