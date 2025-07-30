#pragma once

#include "GameServer.hpp"
#include <conio.h>

int main()
{
	GameServer server(12345);

	server.Start();

	int a = _getch();

	server.End();
}