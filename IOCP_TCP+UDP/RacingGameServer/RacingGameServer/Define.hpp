#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include <stdint.h>
#include <MSWSock.h> // AcceptEx()
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib") // acceptEx()

#include "LogManager.hpp"

// -- GameServer Init

const uint16_t SERVER_PORT = 12345;
const uint16_t UDP_PORT = 8080;
const uint32_t MAX_CLIENT = 100;

// -- IOCP

const unsigned int MAX_SOCKBUF = 4096;
const unsigned short MAX_WORKTHREAD = 4;
const unsigned short MAX_JOBTHREAD = 4;

enum class eIOOperation
{
	TCPRECV,
	UDPRECV,
	TCPSEND,
	UDPSEND,
	ACCEPT
};

const uint16_t MAX_WSABUFS = 16;

struct stOverlappedEx
{
	WSAOVERLAPPED m_overlapped{};
	unsigned short m_userIndex;
	WSABUF m_wsaBuf[MAX_WSABUFS]{};
	eIOOperation m_eOperation;
	sockaddr_in clientAddr{}; // for UDP Send, Recv
	int addrlen = sizeof(clientAddr);
	DWORD flags;
};

// -- Connection
const int MAX_RECV_SLIDEBUFFER_SIZE = MAX_SOCKBUF;
const uint32_t MAX_SEND_SLIDEBUFFER_SIZE = MAX_SOCKBUF * 16;
const uint32_t MAX_TCP_SEGMENT_SIZE = 1460;
const uint32_t MAX_UDP_DATAGRAM_SIZE = 1400; // 그냥 안전하게 1400으로 함.

// -- PacketData
const uint32_t PACKET_SIZE = MAX_SOCKBUF * 2;

// -- PacketPool
const uint32_t INIT_PACKET_COUNT = 100;

// -- NetworkMsg
const uint32_t HEADER_SIZE = 12;
const uint32_t MAX_PAYLOAD_SIZE = PACKET_SIZE - HEADER_SIZE;


// -- MemoryPool (JobFactory)
const uint32_t MAX_MEMORY_BLOCKS = 100;


enum class JobProcess
{
	PARSING,
	EXECUTE,
	SEND
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
	NOT_MY_TURN, // try_lock으로 락을 획득하지 못했음.
};

struct InfoHeader // for tcp send (server -> client)
{
	uint32_t msgSize;
	int32_t resCode;
	uint32_t reqNo;
};

struct ReqHeader // for tcp recv (client -> server)
{
	uint32_t msgSize;
	int32_t reqType;
	uint32_t reqNo;
};

struct UDPHeader
{
	uint32_t msgSize;
	int32_t reqType;
	uint64_t token; // To Identify Client and Mapping on TCPSocket
};

enum class SendProtocol
{
	TCP,
	UDP
};

// -- Macro


// try-catch 문 마지막에 공통으로 작성할 catch문
// try-catch 문을 최소화하되, 사용하는 곳에는 마지막에 항상 해당 매크로를 삽입하자.
// 해당 catch문에 예외가 잡히더라도 로그를 찍은 후 진행한다. 이점 유의
#define DEFAULT_CATCH()																\
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
