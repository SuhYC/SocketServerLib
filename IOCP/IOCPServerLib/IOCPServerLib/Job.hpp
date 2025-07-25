#pragma once

#include <stdint.h>
#include "Define.hpp"
#include <optional>
#include <string_view>
#include <functional>
#include "User.hpp"
#include "Zone.hpp"
#include "ZoneManager.hpp"
#include "UserManager.hpp"
#include "NetworkMessage.hpp"
#include "PacketPool.hpp"

/// <summary>
/// 작업처리에 필요한 의존성 객체를 묶어서 넘기기 위한 구조체.
/// UserManager : L7에서 유저정보를 담을 객체를 관리하는 객체
/// ZoneManager : Zone 기반 동작을 위해 유저들이 존재하는 공간을 관리하는 객체
/// PacketPool : 송신할 메시지를 직렬화할 버퍼를 오브젝트풀로 관리하는 객체.
/// </summary>
struct DIStruct
{
	UserManager* um;
	ZoneManager* zm;
	PacketPool* pp;

	DIStruct& operator=(DIStruct&& rhs_) noexcept
	{
		if (&rhs_ != this)
		{
			this->um = rhs_.um;
			this->zm = rhs_.zm;
			this->pp = rhs_.pp;

			rhs_.um = nullptr;
			rhs_.zm = nullptr;
			rhs_.pp = nullptr;
		}

		return *this;

	}

	DIStruct& operator=(const DIStruct& other_)
	{
		if (&other_ != this)
		{
			this->um = other_.um;
			this->zm = other_.zm;
			this->pp = other_.pp;
		}

		return *this;
	}
};

/// <summary>
/// 해당 추상클래스를 상속하여 구체적인 요청과 처리과정을 구현.
/// 
/// </summary>
class Job
{
public:
	Job(uint32_t userindex_, uint32_t reqNo_, DIStruct* pStDI_);
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

	DIStruct* m_pStDI = nullptr;

	InfoCode SendResultMsg(const uint32_t userindex_, PacketData* pPacket_)
	{
		if (SendMsgFunc == nullptr)
		{
			LOG_ERR("Func Pointer is nullptr.");
			return InfoCode::NULLPTR_ON_FUNCPTR;
		}

		if (pPacket_ == nullptr)
		{
			LOG_ERR("SendMsg Func Requires Packet.");
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
	EchoJob(uint32_t userindex_, uint32_t reqNo_, DIStruct* pStDI_);

	bool Parse(const std::string_view& param_) override;

	InfoCode Execute() override;

private:
	EchoParameter m_param;
};