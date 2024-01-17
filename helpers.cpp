#include <windows.h>
#include <stdio.h>
#include "helpers.h"
#include <string>
#include <memory>
#include <source_location>
#include <iostream>
#include <codecvt>
//#include <stdafx.h>

std::string ws2s(const std::wstring& wstr)
{
    // Convert a Unicode string to an ASCII string
    std::string strTo;
    char* szTo = new char[wstr.length() + 1];
    szTo[wstr.size()] = '\0';
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, szTo, (int)wstr.length(), NULL, NULL);
    strTo = szTo;
    delete[] szTo;
    return strTo;
}

std::wstring s2ws(const std::string& str)
{
    // Convert an ASCII string to a Unicode String
    std::wstring wstrTo;
    wchar_t* wszTo = new wchar_t[str.length() + 1];
    wszTo[str.size()] = L'\0';
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wszTo, (int)str.length());
    wstrTo = wszTo;
    delete[] wszTo;
    return wstrTo;
}
char* getErrorCodeDescription(long errorCode) noexcept
{
	static char MessageFromSystem[1024];
	static char ReturnMessage[2000];
	const DWORD messageReceived = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
		0,
		errorCode,
		1033,                          // US English
		MessageFromSystem,
		1024,
		0);

	if (!messageReceived)
		sprintf_s(ReturnMessage, 2000, "Error code: %i", errorCode);
	else
		sprintf_s(ReturnMessage, 2000, "Error code '%i' with message: '%s'", errorCode, MessageFromSystem);

	return ReturnMessage;
}

void log(std::string_view message,
    std::source_location location = std::source_location::current()
) {
    std::cout << "info:"
        << location.file_name() << ":"
        << location.line() << ":"
        << location.function_name() << " "
        << message << '\n';
}


void
log_it(const char* function, enum log_level level, const char* format, ...)
{
    va_list args;

    std::string the_output = std::string( "LEVEL=" ) + std::to_string( level ) + 
        std::string( ", function='" ) + std::string(function) + "' ==> ";

    va_start(args, format);
    const int len = _vscprintf(format, args) + 1;
    char* buffer = (char*)malloc(len * sizeof(char));
    if (NULL != buffer)
    {
        vsprintf_s(buffer, len, format, args);
        the_output += buffer + std::string("\n");
        free(buffer);
    }
    va_end(args);

    OutputDebugStringA(the_output.c_str());
}
