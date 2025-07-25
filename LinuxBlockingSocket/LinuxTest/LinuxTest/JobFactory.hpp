#pragma once

#include "Job.hpp"
#include "PacketData.hpp"
#include "PacketData.hpp"
#include <new>

/// <summary>
/// 클라이언트의 요청코드.
/// 클라이언트의 이넘값과 동일하게 구성할 것.
/// 신규값을 추가할 때는 반드시 기존값을 건드리지 않고 마지막에 추가한 후 LAST에 지정할것.
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
	/// 정보에 맞는 Job의 파생클래스를 생성하여 페이로드를 Parse해주는 함수.
	/// 헤더정보로 Job의 파생클래스 종류를 파악하고
	/// 헤더를 제외한 데이터로 파라미터를 역직렬화한다.
	/// 
	/// Register함수가 호출되어 있어야한다! Register함수를 호출하지 않은 파생클래스를 요구한다면 nullptr를 반환한다.
	/// </summary>
	/// <param name="userindex_">요청한 클라이언트의 번호</param>
	/// <param name="req_">ReqHeader + Payload</param>
	/// <returns>적절한 파생클래스 포인터. req_에 문제가 있거나 Register가 호출되지 않았다면 nullptr를 반환.</returns>
	Job* CreateJob(uint16_t userindex_, std::string_view& req_);

	/// <summary>
	/// Job의 파생클래스 메모리블럭을 회수하는 함수.
	/// 소멸자도 알아서 호출해주니 반환만 하면 된다.
	/// </summary>
	/// <param name="pJob_"></param>
	void DeallocateJob(Job* pJob_);


private:
	/// <summary>
	/// 추가한 작업객체와 요청코드를 연결하는 함수.
	/// 요청코드에 맞는 작업객체를 생성하는 람다식을 넘긴다.
	/// </summary>
	/// <typeparam name="T">Job의 파생클래스 타입</typeparam>
	/// <param name="eReqType_">요청코드</param>
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