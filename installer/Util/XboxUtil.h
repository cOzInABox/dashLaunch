#pragma once
#include <xtl.h>
#include <string>
#include <cstring>
#include "xkelib.h"

using namespace std;

#define XELL_OFFSET_JTAG	0x80000200C8095060ULL
#define XELL_OFFSET_GLITCH	0x80000200C8070000ULL
#define XELL_DEST_JTAG		0x800000001c040000ULL
#define XELL_DEST_GLITCH	0x800000001c000000ULL
#define SMC_FAN_CPU			0
#define SMC_FAN_GPU			1

#define HVPEEK_NONE		0
#define HVPEEK_BASIC	1
#define HVPEEK_MEMCPY	2

class XboxUtil
{
public:	
	static XboxUtil& GetInstance(){static XboxUtil singleton; return singleton;}

	QWORD HvxPeek(QWORD SourceAddress, QWORD DestAddress, QWORD lenInBytes);
	DWORD HvxGetPeekVer(void);
	VOID StartXell(PBYTE phyBuffer, DWORD len, UINT64 dest);
	HRESULT DeleteLink(const char* szDrive, BOOL both = TRUE);
	HRESULT MountPath(const char* szDrive, const char* szDevice, BOOL both = FALSE);
	PVOID ResolveFunction(PCHAR szModuleName, DWORD dwOrdinal);
	VOID QuitToDefault(VOID);
	BOOL IsFileExist(PCHAR path);
	BOOL IsDriveExist(PCHAR path);
	VOID Reboot(VOID);
	PBYTE ReadFileToBuf(PCHAR szPath, PDWORD size);
	BOOL WriteBufToFile(PCHAR szPath, PBYTE pbData, DWORD dwLen, BOOL wRemoveExisting);
	wstring GetWstring(string cStr);
	wstring GetWstring(char* str);
	wstring BytesToWstring(DWORD lowBytes, DWORD hiBytes);
	VOID InstallThis(char* drive, char* path);
	DWORD MountCon(PCHAR szDrive, PCHAR szPath);
	DWORD MountConExec(PCHAR szDrive, PCHAR szPath);
	DWORD UnmountCon(PCHAR szDrive);
	BOOL GetCpuKey(PBYTE buf);
	BOOL GetDvdKey(PBYTE buf);
	DWORD GetXVal(VOID);
	VOID SetFanSpeeds(DWORD fan, DWORD speed, BOOL disable = FALSE);
	DWORD CalcSmcConfigHash(PBYTE data);
	VOID display_buffer_hex(PBYTE buffer, int size);
	BOOL GetIpAddr(char* ipadd, int ccs);
	BOOL InitNetwork(void);
	BOOL ShutdownNetwork(void);

private:
	HRESULT doDeleteLink(const char* szDrive, const char* sysStr);
	HRESULT doMountPath(const char* szDrive, const char* szDevice, const char* sysStr);
	VOID UsbShutdown(VOID);
	BOOL isNetInit;
	XboxUtil();
	~XboxUtil() {}
	XboxUtil(const XboxUtil&);                 // Prevent copy-construction
	XboxUtil& operator=(const XboxUtil&);      // Prevent assignment
};


