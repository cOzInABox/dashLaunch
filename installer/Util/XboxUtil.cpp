#include "XboxUtil.h"
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include "Strings.h"
#include "logging.h"

using namespace std;

#define write32(x,y) (__storewordbytereverse(y, 0, (void*)x))
#define read32(x) (__loadwordbytereverse(0, (void*)x))

#define SYSCALL_KEY	0x72627472 // rbtr

// when called with type 0xFF
// returns 1 on old old non-peek patches
// returns 0x0000000072627472ULL on first peek patch
// returns 2 on current peek version
QWORD __declspec(naked) HvxPeekCall(DWORD key, QWORD type, QWORD SourceAddress, QWORD DestAddress, QWORD lenInBytes)
{ 
	__asm {
		li      r0, 0x0
		sc
		blr
	}
}

DWORD __declspec(naked) HvxStartXell(DWORD key, DWORD mode, UINT64 dest, UINT64 src, UINT64 linInU32)
{ 
	__asm {
		li      r0, 0x0
		sc
		blr
	}
}

XboxUtil::XboxUtil(VOID)
{
	isNetInit = FALSE;
}

QWORD XboxUtil::HvxPeek(QWORD SourceAddress, QWORD DestAddress, QWORD lenInBytes)
{
// 	lDbgPrint("source: %016I64x\n", SourceAddress);
// 	lDbgPrint("dest  : %016I64x\n", DestAddress);
// 	lDbgPrint("len   : %016I64x\n", lenInBytes);
	return HvxPeekCall(SYSCALL_KEY, 5, SourceAddress, DestAddress, lenInBytes);
}

// when called with type 0xFF
// (0) returns 0 on WTFBBQ?
// (1) returns 1 on old old non-peek patches
// (2) returns 0x0000000072627472ULL on first peek patch
// (3) returns 2 on current peek version
DWORD XboxUtil::HvxGetPeekVer(void)
{
	DWORD resp = (DWORD)(HvxPeekCall(SYSCALL_KEY, 0xFF, 0, 0, 1)&0xFFFFFFFF);
	if(resp == SYSCALL_KEY)
		return HVPEEK_BASIC;
	else if(resp == 2)
		return HVPEEK_MEMCPY;
	return HVPEEK_NONE;
}

VOID XboxUtil::UsbShutdown(VOID)
{
	// EHCI
	// disable interrupts
	write32(0x7FEA3028,0);
	write32(0x7FEA5028,0);
	// halt
	write32(0x7FEA3020,0);
	write32(0x7FEA5020,0);

	// OHCI
	// disable interrupts
	write32(0x7FEA2014,1 << 31);
	write32(0x7FEA4014,1 << 31);
	// halt
	write32(0x7FEA2004,0);
	read32(0x7FEA2004);
	write32(0x7FEA4004,0);
	read32(0x7FEA4004);
}

VOID XboxUtil::StartXell(PBYTE phyBuffer, DWORD len, UINT64 dest)
{
	UINT64 srcp = 0x8000000000000000ULL;
	UINT64 lenp = 0ULL+((len+0x3)&~0x3);
	lenp = (lenp/4)&0xFFFFFFFF;
	srcp = srcp+(((DWORD)MmGetPhysicalAddress(phyBuffer))&0xFFFFFFFF);
	UsbShutdown();
	HvxStartXell(SYSCALL_KEY, 4, dest, srcp, lenp);
}

HRESULT XboxUtil::doDeleteLink(const char* szDrive, const char* sysStr)
{
	STRING LinkName;
	CHAR szDestinationDrive[MAX_PATH];
	sprintf_s(szDestinationDrive, MAX_PATH, sysStr, szDrive);
	RtlInitAnsiString(&LinkName, szDestinationDrive);
	return ObDeleteSymbolicLink(&LinkName);
}

HRESULT XboxUtil::DeleteLink(const char* szDrive, BOOL both)
{
	HRESULT res = -1;
	if(szDrive != NULL)
	{
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
	}
	return res;
}

HRESULT XboxUtil::doMountPath(const char* szDrive, const char* szDevice, const char* sysStr)
{
	STRING DeviceName, LinkName;
	CHAR szDestinationDrive[MAX_PATH];
	sprintf_s(szDestinationDrive, MAX_PATH, sysStr, szDrive);
	RtlInitAnsiString(&DeviceName, szDevice);
	RtlInitAnsiString(&LinkName, szDestinationDrive);
	ObDeleteSymbolicLink(&LinkName);
	return (HRESULT)ObCreateSymbolicLink(&LinkName, &DeviceName);
}

