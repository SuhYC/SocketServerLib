#pragma once


#include "Define.hpp"
#include "LogManager.hpp"

#include <Windows.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <sql.h>
#include <string>

#pragma comment(lib, "odbc32.lib")

extern thread_local SQLHDBC g_tlshDbc;
extern thread_local SQLHSTMT g_tlshStmt;

const std::wstring DB_NAME = L"";
const std::wstring DB_ID = L"";
const std::wstring DB_PW = L"";

/// <summary>
/// 일단 싱글턴.
/// 
/// 각 작업스레드 별로 시작할 때 Connect를 호출하고
/// 각 작업스레드가 종료될 때 Release를 호출할 것.
/// </summary>
class Database
{
public:
	static Database& Instance();

	/// <summary>
	/// 일단 생성자에서 1회 호출될 수 있게 해둠.
	/// 명시적으로 호출해도 괜찮음 (중복호출 방지)
	/// </summary>
	/// <returns></returns>
	bool AllocateHEnv();

	/// <summary>
	/// tls에 hDBC와 hStmt를 할당함.
	/// AllocateHEnv가 전역적으로 1회 선행되어야함.
	/// </summary>
	/// <returns></returns>
	bool Connect();

	/// <summary>
	/// tls에 있는 hDBC 와 hStmt를 해제함.
	/// hEnv는 소멸자에서 해제하니 주의.
	/// </summary>
	/// <returns></returns>
	void Release();

private:
	Database();
	~Database();

	SQLHENV m_hEnv;
};