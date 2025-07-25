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
/// �۾�ó���� �ʿ��� ������ ��ü�� ��� �ѱ�� ���� ����ü.
/// UserManager : L7���� ���������� ���� ��ü�� �����ϴ� ��ü
/// ZoneManager : Zone ��� ������ ���� �������� �����ϴ� ������ �����ϴ� ��ü
/// PacketPool : �۽��� �޽����� ����ȭ�� ���۸� ������ƮǮ�� �����ϴ� ��ü.
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
/// �ش� �߻�Ŭ������ ����Ͽ� ��ü���� ��û�� ó�������� ����.
/// 
/// </summary>
class Job
{
public:
	Job(uint32_t userindex_, uint32_t reqNo_, DIStruct* pStDI_);
	virtual ~Job() = default;

	/// <summary>
	/// ���̳ʸ������͸� �������� �۾���ü�� �Ķ���͸� �ʱ�ȭ�ϴ� �Լ�
	/// </summary>
	/// <param name="param_"></param>
	/// <returns></returns>
	virtual bool Parse(const std::string_view& param_) = 0;

	/// <summary>
	/// �ʱ�ȭ�� �۾���ü�� �Ķ���͸� �������� ��û�� ó���ϰ� ��� �����ϴ� �Լ�.
	/// 
	/// InfoCode::NOT_FINISHED�� ��ȯ�Ͽ� JobQueue�� ������� �� �ִ�.
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
/// ���ÿ�.
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