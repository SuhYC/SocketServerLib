#pragma once

#include "LogManager.hpp"

void LogManager::SetPrintLevel(PrintLevel printLevel_)
{
	std::lock_guard<std::mutex> guard(m_mutex);

	m_printLevel = printLevel_;

	return;
}

PrintLevel LogManager::m_printLevel = PrintLevel::PRINT_ALL;
std::mutex LogManager::m_mutex;