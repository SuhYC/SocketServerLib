#pragma once

#include <string_view>
#include <vector>
#include <functional>
#include <variant>
#include "PacketData.hpp"
#include "PacketPool.hpp"
#include "MemoryPool.hpp"
#include "NetworkMessage.hpp"

using SENDFUNC = std::function<bool(const uint32_t, PacketData*, SendProtocol)>;
using TOKENFUNC = std::function<uint64_t(uint32_t)>;

class DIStruct
{
public:
	SENDFUNC sendFunc;
	TOKENFUNC getTokenFunc;
};

class Job
{
public:
	Job(DIStruct* stDI_);
	virtual ~Job();

	virtual bool Parse(std::string_view& sv_) = 0;
	virtual bool Execute() = 0;
	virtual bool SendRes() = 0;

	uint32_t m_ClientIndex;
	uint32_t m_ReqNo;
	DIStruct* m_stDI;
	PacketData* m_pPacket;
};

class EchoJob : public Job
{
public:
	EchoJob(DIStruct* stDI_);

	bool Parse(std::string_view& sv_) override;
	bool Execute() override;
	bool SendRes() override;
private:
	EchoParameter m_param;
};

class ReqTokenJob : public Job
{
public:
	ReqTokenJob(DIStruct* stDI_);

	bool Parse(std::string_view& sv_) override;
	bool Execute() override;
	bool SendRes() override;
};

class UDPEchoJob : public Job
{
public:
	UDPEchoJob(DIStruct* stDI_);

	bool Parse(std::string_view& sv_) override;
	bool Execute() override;
	bool SendRes() override;

	EchoParameter m_param;
};

using Jobs = std::variant <
	// ----- 사용할 작업객체를 아래에 명시할것. 작업객체 최대크기 추정.
	EchoJob,
	ReqTokenJob,
	UDPEchoJob
>;

const uint32_t MAX_JOB_SIZE = sizeof(Jobs);

class JobFactory final
{
public:
	JobFactory();

	static JobFactory& Instance();

	bool Init(SENDFUNC sendFunc_, TOKENFUNC tokenFunc_);

	Job* CreateJob(uint32_t uUserIndex_, std::string_view& sv);
	Job* CreateUDPJob(uint32_t uUserIndex_, std::string_view& sv_);
	void DeallocateJob(Job* pJob);

private:
	template<typename T>
	typename std::enable_if<std::is_base_of<Job, T>::value, Job*>::type
		Allocate()
	{
		if (sizeof(T) > MAX_JOB_SIZE)
		{
			LOG_ERR("Check Jobs variant.");
			return nullptr;
		}

		void* memory = m_pool.Allocate();

		if (memory == nullptr)
		{
			LOG_ERR("Failed to Allocate Memory Block From Pool.");
			return nullptr;
		}

		Job* pRet = new (memory) T(&m_DIStruct);

		pRet->m_stDI = &m_DIStruct;

		return pRet;
	}

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
			[this]() -> Job* {
			return Allocate<T>();
			};
	}
	MemoryPool m_pool;
	DIStruct m_DIStruct;

	std::vector<std::function<Job* ()>> createFuncs;
};
