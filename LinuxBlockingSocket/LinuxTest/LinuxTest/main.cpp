#pragma once

#include "GameServer.hpp"

#include <cstdio>

constexpr uint16_t PORT = 12345;

int main()
{
	GameServer server(PORT);

	server.Start();

	int c = getchar();

	server.End();

	return 0;
}