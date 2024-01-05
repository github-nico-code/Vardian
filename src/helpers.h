#pragma once

#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

char* getErrorCodeDescription(long errorCode);

#define SHOW_USED_MESSAGES 1

wchar_t* GetMessageText(unsigned int msg);

#ifdef SHOW_USED_MESSAGES
void ShowUsedMessages(void);
#endif

#ifdef __cplusplus
}
#endif
