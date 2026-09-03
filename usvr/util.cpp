#include <xtl.h>
#include <string>
#include "xkelib.h"
#include "util.h"
#include "logging.h"

using namespace std;
// for mem dumps
void binToFile(const char* fname, char* dPtr, ULONG len)
{
	FILE* fp;
	fopen_s (&fp, fname, "ab+" );
	fwrite ( dPtr, len, 1, fp );
	fclose(fp);
}

// for txt logging
void txtToFile(const char* fname, char* sPtr)
{
	FILE* fp;
	fopen_s (&fp, fname, "a+" );
	fwrite ( sPtr, strlen(sPtr), 1, fp );
	fclose(fp);
}

BOOL IsFileExist(PCHAR path)
{
	HANDLE file = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file == INVALID_HANDLE_VALUE)
	{
		if(GetLastError() != 5) // inaccessible means it exists but is probably open somewhere else
			return FALSE;
	}
	else
		CloseHandle(file);
	return TRUE;
}

PBYTE ReadFileToBuf(PCHAR szPath, PDWORD size)
{
	PBYTE buf = NULL;
	if(IsFileExist(szPath))
	{
		HANDLE hFile = CreateFile(szPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile != INVALID_HANDLE_VALUE)
		{
			DWORD dwRead;
			PBYTE buf;
			*size = GetFileSize(hFile, NULL);
			buf = (PBYTE)VirtualAlloc(NULL, *size, MEM_COMMIT|MEM_LARGE_PAGES, PAGE_READWRITE);
			if(buf)
				ReadFile(hFile, buf, *size, &dwRead, NULL);
			CloseHandle(hFile);
		}
	}
	return buf;
}

BOOL WriteBufToFile(PCHAR szPath, PBYTE pbData, DWORD dwLen, BOOL wRemoveExisting)
{
	if(wRemoveExisting)
	{
		if(IsFileExist(szPath))
			DeleteFileA(szPath);
	}
	HANDLE hFile = CreateFile(szPath, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwWrote = 0;
		DWORD currPos = 0;
		while(currPos < dwLen)
		{
			if(WriteFile(hFile, &pbData[currPos], dwLen-currPos, &dwWrote, NULL) == 0)
			{
				CloseHandle(hFile);
				return FALSE;
			}
			currPos += dwWrote;
		}
		CloseHandle(hFile);
		return TRUE;
	}
	return FALSE;
}

HRESULT doDeleteLink(const char* szDrive, const char* sysStr)
{
	STRING LinkName;
	CHAR szDestinationDrive[MAX_PATH];
	sprintf_s(szDestinationDrive, MAX_PATH, sysStr, szDrive);
	RtlInitAnsiString(&LinkName, szDestinationDrive);
	return ObDeleteSymbolicLink(&LinkName);
}

HRESULT deleteLink(const char* szDrive, BOOL both)
{
	HRESULT res = -1;
	if(both)
	{
		res = doDeleteLink(szDrive, OBJ_SYS_STRING);
		res |= doDeleteLink(szDrive, OBJ_USR_STRING);
	}
	else
	{
		if(KeGetCurrentProcessType() == PROC_SYSTEM)
			res = doDeleteLink(szDrive, OBJ_SYS_STRING);
		else
			res = doDeleteLink(szDrive, OBJ_USR_STRING);
	}
	return res;
}

HRESULT doMountPath(const char* szDrive, const char* szDevice, const char* sysStr)
{
	STRING DeviceName, LinkName;
	CHAR szDestinationDrive[MAX_PATH];
	sprintf_s(szDestinationDrive, MAX_PATH, sysStr, szDrive);
	RtlInitAnsiString(&DeviceName, szDevice);
	RtlInitAnsiString(&LinkName, szDestinationDrive);
	ObDeleteSymbolicLink(&LinkName);
	return (HRESULT)ObCreateSymbolicLink(&LinkName, &DeviceName);
}

HRESULT MountPath(const char* szDrive, const char* szDevice, BOOL both)
{
	HRESULT res = -1;
	if(both)
	{
		res = doMountPath(szDrive, szDevice, OBJ_SYS_STRING);
		res |= doMountPath(szDrive, szDevice, OBJ_USR_STRING);
	}
	else
	{
		if(KeGetCurrentProcessType() == PROC_SYSTEM)
			res = doMountPath(szDrive, szDevice, OBJ_SYS_STRING);
		else
			res = doMountPath(szDrive, szDevice, OBJ_USR_STRING);
	}
	return res;
}

BOOL setSockSecurity(SOCKET ss)
{
	BOOL btemp = TRUE;
	if(setsockopt(ss, SOL_SOCKET, SO_PRIVATE, (PCSTR)&btemp, sizeof(BOOL) ) != 0 )//PATCHED!
	{
// 		DbgLog::GetInstance().log("Failed to set debug send socket SO_PRIVATE, error %d\n", WSAGetLastError());
		return FALSE;
	}

	if(setsockopt(ss, SOL_SOCKET, SO_MARKINSECURE, (PCSTR)&btemp, sizeof(BOOL) ) != 0 )//PATCHED!
	{
// 		DbgLog::GetInstance().log("Failed to set debug send socket SO_MARKINSECURE, error %d\n", WSAGetLastError());
		return FALSE;
	}
	return TRUE;
}

BOOL setSockNonblock(SOCKET ss)
{
	DWORD btemp = TRUE;
	if(ioctlsocket(ss, FIONBIO, &btemp) != 0)
	{
// 		DbgLog::GetInstance().log("Failed to set socket FIONBIO, error %d\n", WSAGetLastError());
		return FALSE;
	}
	return TRUE;
}

#pragma warning (disable:4996)
#define SYSCALL_KEY	0x72627472 // rbtr

QWORD __declspec(naked) HvxPeekCall(DWORD key, QWORD type, QWORD SourceAddress, QWORD DestAddress, QWORD lenInBytes)
{ 
	__asm {
		li      r0, 0x0
		sc
		blr
	}
}

QWORD HvxPeek(QWORD SourceAddress, QWORD DestAddress, QWORD lenInBytes)
{
	return HvxPeekCall(SYSCALL_KEY, 5, SourceAddress, DestAddress, lenInBytes);
}

// when called with type 0xFF
// (0) returns 0 on WTFBBQ?
// (1) returns 1 on old old non-peek patches
// (2) returns 0x0000000072627472ULL on first peek patch
// (3) returns 2 on current peek version
DWORD HvxGetPeekVer(void)
{
	DWORD resp = (DWORD)(HvxPeekCall(SYSCALL_KEY, 0xFF, 0, 0, 1)&0xFFFFFFFF);
	if(resp == SYSCALL_KEY)
		return HVPEEK_BASIC;
	else if(resp == 2)
		return HVPEEK_MEMCPY;
	return HVPEEK_NONE;
}

// converts a ascii hex char to a nibble
unsigned char myatox(char c) // 0x2D
{
	if((c >= 0x61) && (c <= 0x66)) // a thru f
		return (unsigned char)(c-0x57);
	else if((c >= 0x30) && (c <= 0x39)) // 0 thru 9
		return (unsigned char)(c-0x30);
	else if((c >= 0x41) && (c <= 0x46)) // A thru F
		return (unsigned char)(c-0x37);
	return 0xFF;
}
