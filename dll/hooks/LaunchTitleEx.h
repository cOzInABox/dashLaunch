#ifndef _LAUNCHTITLEEX_H
#define _LAUNCHTITLEEX_H


DWORD LaunchTitleExSaveCall(const char * arg1, const char * arg2, const char * arg3, DWORD flags);

void launchTitleExHook(void);
void launchTitleExUnhook(void);

#endif // _LAUNCHTITLEEX_H
