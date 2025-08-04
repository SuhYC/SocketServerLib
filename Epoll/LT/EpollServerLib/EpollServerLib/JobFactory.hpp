#pragma once

#include "Define.hpp"
#include "Job.hpp"
#include <functional>
#include <string_view>
#include <iostream>
#include <cstring>

class JobFactory
{
public:
	void Init();

	Job* CreateJob(const int fd_, std::string_view& sv_);
private:
	template<typename T>
	typename std::enable_if<std::is_base_of<Job, T>::value, void>::type
		Register(ReqType eReqType_)
	{
		createFuncs[static_cast<int32_t>(eReqType_)] =
			[this](const int fd_, uint32_t reqNo_) -> Job* {
			return new T(fd_, reqNo_);
			};
	}
	std::vector<std::function<Job* (const int, uint32_t)>> createFuncs;
};