#ifndef _UTIL_H
#define _UTIL_H


BOOL fileExists(PCHAR path);
void getPath(char* path, char* outstr);
HRESULT deleteLink(const char* szDrive, const char* sysStr);
HRESULT mountPath(const char* szDrive, const char* szDevice, const char* sysStr);
HRESULT mount(const char* szDrive, const char* szDevice);
u32 resolveFunct(char* modname, u32 ord);
u32 mountCon(CHAR* szDrive, CHAR* szDevice, CHAR* szPath);
u32 unmountCon(CHAR* szDrive);
VOID patchInJump(u32* addr, u32 dest, BOOL linked);
DWORD HookOrd(DWORD ord, DWORD dstFun, PIMAGE_EXPORT_ADDRESS_TABLE expbase);
PIMAGE_EXPORT_ADDRESS_TABLE getModuleEat(char* modName);
// ** warning, this implementation may still be very broken...
BOOL hookImpStub(char* modname, char* impmodname, DWORD ord, DWORD patchAddr, BOOL linked);


#endif