HRESULT XboxUtil::MountPath(const char* szDrive, const char* szDevice, BOOL both)
{
	HRESULT res = -1;
	if((szDrive != NULL)&&(szDevice != NULL))
	{
#ifdef LOG_EXTRA_OUT
		lDbgPrint("MountPath: drive %s device %s both %d\n", szDrive, szDevice, both);
#endif
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
	}
#ifdef LOG_EXTRA_OUT
	else
		lDbgPrint("MountPath: called with NULL arg!!\n");
#endif
	return res;
}

PVOID XboxUtil::ResolveFunction(PCHAR szModuleName, DWORD dwOrdinal)
{
	PVOID pProc = NULL;
	HANDLE hModuleHandle;
	if(NT_SUCCESS(XexGetModuleHandle(szModuleName, &hModuleHandle)))
		XexGetProcedureAddress(hModuleHandle, dwOrdinal, &pProc);
	return pProc;
}

BOOL XboxUtil::IsFileExist(PCHAR path)
{
	if(path != NULL)
	{
		HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(file == INVALID_HANDLE_VALUE)
		{
			if(GetLastError() != 5) // inaccessible means it exists but is probably open somewhere else
				return FALSE;
		}
		else
			CloseHandle(file);
		return TRUE;
	}
#ifdef LOG_EXTRA_OUT
	else
		lDbgPrint("IsFileExist: called with NULL arg!!\n");
#endif
	return FALSE;
}

BOOL XboxUtil::IsDriveExist(PCHAR path)
{
	if(path != NULL)
	{
		string dd(path);
		if(path[(dd.size()-1)] != '\\')
			dd += "\\";
#ifdef LOG_EXTRA_OUT
		else
			lDbgPrint("IsDriveExist: before %s after %s\n", path, dd.c_str());
#endif
		DWORD attr = GetFileAttributes(dd.c_str());
		if(attr != NEG_ONE_AS_DWORD)
			return TRUE;
	}
#ifdef LOG_EXTRA_OUT
	else
		lDbgPrint("IsDriveExist: called with NULL arg!!\n");
#endif
	return FALSE;
}

VOID XboxUtil::QuitToDefault(VOID)
{
	XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
}

VOID XboxUtil::Reboot(VOID)
{
	HalReturnToFirmware(HalPowerDownRoutine);
}

PBYTE XboxUtil::ReadFileToBuf(PCHAR szPath, PDWORD size)
{
	if(IsFileExist(szPath))
	{
		HANDLE hFile = CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile != INVALID_HANDLE_VALUE)
		{
			DWORD dwRead;
			PBYTE buf;
			*size = GetFileSize(hFile, NULL);
			buf = new (std::nothrow) BYTE[*size];
			if(buf)
				ReadFile(hFile, buf, *size, &dwRead, NULL);
			CloseHandle(hFile);
			return buf;
		}
	}
	return NULL;
}

BOOL XboxUtil::WriteBufToFile(PCHAR szPath, PBYTE pbData, DWORD dwLen, BOOL wRemoveExisting)
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

wstring XboxUtil::GetWstring(string cStr)
{
	DWORD len = cStr.size();
	PWCHAR wStr = new WCHAR[len+1];
	ZeroMemory(wStr, (len+1)*2);
	MultiByteToWideChar(CP_UTF8, 0, cStr.c_str(), len, wStr, len);
	wstring ret(wStr);
	delete wStr;
	return ret;
}

wstring XboxUtil::GetWstring(char* str)
{
	DWORD len = strlen(str);
	PWCHAR wStr = new WCHAR[len+1];
	ZeroMemory(wStr, (len+1)*2);
	MultiByteToWideChar(CP_UTF8, 0, str, len, wStr, len);
	wstring ret(wStr);
	delete wStr;
	return ret;
}

wstring XboxUtil::BytesToWstring(DWORD lowBytes, DWORD hiBytes)
{
	ULONGLONG usz = hiBytes;
	WCHAR temp[128];
	usz = (usz << 32)|lowBytes;
	long double sz = (long double)usz;
	if(sz < 1024.0)
		wsprintfW(temp, L"%4.1f%s", sz, Strings::GetInstance().Lookup(L"symbytes"));
	else // bytes to kilobytes
	{
		sz = sz/1024;
		if(sz < 1024.0)
			wsprintfW(temp, L"%4.1f%s", sz, Strings::GetInstance().Lookup(L"symkiloby"));
		else // kilobytes to MB
		{
			sz = sz/1024.0;
			if(sz < 1024.0)
				wsprintfW(temp, L"%4.1f%s", sz, Strings::GetInstance().Lookup(L"symmegaby"));
			else // MB to GB
			{
				sz = sz/1024.0;
				wsprintfW(temp, L"%4.1f%s", sz, Strings::GetInstance().Lookup(L"symgigaby"));
			}
		}
	}
	wstring ret(temp);
	return ret;
}

