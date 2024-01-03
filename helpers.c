#include <windows.h>
#include <stdio.h>
#include "helpers.h"

char* getErrorCodeDescription(long errorCode)
{
	static char MessageFromSystem[1024];
	static char ReturnMessage[2000];
	DWORD messageReceived = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
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
