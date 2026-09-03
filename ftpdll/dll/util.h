#ifndef _UTIL_DEFINES_H
#define _UTIL_DEFINES_H


HRESULT deleteLink(const char* szDrive);
HRESULT ObMountBoth(const char* szDrive, const char* szDevice);
HRESULT ObMount(const char* szDrive, const char* szDevice);
DWORD mountCon(CHAR* szDrive, CHAR* szPath);
DWORD unmountCon(CHAR* szDrive);
void getPath(char* path, char* outstr);
void utilRelaunch(char* path);

#endif // _UTIL_DEFINES_H