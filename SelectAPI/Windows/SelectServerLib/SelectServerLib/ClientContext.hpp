#pragma once

#include "Define.hpp"

#include "SlideBuffer.hpp"
#include "PacketData.hpp"
#include <string_view>


/// <summary>
/// ������� �̰��.
/// �ݵ�� �ϳ��� �����帧���� �����Ұ�
/// </summary>
class ClientContext
{
public:
	ClientContext(SOCKET s_);
	~ClientContext();

	/// <summary>
	/// ���ᰴü�� �ʱ�ȭ�ϴ� �Լ�
	/// </summary>
	bool Init(SOCKET s_);

	/// <summary>
	/// ������ �ݴ� �Լ�
	/// </summary>
	void Close();

	/// <summary>
	/// ������ ������ ���ۿ� ��� �Լ�
	/// </summary>
	/// <param name="pMsg_"></param>
	/// <param name="ioSize_"></param>
	/// <returns></returns>
	bool StorePartialMsg(const char* pMsg_, uint32_t ioSize_);

	/// <summary>
	/// ���ۿ� ��Ƶ� ���������� �޽��� ������ ���� out���ۿ� ����ִ� �Լ�
	/// </summary>
	/// <param name="out_"></param>
	/// <param name="maxBufferSize_"></param>
	/// <returns></returns>
	bool GetReqMsg(std::string_view& out_);

	/// <summary>
	/// ���ۿ��� �ش��ϴ� �޽�����ŭ�� �����ϴ� �Լ�
	/// sv.size()��ŭ �����Ѵ�.
	/// </summary>
	/// <param name="sv_"></param>
	/// <returns></returns>
	bool PopMsg(std::string_view& sv_);

	/// <summary>
	/// ó�� ����� �۽��ϴ� �Լ�.
	/// </summary>
	/// <param name="pPacket_"></param>
	/// <returns>-1 : Failed, 0 : Success, >0 : Partial Success</returns>
	SendStatus SendMsg(PacketData* pPacket_);

	/// <summary>
	/// Ŀ�ι��۰� �������� �����ߴ� �۽��� �簳�ϴ� ��� ȣ��
	/// </summary>
	/// <returns></returns>
	SendStatus ResumeSend();

private:

	SOCKET m_Socket;

	SlideBuffer recvBuffer;
	SlideBuffer sendBuffer;
};