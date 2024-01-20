#include <windows.h>
#include <stdio.h>
#include "helpers.h"
#include <string>
#include <memory>
#include <source_location>
#include <iostream>
#include <codecvt>
#include <vector>

std::string ws2s(const std::wstring& wstr)
{
    // Convert a Unicode string to an ASCII string
    std::string strTo;
    auto szTo = std::vector<char>(wstr.length() + 1);
    szTo.at(wstr.size()) = '\0';
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, szTo.data(), wstr.length(), NULL, NULL);
    strTo = szTo.data();
    return strTo;
}

std::wstring s2ws(const std::string& str)
{
    // Convert an ASCII string to a Unicode String
    std::wstring wstrTo;
    auto wszTo = std::vector<wchar_t>(str.length() + 1);
    wszTo.at(str.size()) = L'\0';
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wszTo.data(), str.length());
    wstrTo = wszTo.data();
    return wstrTo;
}

std::string getErrorCodeDescription(long errorCode)
{
	auto MessageFromSystem = std::vector<char>(1024);
	std::string ReturnMessage;
	const DWORD messageReceived = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
		0,
		errorCode,
		1033,                          // US English
		MessageFromSystem.data(),
		1024,
		0);

    if (!messageReceived)
        ReturnMessage = std::string("Error code: ") + std::to_string(errorCode);
	else
        ReturnMessage = std::string("Error code: '") + std::to_string(errorCode) +
            std::to_string(errorCode) + "' with message: " + MessageFromSystem.data();

	return ReturnMessage;
}