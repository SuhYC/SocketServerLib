#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <WS2tcpip.h>
#include <WinSock2.h>
#include <iostream>

#include <string>
#include <vector>
#include <stdint.h>

#pragma comment(lib, "ws2_32.lib")

enum class ReqType
{
    ECHO,
    LAST = ECHO
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

SOCKET Init()
{
    WSADATA wsaData;
    int iRet = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (iRet != 0)
    {
        std::cerr << "WSAStartup failed\n";
        return INVALID_SOCKET;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "SOCKET creation falied\n";

        return INVALID_SOCKET;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    iRet = connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (iRet == SOCKET_ERROR) {
        std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    return clientSocket;
}

bool CreateMsg(char* buffer_, int maxBufferLen_, ReqHeader& header_, EchoParameter& param_)
{
    if (sizeof(header_) + sizeof(param_) > maxBufferLen_)
    {
        return false;
    }

    ZeroMemory(buffer_, maxBufferLen_);

    CopyMemory(buffer_, &header_, sizeof(ReqHeader));
    CopyMemory(buffer_ + sizeof(ReqHeader), &param_, sizeof(EchoParameter));

    return true;
}

int main()
{
    SOCKET clientSocket = Init();

    if (clientSocket == INVALID_SOCKET)
    {
        return 0;
    }

    char buf[2048]{};
    const char* msg = "Hi Hello Annyung?";

    ReqHeader header{sizeof(ReqHeader) + sizeof(EchoParameter), static_cast<int32_t>(ReqType::ECHO), 1};
    EchoParameter param{};

    param.PayloadSize = strlen(msg);
    CopyMemory(param.Payload, msg, param.PayloadSize);

    CopyMemory(buf, &header, sizeof(ReqHeader));
    CopyMemory(buf + sizeof(ReqHeader), &param, sizeof(EchoParameter));

    int nRet = send(clientSocket, buf, header.msgSize, 0);

    if (nRet == SOCKET_ERROR)
    {
        std::cerr << "송신에러\n";
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }

    ZeroMemory(buf, 2048);

    nRet = recv(clientSocket, buf, 2047, 0);

    if (nRet == 0)
    {
        std::cerr << "연결 해제\n";
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }
    else if (nRet > 0 && nRet < 2048)
    {
        buf[nRet] = NULL;
    }
    else
    {
        std::cerr << "수신에러\n";
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }

    EchoParameter result{};

    CopyMemory(&result, buf + sizeof(InfoHeader), nRet - sizeof(InfoHeader));

    std::cout << result.Payload;

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}