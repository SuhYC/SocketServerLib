#pragma once

#include "Job.hpp"

class JobFactory
{
public:
	void Init();

	Job* CreateJob(const SOCKET clientSocket_, std::string_view& sv_);
private:
	template<typename T>
	typename std::enable_if<std::is_base_of<Job, T>::value, void>::type
		Register(ReqType eReqType_)
	{
		createFuncs[static_cast<int32_t>(eReqType_)] =
			[this](const SOCKET clientSocket_, uint32_t reqNo_) -> Job* {
			return new T(clientSocket_, reqNo_);
			};
	}
	std::vector<std::function<Job* (const SOCKET, uint32_t)>> createFuncs;
};