#pragma once

#include <thread>
#include <functional>
#include <iostream>

struct ClientContext
{
public:
	ClientContext(int fd_);

	bool Init(std::function<void()> func_);

	void check();
	bool GetStatus();

	bool& GetAliveRef() { return isAlive; }

	int fd;
	std::thread th;

private:
	bool isAlive; // �������� ���� �����.
	bool isTargeted; // �������
};