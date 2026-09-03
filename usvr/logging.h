#pragma once

//#define FULL_DEBUG_OUT		1
//#define FULL_DEBUG_STATIC	1
//#ifdef FULL_DEBUG_STATIC
//#endif

// #ifdef FULL_DEBUG_OUT
// #endif


class DbgLog
{
public:	
	static DbgLog& GetInstance(){static DbgLog singleton; return singleton;}
	void log(const char* s, ...);

private:
#ifdef FULL_DEBUG_OUT
	char logName[MAX_PATH];
	BOOL logToFile;
	CRITICAL_SECTION writeLock;

	void startLog(void);
	void appendLog(int len, char* data);
#endif

	DbgLog();
	~DbgLog() {}
	DbgLog(const DbgLog&);                 // Prevent copy-construction
	DbgLog& operator=(const DbgLog&);      // Prevent assignment
};
