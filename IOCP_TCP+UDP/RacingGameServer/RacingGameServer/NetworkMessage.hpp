#pragma once

#include <stdint.h>
#include <string>
#include "Define.hpp"

/// <summary>
/// 클라이언트의 요청코드.
/// 클라이언트의 이넘값과 동일하게 구성할 것.
/// 신규값을 추가할 때는 반드시 기존값을 건드리지 않고 마지막에 추가한 후 LAST에 지정할것.
/// </summary>
enum class ReqType
{
	ECHO,
	REQ_TOKEN,
	UDPECHO,
	LAST = UDPECHO
};

struct ReqMessage
{
	ReqHeader header;
	uint32_t payloadSize;
	char payload[MAX_PAYLOAD_SIZE];
};

struct ResMessage
{
	InfoHeader header;
	char payload[MAX_PAYLOAD_SIZE];
};

struct UDPReqMessage
{
	UDPHeader header;
	uint32_t payloadSize;
	char payload[MAX_PAYLOAD_SIZE];
};

const uint32_t MAX_ECHO_MSG_SIZE = 80;

struct EchoParameter
{
	uint32_t msgSize;
	char msg[MAX_ECHO_MSG_SIZE];
};

struct ReqTokenRes
{
	uint64_t token;
};