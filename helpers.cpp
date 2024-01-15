#include <windows.h>
#include <stdio.h>
#include "helpers.h"
#include <string>
#include <memory>
#include <source_location>
#include <iostream>
//#include <stdafx.h>

std::string ws2s(const wchar_t* pcs)
{

    const size_t origsize = wcslen(pcs) + 1;
    size_t convertedChars = 0;

    const size_t new_size = origsize * 2;
    // Create a unique_ptr to an array of 5 elements.
    std::unique_ptr<char[]> new_string = std::make_unique<char[]>(new_size);

    wcstombs_s(&convertedChars, new_string.get(), new_size, pcs, _TRUNCATE);

    std::string return_value(new_string.get());

    return return_value;
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
