#include "ClientContext.hpp"

ClientContext::ClientContext()
{

}

ClientContext::~ClientContext()
{

}

bool ClientContext::Init(const int fd_)
{
	if (fd_ < 0)
	{
		std::cerr << "ClientContext::Init : Invalid fd.\n";
		return false;
	}

	if (m_Socket != -1)
	{
		Close();
	}

	m_Socket = fd_;
	ep_ev.events = EPOLLIN;
	ep_ev.data.fd = fd_;

	int flag = fcntl(m_Socket, F_GETFL, 0);
	if (flag == -1)
	{

		return false;
	}

	if (fcntl(m_Socket, F_SETFL, flag | O_NONBLOCK) == -1)
	{
		std::cerr << "ClientContext::Init : Failed to Set NonBlocking.\n";
		return false;
	}

	int nodelay = 1;

	if (setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)) == -1)
	{
		std::cerr << "ClientContext::Init : Failed to Set NoDelay.\n";
		return false;
	}

	bool bRet = recvBuffer.Init(MAX_RECV_SLIDEBUFFER_SIZE);
	if (!bRet)
	{
		std::cerr << "ClientContext::Init : Failed to Allocate recvBuffer.\n";
		return false;
	}

	bRet = sendBuffer.Init(MAX_SEND_SLIDEBUFFER_SIZE);
	if (!bRet)
	{
		std::cerr << "ClientContext::Init : Failed to Allocate sendBuffer.\n";
		return false;
	}


	return true;
}

epoll_event& ClientContext::GetEpollEvent()
{
	return ep_ev;
}

void ClientContext::Close()
{
	if (m_Socket == -1)
	{
		return;
	}

	close(m_Socket);
	m_Socket = -1;
}

bool ClientContext::StorePartialMsg(const char* pMsg_, uint32_t ioSize_)
{
	return recvBuffer.Enqueue(pMsg_, ioSize_);
}

bool ClientContext::GetReqMsg(std::string_view& out_)
{
	if (recvBuffer.GetSize() < 5)
	{
		return false;
	}

	if (recvBuffer.GetSize() < recvBuffer.Peek())
	{
		return false;
	}

	out_ = std::string_view(recvBuffer.GetBuf(), recvBuffer.Peek());
	return true;
}

bool ClientContext::PopMsg(std::string_view& sv_)
{
	return recvBuffer.Pop(sv_.size());
}

SendStatus ClientContext::SendMsg(PacketData* pPacket_)
{
	if (pPacket_ == nullptr || m_Socket < 0)
	{
		return SendStatus::FAILED;
	}

	SpinLockGuard guard(m_SendLock);

	sendBuffer.Enqueue(pPacket_->GetData(), pPacket_->GetSize());

	ssize_t nRet = send(m_Socket, sendBuffer.GetBuf(), sendBuffer.GetSize(), 0);

	if (nRet == -1)
	{
		std::cerr << "ClientContext::SendMsg : send Failed.\n";
		return SendStatus::FAILED;
	}

	sendBuffer.Pop(nRet);

	if (sendBuffer.IsEmpty())
	{
		return SendStatus::WHOLE_SUCCESS;
	}
	else
	{
		return SendStatus::PARTIAL_SUCCESS;
	}
}

SendStatus ClientContext::ResumeSend()
{
	SpinLockGuard guard(m_SendLock);

	ssize_t nRet = send(m_Socket, sendBuffer.GetBuf(), sendBuffer.GetSize(), 0);

	if (nRet == -1)
	{
		std::cerr << "ClientContext::SendMsg : send Failed.\n";
		return SendStatus::FAILED;
	}

	sendBuffer.Pop(nRet);

	if (sendBuffer.IsEmpty())
	{
		return SendStatus::WHOLE_SUCCESS;
	}
	else
	{
		return SendStatus::PARTIAL_SUCCESS;
	}
}