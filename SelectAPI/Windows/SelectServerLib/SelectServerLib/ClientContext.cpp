#include "ClientContext.hpp"

ClientContext::ClientContext(SOCKET s_)
{
	bool bRet = recvBuffer.Init(MAX_RECV_SLIDEBUFFER_SIZE);

	if (!bRet)
	{
		std::cerr << "ClientContext::Constructor : recvBuf init Failed.\n";
	}

	bRet = sendBuffer.Init(MAX_SEND_SLIDEBUFFER_SIZE);

	if (!bRet)
	{
		std::cerr << "ClientContext::Constructor : sendBuf init Failed.\n";
	}

	bRet = Init(s_);
	if (!bRet)
	{
		std::cerr << "ClientContext::Constructor : Socket init Failed.\n";
	}
}

ClientContext::~ClientContext()
{
	if (m_Socket != INVALID_SOCKET)
	{
		Close();
	}
}

bool ClientContext::Init(SOCKET s_)
{
	if (m_Socket != INVALID_SOCKET)
	{
		Close();
	}

	m_Socket = s_;

	u_long nonBlocking = 1;
	if (ioctlsocket(m_Socket, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
		std::cerr << "ClientSocket::Init : ioctlsocket(FIONBIO) failed: " << WSAGetLastError() << "\n";
		closesocket(m_Socket);
		m_Socket = INVALID_SOCKET;
		return false;
	}

	int flag = 1;
	if (setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)) == SOCKET_ERROR) {
		std::cerr << "ClientSocket::Init : setsockopt(TCP_NODELAY) failed: " << WSAGetLastError() << "\n";
		closesocket(m_Socket);
		m_Socket = INVALID_SOCKET;
		return false;
	}

	return true;
}

void ClientContext::Close()
{
	if (m_Socket != INVALID_SOCKET)
	{
		closesocket(m_Socket);
		m_Socket = INVALID_SOCKET;
	}

	recvBuffer.Clear();
	sendBuffer.Clear();
}

bool ClientContext::StorePartialMsg(const char* pMsg_, uint32_t ioSize_)
{
	return recvBuffer.Enqueue(pMsg_, ioSize_);
}

bool ClientContext::GetReqMsg(std::string_view& out_)
{
	uint32_t uRet = recvBuffer.Peek();

	if (uRet == 0)
	{
		return false;
	}

	if (uRet > recvBuffer.GetSize())
	{
		return false;
	}

	out_ = std::string_view(recvBuffer.GetBuf(), uRet);

	return true;
}

bool ClientContext::PopMsg(std::string_view& sv_)
{
	return recvBuffer.Pop(sv_.size());
}

SendStatus ClientContext::SendMsg(PacketData* pPacket_)
{
	if (pPacket_ == nullptr)
	{
		return SendStatus::FAILED;
	}
	
	bool bRet = sendBuffer.Enqueue(pPacket_->GetData(), pPacket_->GetSize());

	if (!bRet)
	{
		// 버퍼에 담는것도 실패했으므로
		return SendStatus::FAILED;
	}

	int nRet = send(m_Socket, sendBuffer.GetBuf(), sendBuffer.GetSize(), 0);

	if (nRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK) {
			// 송신 불가능 -> 나중에 다시 시도해야 함
		}
		std::cerr << "ClientContext::SendMsg : err : " << err << '\n';

		return SendStatus::PARTIAL_SUCCESS;
	}

	sendBuffer.Pop(nRet);

	if (sendBuffer.IsEmpty())
	{
		// 모두 송신 성공
		return SendStatus::WHOLE_SUCCESS;
	}

	// 일단 송신하지 못했어도 L7버퍼에 담았다면
	return SendStatus::PARTIAL_SUCCESS;
}

SendStatus ClientContext::ResumeSend()
{
	if (sendBuffer.IsEmpty())
	{
		return SendStatus::WHOLE_SUCCESS;
	}
	int nRet = send(m_Socket, sendBuffer.GetBuf(), sendBuffer.GetSize(), 0);

	if (nRet == SOCKET_ERROR)
	{
		int err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK) {
			// 송신 불가능 -> 나중에 다시 시도해야 함
		}
		std::cerr << "ClientContext::SendMsg : err : " << err << '\n';

		return SendStatus::PARTIAL_SUCCESS;
	}

	sendBuffer.Pop(nRet);

	if (sendBuffer.IsEmpty())
	{
		// 모두 송신 성공
		return SendStatus::WHOLE_SUCCESS;
	}

	// 일단 송신하지 못했어도 L7버퍼에 담았다면
	return SendStatus::PARTIAL_SUCCESS;
}