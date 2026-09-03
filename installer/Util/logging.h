#pragma once

//#define LOG_DEBUG_OUT		1
//#define LOG_EXTRA_OUT		1
//#ifdef LOG_EXTRA_OUT
//#endif

// #ifdef LOG_DEBUG_OUT
// #endif

#ifdef LOG_DEBUG_OUT
void lDbgPrint(const char* s, ...);

class DbgLog
{
public:	
	static DbgLog& GetInstance(){static DbgLog singleton; return singleton;}
	void appendLog(int len, char* data);
	BOOL logToFile;

private:
	CRITICAL_SECTION writeLock;

	DbgLog();
	~DbgLog() {}
	DbgLog(const DbgLog&);                 // Prevent copy-construction
	DbgLog& operator=(const DbgLog&);      // Prevent assignment
};
#else
#define lDbgPrint DbgPrint
#endif

#ifdef __cplusplus
extern "C" {
#endif
	LONG WINAPI UnHandleExceptionFilter(struct _EXCEPTION_POINTERS *lpExceptionInfo);

#ifdef __cplusplus
}
#endif
