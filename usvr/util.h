#pragma once

#define HVPEEK_NONE		0
#define HVPEEK_BASIC	1
#define HVPEEK_MEMCPY	2

void binToFile(const char* fname, char* dPtr, ULONG len);
void txtToFile(const char* fname, char* sPtr);
BOOL IsFileExist(PCHAR path);
PBYTE ReadFileToBuf(PCHAR szPath, PDWORD size);
BOOL WriteBufToFile(PCHAR szPath, PBYTE pbData, DWORD dwLen, BOOL wRemoveExisting);

HRESULT deleteLink(const char* szDrive, BOOL both);
HRESULT MountPath(const char* szDrive, const char* szDevice, BOOL both);
BOOL setSockSecurity(SOCKET ss);
BOOL setSockNonblock(SOCKET ss);
QWORD HvxPeek(QWORD SourceAddress, QWORD DestAddress, QWORD lenInBytes);
DWORD HvxGetPeekVer(void);
unsigned char myatox(char c);