VOID XboxUtil::InstallThis(char* drive, char* path)
{
	char inst[MAX_PATH];
	char sub[MAX_PATH];
	char dest[MAX_PATH];
	strcpy(inst, drive);
	strcat(inst, path);
	strcat(inst, "\\DL30");
	strcpy(sub, inst);
	//lDbgPrint("installing to: %s%s == %s\n", drive, path, inst);
	if(!CreateDirectoryA(inst, NULL))
	{
		DWORD err = GetLastError();
		if(err != ERROR_ALREADY_EXISTS)
		{
			lDbgPrint("error %08x creating directory %s!\n", inst);
			return;
		}
	}
	strcat(inst, "\\default.xex");
	//lDbgPrint("copying file to: %s\n", inst);
	if(IsFileExist("Game:\\default.xex"))
	{
		if(!CopyFileA("Game:\\default.xex", inst, FALSE))
		{
			lDbgPrint("copy file Game:\\default.xex to %s failed %08x\n", inst, GetLastError());
			return;
		}
	}
	else if(IsFileExist("Game:\\installer.xex"))
	{
		if(!CopyFileA("Game:\\installer.xex", inst, FALSE))
		{
			lDbgPrint("copy file failed %08x\n", GetLastError());
			return;
		}
	}
	if(IsFileExist("Game:\\skin.xzp"))
	{
		strcpy(dest, sub);
		strcat(dest, "\\skin.xzp");
		if(!CopyFileA("Game:\\skin.xzp", dest, FALSE))
		{
			lDbgPrint("copy skin failed %08x\n", GetLastError());
		}
	}
	if(IsFileExist("Game:\\background.png"))
	{
		strcpy(dest, sub);
		strcat(dest, "\\background.png");
		if(!CopyFileA("Game:\\background.png", dest, FALSE))
		{
			lDbgPrint("copy background failed %08x\n", GetLastError());
		}
	}
	if(IsFileExist("Game:\\font.ttf"))
	{
		strcpy(dest, sub);
		strcat(dest, "\\font.ttf");
		if(!CopyFileA("Game:\\font.ttf", dest, FALSE))
		{
			lDbgPrint("copy font failed %08x\n", GetLastError());
		}
	}
}

// returns 0 on success, creates symbolic link on it's own to the new szDrive
DWORD XboxUtil::MountCon(PCHAR szDrive, PCHAR szPath)
{
	DWORD ret;
	CHAR szMountPath[MAX_PATH];
	sprintf_s(szMountPath, MAX_PATH, "\\??\\%s", szPath);
	ret = XamContentOpenFile(XCONTENT_ANY_USER, szDrive, szMountPath, XCONTENT_MOUNT_FOR_READ_ONLY,0,0,0);
// 	lDbgPrint("XboxUtil::MountCon '%s' to '%s' ret %08x\n", szDrive, szMountPath, ret);
	return ret;
}

DWORD XboxUtil::MountConExec(PCHAR szDrive, PCHAR szPath)
{
	DWORD ret;
	CHAR szMountPath[MAX_PATH];
	sprintf_s(szMountPath, MAX_PATH, "\\??\\%s", szPath);
	ret = XamContentOpenFile(XCONTENT_ANY_USER, szDrive, szMountPath, XCONTENT_MOUNT_FOR_EXEC,0,0,0);
// 	lDbgPrint("XboxUtil::MountConExec '%s' to '%s' ret %08x\n", szDrive, szMountPath, ret);
	return ret;
}

// returns 0 on success, destroys symbolic link as well
DWORD XboxUtil::UnmountCon(PCHAR szDrive)
{
	DWORD ret;
	CHAR szMountPath[MAX_PATH];
	sprintf_s(szMountPath, MAX_PATH, "\\??\\%s", szDrive);
	ret = XamContentClose(szMountPath, 0);
// 	lDbgPrint("XboxUtil::UnmountCon '%s' ret: %08x\n", szMountPath, ret);
	return ret;
}


