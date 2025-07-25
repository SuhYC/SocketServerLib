#pragma once

#include <iostream>
#include <mutex>
#include <sstream>

enum class PrintLevel
{
	NO_PRINT,
	PRINT_ERR,
	PRINT_ALL
};

enum class Criticality
{
	ERR,
	DEBUG
};

inline void MakeString(std::ostringstream& oss_)
{
	oss_ << "\n";
	return;
}

template<typename T, typename... Args>
inline void MakeString(std::ostringstream& oss_, T first_, Args... rest_)
{
	oss_ << first_;
	MakeString(oss_, rest_...);
	return;
}

class LogManager final
{
public:
	/// <summary>
	/// 출력레벨을 설정합니다.
	/// 해당 출력레벨보다 같거나 낮은 레벨의 로그는 출력하지 않습니다.
	/// 
	/// lock-based
	/// </summary>
	/// <param name="printLevel_">출력레벨</param>
	static void SetPrintLevel(PrintLevel printLevel_);

	template<typename T, typename... Args>
	static void Log(Criticality crit_, const T first_, const Args... rest_)
	{
		if (static_cast<int32_t>(crit_) >= static_cast<int32_t>(m_printLevel))
		{
			return;
		}

		std::ostringstream oss;

		MakeString(oss, first_, rest_...);

		std::lock_guard<std::mutex> guard(m_mutex);

		std::cout << oss.str();

		return;
	}


private:
	LogManager() = delete;
	static PrintLevel m_printLevel;
	static std::mutex m_mutex;
};