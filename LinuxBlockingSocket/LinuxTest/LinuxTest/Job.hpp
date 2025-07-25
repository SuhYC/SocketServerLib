#pragma once

#include <stdint.h>
#include <optional>
#include <string_view>
#include <functional>
#include "PacketData.hpp"
#include <iostream>

/// <summary>
/// 해당 추상클래스를 상속하여 구체적인 요청과 처리과정을 구현.
/// 
/// </summary>
class Job
{
public:
	Job(uint32_t userindex_, uint32_t reqNo_);
	virtual ~Job() = default;

	/// <summary>
	/// 바이너리데이터를 바탕으로 작업객체의 파라미터를 초기화하는 함수
	/// </summary>
	/// <param name="param_"></param>
	/// <returns></returns>
	virtual bool Parse(const std::string_view& param_) = 0;

	/// <summary>
	/// 초기화된 작업객체의 파라미터를 바탕으로 요청을 처리하고 결과 응답하는 함수.
	/// 
	/// InfoCode::NOT_FINISHED를 반환하여 JobQueue에 재삽입할 수 있다.
	/// </summary>
	/// <returns></returns>
	virtual InfoCode Execute() = 0;

	static std::function<InfoCode(const uint32_t, PacketData*)> SendMsgFunc;
protected:
	uint16_t m_UserIndex;
	uint32_t m_ReqNo;

	InfoCode SendResultMsg(const uint32_t userindex_, PacketData* pPacket_)
	{
		if (SendMsgFunc == nullptr)
		{
			std::cerr << "Job::SendResultMsg : Func Pointer is nullptr.\n";
			return InfoCode::NULLPTR_ON_FUNCPTR;
		}

		if (pPacket_ == nullptr)
		{
			std::cerr << "Job::SendResultMsg : SendMsg Func Requires Packet.\n";
			return InfoCode::OTHER_ERR;
		}

		return SendMsgFunc(userindex_, pPacket_);
	}
};


/// <summary>
/// 예시용.
/// </summary>
class EchoJob : public Job
{
public:
	EchoJob(uint32_t userindex_, uint32_t reqNo_);

	bool Parse(const std::string_view& param_) override;

	InfoCode Execute() override;

private:
	EchoParameter m_param;
};