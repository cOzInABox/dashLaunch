#ifndef _LOADPREP_H
#define _LOADPREP_H

void loadPrepSetRegion(DWORD region);
DWORD loadPrepGetRegion(void);

DWORD loadPrep(DWORD argR3, const char *xex, DWORD argR5, PVOID handle, DWORD typeinfo, DWORD ver, DWORD argR9, DWORD argR10, DWORD argSt1);

void loadPrepHook(PDWORD addr);
void loadPrepUnhook(void);
void loadPrepRegionHook(void);
void loadPrepRegionUnhook(void);

#endif // _LOADPREP_H
