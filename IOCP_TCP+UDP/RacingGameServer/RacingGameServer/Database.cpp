#include "Database.hpp"

thread_local SQLHDBC g_tlshDbc = nullptr;
thread_local SQLHSTMT g_tlshStmt = nullptr;

Database::Database()
{
	AllocateHEnv();
}

Database::~Database()
{
	if (m_hEnv != nullptr)
	{
		SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
		m_hEnv = nullptr;
	}
}

Database& Database::Instance()
{
	static Database instance;
	return instance;
}

bool Database::AllocateHEnv()
{
	if (m_hEnv != nullptr)
	{
		// 이미 할당되어 있음.
		LOG_DEBUG("환경핸들이 이미 발급되어 있습니다.");
		return true;
	}

	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_hEnv);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
	{
		LOG_ERR("환경핸들 발급 실패.");
		m_hEnv = nullptr;
		return false;
	}

	ret = SQLSetEnvAttr(m_hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
	{
		LOG_ERR("설정 실패");
		SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
		m_hEnv = nullptr;
		return false;
	}

	return true;
}

bool Database::Connect()
{
	// 기존 할당된 핸들 있으면 해제.
	Release();

	SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_DBC, m_hEnv, &g_tlshDbc);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
	{
		LOG_ERR("연결핸들 발급 실패");
		g_tlshDbc = nullptr;
		return false;
	}

	ret = SQLSetConnectAttr(g_tlshDbc, SQL_LOGIN_TIMEOUT, (void*)5, 0);
	
	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
	{
		LOG_ERR("연결 설정 실패");
		SQLFreeHandle(SQL_HANDLE_DBC, g_tlshDbc);
		g_tlshDbc = nullptr;
		return false;
	}

	ret = SQLConnect(g_tlshDbc, (SQLWCHAR*)DB_NAME.c_str(), SQL_NTS, (SQLWCHAR*)DB_ID.c_str(), SQL_NTS, (SQLWCHAR*)DB_PW.c_str(), SQL_NTS);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
	{
		LOG_ERR("연결 실패.");
		SQLFreeHandle(SQL_HANDLE_DBC, g_tlshDbc);
		g_tlshDbc = nullptr;
		return false;
	}

	ret = SQLAllocHandle(SQL_HANDLE_STMT, g_tlshDbc, &g_tlshStmt);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
	{
		LOG_ERR("구문핸들 발급 실패");
		SQLDisconnect(g_tlshDbc);
		SQLFreeHandle(SQL_HANDLE_DBC, g_tlshDbc);
		g_tlshDbc = nullptr;
		g_tlshStmt = nullptr;
		return false;
	}

	return true;
}

void Database::Release()
{
	if (g_tlshStmt != nullptr)
	{
		SQLFreeHandle(SQL_HANDLE_STMT, g_tlshStmt);
		g_tlshStmt = nullptr;
	}

	if (g_tlshDbc != nullptr)
	{
		SQLFreeHandle(SQL_HANDLE_DBC, g_tlshDbc);
		g_tlshDbc = nullptr;
	}

	return;
}