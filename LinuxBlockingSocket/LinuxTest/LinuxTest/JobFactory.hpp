#pragma once

#include "Job.hpp"
#include "PacketData.hpp"
#include "PacketData.hpp"
#include <new>

/// <summary>
/// Ŭ���̾�Ʈ�� ��û�ڵ�.
/// Ŭ���̾�Ʈ�� �̳Ѱ��� �����ϰ� ������ ��.
/// �ű԰��� �߰��� ���� �ݵ�� �������� �ǵ帮�� �ʰ� �������� �߰��� �� LAST�� �����Ұ�.
/// </summary>
enum class ReqType
{
	ECHO,
	LAST = ECHO
};

class JobFactory
{
public:
	void Init();

	/// <summary>
	/// ������ �´� Job�� �Ļ�Ŭ������ �����Ͽ� ���̷ε带 Parse���ִ� �Լ�.
	/// ��������� Job�� �Ļ�Ŭ���� ������ �ľ��ϰ�
	/// ����� ������ �����ͷ� �Ķ���͸� ������ȭ�Ѵ�.
	/// 
	/// Register�Լ��� ȣ��Ǿ� �־���Ѵ�! Register�Լ��� ȣ������ ���� �Ļ�Ŭ������ �䱸�Ѵٸ� nullptr�� ��ȯ�Ѵ�.
	/// </summary>
	/// <param name="userindex_">��û�� Ŭ���̾�Ʈ�� ��ȣ</param>
	/// <param name="req_">ReqHeader + Payload</param>
	/// <returns>������ �Ļ�Ŭ���� ������. req_�� ������ �ְų� Register�� ȣ����� �ʾҴٸ� nullptr�� ��ȯ.</returns>
	Job* CreateJob(uint16_t userindex_, std::string_view& req_);

	/// <summary>
	/// Job�� �Ļ�Ŭ���� �޸𸮺��� ȸ���ϴ� �Լ�.
	/// �Ҹ��ڵ� �˾Ƽ� ȣ�����ִ� ��ȯ�� �ϸ� �ȴ�.
	/// </summary>
	/// <param name="pJob_"></param>
	void DeallocateJob(Job* pJob_);


private:
	/// <summary>
	/// �߰��� �۾���ü�� ��û�ڵ带 �����ϴ� �Լ�.
	/// ��û�ڵ忡 �´� �۾���ü�� �����ϴ� ���ٽ��� �ѱ��.
	/// </summary>
	/// <typeparam name="T">Job�� �Ļ�Ŭ���� Ÿ��</typeparam>
	/// <param name="eReqType_">��û�ڵ�</param>
	/// <returns></returns>
	template<typename T>
	typename std::enable_if<std::is_base_of<Job, T>::value, void>::type
		Register(ReqType eReqType_)
	{
		createFuncs[static_cast<int32_t>(eReqType_)] =
			[this](uint16_t userindex_, uint32_t reqNo_) -> Job* {
			return new(std::nothrow) T(userindex_, reqNo_);
			};
	}

	std::vector<std::function<Job* (uint16_t, uint32_t)>> createFuncs;
};