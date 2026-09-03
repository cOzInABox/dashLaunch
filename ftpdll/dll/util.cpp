#include <xtl.h>
#include <assert.h>
#include <stdio.h>
#include "xamext.h"
#include "util.h"

HRESULT deleteLink(const char* szDrive)
{
	STRING LinkName;
	CHAR szDestinationDrive[MAX_PATH];
	sprintf_s(szDestinationDrive, MAX_PATH, SYS_STRING, szDrive);
	RtlInitAnsiString(&LinkName, szDestinationDrive);
	return ObDeleteSymbolicLink(&LinkName);
}

HRESULT mountIt(const char* szDrive, const char* szDevice, const char* typestr)
{
	STRING DeviceName, LinkName;
	CHAR szDestinationDrive[MAX_PATH];
	sprintf_s(szDestinationDrive, MAX_PATH, typestr, szDrive);
	RtlInitAnsiString(&DeviceName, szDevice);
	RtlInitAnsiString(&LinkName, szDestinationDrive);
	ObDeleteSymbolicLink(&LinkName);
	return (HRESULT)ObCreateSymbolicLink(&LinkName, &DeviceName);
}

HRESULT ObMountBoth(const char* szDrive, const char* szDevice)
{
	mountIt(szDrive, szDevice, SYS_STRING);
	return mountIt(szDrive, szDevice, USR_STRING);
}

HRESULT ObMount(const char* szDrive, const char* szDevice)
{
	return mountIt(szDrive, szDevice, SYS_STRING);
}

// returns 0 on success, creates symbolic link on it's own to the new szDrive
DWORD mountCon(CHAR* szDrive, CHAR* szPath)
{
	DWORD ret;
	CHAR szMountPath[MAX_PATH];
	sprintf_s( szMountPath,MAX_PATH, FUSR_STRING, szPath );
	ret = XamContentOpenFile(0xFE, szDrive, szMountPath, 0x4000003,0,0,0);
	DbgPrint("mounting '%s' to '%s' returns %08x\n", szMountPath, szDrive, ret);
	if(ret == 0)
	{
		getPath("\\??\\fxcon:", szMountPath);
		ret = ObMount("fxcon:", szMountPath);
		DbgPrint("mounting '%s' to sys returns %08x\n", szMountPath, ret);
	}
	return ret;
}

// returns 0 on success, destroys symbolic link as well
DWORD unmountCon( CHAR* szDrive )
{
	CHAR szMountPath[MAX_PATH];
	sprintf_s(szMountPath,MAX_PATH, USR_STRING, szDrive);
	deleteLink(szDrive);
	//DbgPrint("umt path: %s\n", szMountPath);
	return XamContentClose(szMountPath,0);
}

// gets the path of a symbolic link
void getPath(char* path, char* outstr)
{
	NTSTATUS retsts;
	HANDLE objHandle;
	OBJECT_ATTRIBUTES oob;
	ULONG retlen;
	STRING unistr = {255,256,outstr};
	STRING oStr;
	RtlInitAnsiString(&oStr, path);

	InitializeObjectAttributes(&oob, &oStr, OBJ_CASE_INSENSITIVE, 0);
	memset(outstr, 0x0, 256);
	retsts = NtOpenSymbolicLinkObject(&objHandle, &oob);
	//DbgPrint("getpath: open symbolic link returns %x\n", retsts);
	if(retsts == 0)
	{
		retsts = NtQuerySymbolicLinkObject(objHandle, &unistr, &retlen);
		//DbgPrint("getpath: Query symbolic link returns %x\n", retsts);
		//if(retsts == 0)
		//	DbgPrint("getpath: %s link is: %s\n", path, outstr);
	}
	CloseHandle(objHandle);
}

void utilRelaunch(char* path)
{
	//CHAR szMountPath[MAX_PATH];
	DbgPrint("I'm supposed to launch %s now...\n", path);
	XLaunchNewImage(path,0);
}
