#include "ClientContext.hpp"


ClientContext::ClientContext(int fd_) : fd(fd_), isAlive(true), isTargeted(false)
{

}

bool ClientContext::Init(std::function<void()> func_)
{
	if (th.joinable())
	{
		std::cerr << "ClientContext::Init : Already Have Func On Thread.\n";
		return false;
	}

	th = std::thread(func_);
	return true;
}

void ClientContext::check()
{
	if (isAlive == false)
	{
		isTargeted = true;
		th.join();
	}
}

bool ClientContext::GetStatus()
{
	return isTargeted;
}