#include <xtl.h>
#include <stdio.h>
#include "xkelib.h"
#include "logging.h"

#ifdef FULL_DEBUG_STATIC
#define STATIC_FILE_NAME	"Game:\\updsvr.log"
#endif

DbgLog::DbgLog()
{
#ifdef FULL_DEBUG_OUT
	#ifdef FULL_DEBUG_STATIC
		logToFile = FALSE;
		strcpy(logName, STATIC_FILE_NAME);
		startLog();
	#else
		SYSTEMTIME st;
		logToFile = FALSE;
		GetLocalTime(&st);
		sprintf_s(logName, MAX_PATH, "Game:\\log_%02d%02d%02d.txt",st.wHour,st.wMinute,st.wSecond);
		startLog();
	#endif
#endif
}

#ifdef FULL_DEBUG_OUT
void DbgLog::startLog(void)
{
	HANDLE fhand;
	DbgPrint("logName: %s\n", logName);
	DeleteFile(logName);
	fhand = CreateFile(logName, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fhand != INVALID_HANDLE_VALUE)
	{
		CloseHandle(fhand);
		InitializeCriticalSection(&writeLock);
		logToFile = TRUE;
	}
}

void DbgLog::appendLog(int len, char* data)
{
	HANDLE fhand;
	EnterCriticalSection(&writeLock);
	fhand = CreateFile(logName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fhand != INVALID_HANDLE_VALUE)
	{
		DWORD bWrote;
		SetFilePointer(fhand, 0, NULL, FILE_END);
		WriteFile(fhand, data, len, &bWrote, NULL);
		CloseHandle(fhand);
	}
	else
		DbgPrint("warning log line dropped!\n");
	LeaveCriticalSection(&writeLock);
}
#endif

void DbgLog::log(const char* s, ...)
{
	if(s != NULL)
	{
		int chars;
		va_list argp;
		char temp[512];

		va_start(argp, s);
		chars = vsnprintf_s(temp, 512, 512, s, argp);
		va_end(argp);
		if(chars > 0)
		{
			DbgPrint("%s", temp);
#ifdef FULL_DEBUG_OUT
			if(logToFile)
			{
				appendLog(chars, temp);
			}
#endif
		}
	}
}
