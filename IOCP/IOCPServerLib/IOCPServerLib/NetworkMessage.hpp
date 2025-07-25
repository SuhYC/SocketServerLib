#pragma once

#include "Define.hpp"

const uint32_t MAX_ECHO_SIZE = 80;

struct EchoParameter
{
	uint32_t PayloadSize;
	char Payload[MAX_ECHO_SIZE];
};