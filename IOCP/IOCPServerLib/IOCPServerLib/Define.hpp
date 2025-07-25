#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <stdint.h>
#include "LogManager.hpp"

// -- GameServer Init

const uint32_t SERVER_PORT = 12345;
const uint32_t MAX_CLIENT = 100;

// -- IOCP

const unsigned int MAX_SOCKBUF = 4096;
const unsigned short MAX_WORKTHREAD = 4;
const unsigned short MAX_JOBTHREAD = 4;

enum class eIOOperation
{
	RECV,
	SEND,
	ACCEPT
};

struct stOverlappedEx
{
	WSAOVERLAPPED m_overlapped;
	unsigned short m_userIndex;
	WSABUF m_wsaBuf;
	eIOOperation m_eOperation;
};

// -- Connection
const int MAX_SEND_SLIDEBUFFER_SIZE = MAX_SOCKBUF * 32;
const int MAX_RECV_SLIDEBUFFER_SIZE = MAX_SOCKBUF;

// -- PacketData
const uint32_t PACKET_SIZE = MAX_SOCKBUF * 2;

// -- PacketPool
const uint32_t INIT_PACKET_COUNT = 100;

// -- NetworkMsg
const uint32_t HEADER_SIZE = 12;
const uint32_t MAX_PAYLOAD_SIZE = PACKET_SIZE - HEADER_SIZE;

/// <summary>
/// Ŭ���̾�Ʈ�� ��û�ڵ�.
/// Ŭ���̾�Ʈ�� �̳Ѱ��� �����ϰ� ������ ��.
/// �ű԰��� �߰��� ���� �ݵ�� �������� �ǵ帮�� �ʰ� �������� �߰��� �� LAST�� �����Ұ�.
/// </summary>
enum class ReqType
{
	ECHO,
	LAST = ECHO
};

/// <summary>
/// ���������� ó�� ����� ��� �ڵ�.
/// NOT_FINISHED�� Ŭ���̾�Ʈ�� �������� ���� �۾�ť�� �ٽ� ���� ��.
/// </summary>
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

// -- Macro


// try-catch �� �������� �������� �ۼ��� catch��
// try-catch ���� �ּ�ȭ�ϵ�, ����ϴ� ������ �������� �׻� �ش� ��ũ�θ� ��������.
// �ش� catch���� ���ܰ� �������� �α׸� ���� �� �����Ѵ�. ���� ����
#define DEFAULT_CATCH()																	\
catch (const std::exception& e) {													\
    LogManager::Log(Criticality::ERR, std::string(__FUNCSIG__) + ": ", e.what());	\
}																					\
catch (...) {																		\
    LogManager::Log(Criticality::ERR, std::string(__FUNCSIG__) + ": Unknown Err."); \
}

// LogManager::Log �Լ��� �Լ��ñ״�ó�� �Բ� �Է����ִ� ��ũ��
// Criticality::ERR�� ȣ���Ѵ�. (PrintLevel::NO_PRINT�� �ƴ϶�� ������ ����Ѵ�.)
#define LOG_ERR(...) \
LogManager::Log(Criticality::ERR, std::string(__FUNCSIG__) + " : ", ##__VA_ARGS__)

// LogManager::Log �Լ��� �Լ��ñ״�ó�� �Բ� �Է����ִ� ��ũ��
// Criticality::DEBUG�� ȣ���Ѵ�. (PrintLevel::ALL�� ��츸 ����Ѵ�.
#define LOG_DEBUG(...) \
LogManager::Log(Criticality::DEBUG, std::string(__FUNCSIG__) + " : ", ##__VA_ARGS__)
