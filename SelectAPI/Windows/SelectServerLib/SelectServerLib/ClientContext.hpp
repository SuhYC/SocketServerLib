#pragma once

#include "Define.hpp"

#include "SlideBuffer.hpp"
#include "PacketData.hpp"
#include <string_view>


/// <summary>
/// 경쟁상태 미고려.
/// 반드시 하나의 실행흐름으로 접근할것
/// </summary>
class ClientContext
{
public:
	ClientContext(SOCKET s_);
	~ClientContext();

	/// <summary>
	/// 연결객체를 초기화하는 함수
	/// </summary>
	bool Init(SOCKET s_);

	/// <summary>
	/// 소켓을 닫는 함수
	/// </summary>
	void Close();

	/// <summary>
	/// 수신한 정보를 버퍼에 담는 함수
	/// </summary>
	/// <param name="pMsg_"></param>
	/// <param name="ioSize_"></param>
	/// <returns></returns>
	bool StorePartialMsg(const char* pMsg_, uint32_t ioSize_);

	/// <summary>
	/// 버퍼에 담아둔 수신정보를 메시지 단위로 끊어 out버퍼에 담아주는 함수
	/// </summary>
	/// <param name="out_"></param>
	/// <param name="maxBufferSize_"></param>
	/// <returns></returns>
	bool GetReqMsg(std::string_view& out_);

	/// <summary>
	/// 버퍼에서 해당하는 메시지만큼을 제거하는 함수
	/// sv.size()만큼 제거한다.
	/// </summary>
	/// <param name="sv_"></param>
	/// <returns></returns>
	bool PopMsg(std::string_view& sv_);

	/// <summary>
	/// 처리 결과를 송신하는 함수.
	/// </summary>
	/// <param name="pPacket_"></param>
	/// <returns>-1 : Failed, 0 : Success, >0 : Partial Success</returns>
	SendStatus SendMsg(PacketData* pPacket_);

	/// <summary>
	/// 커널버퍼가 가득차서 실패했던 송신을 재개하는 경우 호출
	/// </summary>
	/// <returns></returns>
	SendStatus ResumeSend();

private:

	SOCKET m_Socket;

	SlideBuffer recvBuffer;
	SlideBuffer sendBuffer;
};