BOOL XboxUtil::GetCpuKey(PBYTE buf)
{
	BOOL ret = FALSE;
	if(HvxGetPeekVer() >= HVPEEK_BASIC)
	{
		PBYTE keybuf = (PBYTE)XPhysicalAlloc(0x10, MAXULONG_PTR, 0, MEM_LARGE_PAGES|PAGE_READWRITE|PAGE_NOCACHE);
	// 	lDbgPrint("keybuf: %08x phy %08x\n", keybuf,MmGetPhysicalAddress(keybuf) );
		if(keybuf != NULL)
		{
			QWORD dest = 0x8000000000000000ULL | ((DWORD)MmGetPhysicalAddress(keybuf)&0xFFFFFFFF);
			ZeroMemory(keybuf, 0x10);
			HvxPeek(0x20ULL, dest, 0x10ULL);
			memcpy(buf, keybuf, 0x10);
			ret = TRUE;
			XPhysicalFree(keybuf);
		}
	}
	return ret;
}

BOOL XboxUtil::GetDvdKey(PBYTE buf)
{
	DWORD len = 0x10;
	if(XeKeysGetKey(XEKEY_DVD_KEY, buf, &len) >= 0)
		return TRUE;
	return FALSE;
}

DWORD XboxUtil::GetXVal(VOID)
{
	return XamGetSecurityViolationsDetected();
}

// 0 is cpu speed (trinity) 1 is gpu speed (fats)
VOID XboxUtil::SetFanSpeeds(DWORD fan, DWORD speed, BOOL disable)
{
	BYTE mess[0x10];
	ZeroMemory(mess, 0x10);
	mess[0] = fan ? 0x94 : 0x89;
	if(disable || (speed == 0x7F))
		mess[1] = 0x7F;
	else
	{
		if(speed > 100)
			speed = 100;
		mess[1] = (unsigned char)(speed | 0x80);
	}
	HalSendSMCMessage(mess, NULL);
}

// use this to calculate XCONFIG_STATIC_SETTINGS.CheckSum
// where data is a BYTE pointer to the full XCONFIG_STATIC_SETTINGS struct
DWORD XboxUtil::CalcSmcConfigHash(PBYTE data)
{
	UINT i, len, sum = 0;
	data += 0x10;
	for(i=0, len=252; i<len; i++)
		sum += data[i]&0xFF;
	sum = (~sum)&0xFFFF;
	return ((sum&0xFF00)<<8)+((sum&0xFF)<<24);
}

VOID XboxUtil::display_buffer_hex(PBYTE buffer, int size)
{
	int i;
	for (i=0; i<size; i++)
	{
		if (!(i%0x10))
			lDbgPrint("\n  ");
		lDbgPrint(" %02X,", buffer[i]);
	}
	lDbgPrint("\n");
}

// do not delay more than 1s
BOOL XboxUtil::GetIpAddr(char* ipadd, int ccs)
{
	int i;
	XNADDR xna;
	DWORD dwRes = XNET_GET_XNADDR_PENDING;
	for(i = 0; i < 10; i++)
	{
		dwRes = XNetGetTitleXnAddr(&xna);
		if(dwRes != XNET_GET_XNADDR_PENDING)
		{
			XNetInAddrToString(xna.ina, ipadd, ccs);
			return TRUE;
		}
		Sleep(100);
	}
	return FALSE;
}

BOOL XboxUtil::InitNetwork(void)
{
	if(isNetInit == FALSE)
	{
		XNetStartupParams xnsp;
		memset(&xnsp, 0, sizeof(xnsp));
		xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);
		xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
		xnsp.cfgSockDefaultRecvBufsizeInK = 128; // default = 16
		xnsp.cfgSockDefaultSendBufsizeInK = 128; // default = 16
		xnsp.cfgQosSrvMaxSimultaneousResponses = 16;

		if(XNetStartup(&xnsp) == NO_ERROR)
		{
			WSADATA wsad;
			if(WSAStartup(MAKEWORD(2,2),&wsad) ==0)
				isNetInit = TRUE;
			else
				lDbgPrint("WSAStartup failed!\n");
		}
		else
			lDbgPrint("XNetStartup failed!\n");
	}
#ifdef LOG_EXTRA_OUT
	lDbgPrint("InitNetwork: %d\n", isNetInit);
#endif
	return isNetInit;
}

BOOL XboxUtil::ShutdownNetwork(void)
{
	// clean up network stack
	if(isNetInit)
	{
		WSACleanup();
		XNetCleanup();
		isNetInit = FALSE;
	}
#ifdef LOG_EXTRA_OUT
	lDbgPrint("ShutdownNetwork\n");
#endif
	return TRUE;
}
