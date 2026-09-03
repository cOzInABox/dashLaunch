#ifndef _EXCEPT_H_
#define _EXCEPT_H_

void dbgPrintFake(const char* s, ...);

void exceptTrapHook(PDWORD addr);
void exceptTrapUnhook(void);
void exceptBugcheckHook(void);
void exceptBugcheckUnhook(void);

#endif //_EXCEPT_H_
