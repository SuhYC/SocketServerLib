#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#include <stdint.h>


// -- Connection
constexpr uint32_t MAX_SEND_SLIDEBUFFER_SIZE = 1024 * 32;
constexpr uint32_t MAX_RECV_SLIDEBUFFER_SIZE = 1024;

constexpr uint32_t PACKET_SIZE = 4096;

enum class InfoCode
{
	REQ_SUCCESS,
	REQ_FAILED,
	NOT_FINISHED, // �������� ���� ó������ ���� �۾��� ���� �ٽ� ť��. Ŭ���̾�Ʈ�� �������� ����.
	NULLPTR_ON_FUNCPTR,
	OTHER_ERR, // ��û�� ���ռ��� ������ ������ ���� �߻�. (��������)
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
	WHOLE_SUCCESS, // L7������ ������ ��� �۽��ϴµ� ����
	PARTIAL_SUCCESS, // L7���ۿ� ���� �������� ���� �����Ͱ� ���� ä ����
	FAILED // L7���ۿ� ������ ���ϰ� ����
};

constexpr uint32_t MAX_ECHO_SIZE = 80;

struct EchoParameter
{
	uint32_t PayloadSize;
	char Payload[MAX_ECHO_SIZE];
};