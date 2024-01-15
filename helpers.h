#pragma once

#include <Windows.h>
#include <string>
#include <source_location>

char* getErrorCodeDescription(long errorCode) noexcept;

std::string ws2s(const wchar_t* pcs);


enum log_level
{
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR
};


#define LOG_IT(...) \
	do { \
		log_it(__FUNCTION__, __VA_ARGS__); \
	} while (false)


void
log_it(const char* function, enum log_level level, const char* format, ...);

