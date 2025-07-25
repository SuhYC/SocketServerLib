#pragma once

#include <stdint.h>
#include <optional>
#include <string_view>
#include <functional>
#include "PacketData.hpp"
#include <iostream>

/// <summary>
/// �ش� �߻�Ŭ������ ����Ͽ� ��ü���� ��û�� ó�������� ����.
/// 
/// </summary>
class Job
{
public:
	Job(uint32_t userindex_, uint32_t reqNo_);
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
/// ���ÿ�.
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