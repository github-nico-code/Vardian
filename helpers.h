#pragma once

#include <Windows.h>
#include <string>
#include <source_location>
#include <format>

std::string getErrorCodeDescription(long errorCode);

std::wstring s2ws(const std::string& str);

std::string ws2s(const std::wstring& wstr);

enum log_level
{
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR
};

/*
#define LOG_IT(...) \
	do { \
		log_it_new(std::source_location::current(), __VA_ARGS__); \
	} while (false)

template<typename... Args>
void log_it_new(std::source_location location, enum log_level level, const std::string_view& print_format, Args... the_args)
{
	std::string the_output = std::string("## LEVEL=") + std::to_string(level) +
		std::string(", function='") + location.function_name() + "' ==> " + 
		std::vformat(print_format, std::make_format_args(the_args...)) + "\n";

	logString(the_output);
}
*/
void ReplaceStringInPlace(std::string& subject, const std::string& search,
	const std::string& replace);
