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
/// 클라이언트의 요청코드.
/// 클라이언트의 이넘값과 동일하게 구성할 것.
/// 신규값을 추가할 때는 반드시 기존값을 건드리지 않고 마지막에 추가한 후 LAST에 지정할것.
/// </summary>
enum class ReqType
{
	ECHO,
	LAST = ECHO
};

/// <summary>
/// 서버에서의 처리 결과를 담는 코드.
/// NOT_FINISHED는 클라이언트로 전송하지 말고 작업큐에 다시 담을 것.
/// </summary>
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

// -- Macro


// try-catch 문 마지막에 공통으로 작성할 catch문
// try-catch 문을 최소화하되, 사용하는 곳에는 마지막에 항상 해당 매크로를 삽입하자.
// 해당 catch문에 예외가 잡히더라도 로그를 찍은 후 진행한다. 이점 유의
#define DEFAULT_CATCH()																	\
catch (const std::exception& e) {													\
    LogManager::Log(Criticality::ERR, std::string(__FUNCSIG__) + ": ", e.what());	\
}																					\
catch (...) {																		\
    LogManager::Log(Criticality::ERR, std::string(__FUNCSIG__) + ": Unknown Err."); \
}

// LogManager::Log 함수를 함수시그니처와 함께 입력해주는 매크로
// Criticality::ERR로 호출한다. (PrintLevel::NO_PRINT가 아니라면 무조건 출력한다.)
#define LOG_ERR(...) \
LogManager::Log(Criticality::ERR, std::string(__FUNCSIG__) + " : ", ##__VA_ARGS__)

// LogManager::Log 함수를 함수시그니처와 함께 입력해주는 매크로
// Criticality::DEBUG로 호출한다. (PrintLevel::ALL인 경우만 출력한다.
#define LOG_DEBUG(...) \
LogManager::Log(Criticality::DEBUG, std::string(__FUNCSIG__) + " : ", ##__VA_ARGS__)
