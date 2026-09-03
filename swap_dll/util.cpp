#include <xtl.h>
#include <stdio.h>
#include <string.h>
#include "xkelib.h"

// resolves the export table address for the given module
// only works in threads with the ability to peek crypted memory
// only tested on "xam.xex" and "xboxkrnl.exe"
PIMAGE_EXPORT_ADDRESS_TABLE getModuleEat(char* modName)
{
	PLDR_DATA_TABLE_ENTRY moduleHandle = (PLDR_DATA_TABLE_ENTRY)GetModuleHandle(modName);
	if(moduleHandle != NULL)
	{
		PIMAGE_DATA_DIRECTORY pddir;
		PIMAGE_XEX_HEADER xhead = (PIMAGE_XEX_HEADER)moduleHandle->XexHeaderBase;
		pddir = (PIMAGE_DATA_DIRECTORY)RtlImageXexHeaderField(xhead, XEX_HEADER_PE_EXPORTS);
		if(pddir == NULL)
		{
			PXEX_SECURITY_INFO xs = (PXEX_SECURITY_INFO)(xhead->SecurityInfo);
			return (PIMAGE_EXPORT_ADDRESS_TABLE)xs->ImageInfo.ExportTableAddress;
		}
	}
	return NULL;
}

BOOL fileExists(PCHAR path)
{
	//DbgPrint("Check exist: %s\n", path);
	HANDLE file = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file == INVALID_HANDLE_VALUE)
	{
		DWORD err = GetLastError();
		//DbgPrint("check exist failed %08x\n", err);
		if(err != 5)// access denied, it exists we just couldn't get a handle to it
			return FALSE;
	}
	CloseHandle(file);
	return TRUE;
}

// gets the path of a symbolic link
void getPath(char* path, char* outstr)
{
	NTSTATUS retsts;
	HANDLE objHandle;
	OBJECT_ATTRIBUTES oob;
	ULONG retlen;
	STRING unistr = {255,256,outstr};
	STRING oStr = MAKE_STRING(path);
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
		CloseHandle(objHandle);
	}
}

HRESULT deleteLink(const char* szDrive, const char* sysStr)
{
	STRING LinkName;
	CHAR szDestinationDrive[MAX_PATH];
	RtlSnprintf(szDestinationDrive, MAX_PATH, sysStr, szDrive);
	RtlInitAnsiString(&LinkName, szDestinationDrive);
	return ObDeleteSymbolicLink(&LinkName);
}

HRESULT mountPath(const char* szDrive, const char* szDevice, const char* sysStr)
{
	STRING DeviceName, LinkName;
	CHAR szDestinationDrive[MAX_PATH];
	RtlSnprintf(szDestinationDrive, MAX_PATH, sysStr, szDrive);
	RtlInitAnsiString(&DeviceName, szDevice);
	RtlInitAnsiString(&LinkName, szDestinationDrive);
	ObDeleteSymbolicLink(&LinkName);
	return (HRESULT)ObCreateSymbolicLink(&LinkName, &DeviceName);
}

HRESULT mount(const char* szDrive, const char* szDevice)
{
	if(KeGetCurrentProcessType() == PROC_SYSTEM)
		return mountPath(szDrive, szDevice, OBJ_SYS_STRING);
	else
		return mountPath(szDrive, szDevice, OBJ_USR_STRING);
}

u32 resolveFunct(char* modname, u32 ord)
{
	DWORD ret=0, ptr2=0;
	HANDLE modHandle;
	modHandle = GetModuleHandle(modname);
	//DbgPrint("%s - XexGetModuleHandle ret: %08x, ptr32: %08x\n", modname, ret, ptr32);
	if(modHandle != NULL)
	{
		ret = XexGetProcedureAddress(modHandle, ord, &ptr2 );
		//DbgPrint("%s - XexGetProcedureAddress ret: %08x, ptr2: %08x\n", modname, ret, ptr2);
		return ptr2;
	}
	return 0; // function not found
}

VOID patchInJump(u32* addr, u32 dest, BOOL linked)
{
	if(dest & 0x8000) // If bit 16 is 1
		addr[0] = 0x3D600000 + (((dest >> 16) & 0xFFFF) + 1); // lis 	%r11, dest>>16 + 1
	else
		addr[0] = 0x3D600000 + ((dest >> 16) & 0xFFFF); // lis 	%r11, dest>>16

	addr[1] = 0x396B0000 + (dest & 0xFFFF); // addi	%r11, %r11, dest&0xFFFF
	addr[2] = 0x7D6903A6; // mtctr	%r11

	if(linked)
		addr[3] = 0x4E800421; // bctrl
	else
		addr[3] = 0x4E800420; // bctr
	doSync(addr);
}

BOOL hookImpStub(char* modname, char* impmodname, DWORD ord, DWORD patchAddr, BOOL linked)
{
	DWORD orgAddr;
	PLDR_DATA_TABLE_ENTRY ldat;
	PXEX_IMPORT_DESCRIPTOR imps;
	PXEX_IMPORT_TABLE impTbl;
	char* impName;
	int i, j;
	BOOL ret = FALSE;
	// get the address of the actual function that is jumped to
	orgAddr = resolveFunct(impmodname, ord);
	if(orgAddr == 0)
	{
		DbgPrint("could not find ordinal %d in mod %s\n", ord, impmodname);
		return FALSE;
	}
	// find where kmodule info is stowed
	ldat = (PLDR_DATA_TABLE_ENTRY)GetModuleHandle(modname);
	if(ldat == NULL)
	{
		DbgPrint("could not find data table for mod %s\n", modname);
		return FALSE;
	}
	// use kmod info to find xex header in memory
	imps = (PXEX_IMPORT_DESCRIPTOR)RtlImageXexHeaderField(ldat->XexHeaderBase, 0x000103FF);
	if(imps == NULL)
	{
		DbgPrint("could not find import descriptor for mod %s\n", modname);
		return FALSE;
	}
	impName = (char*)(imps+1);
	impTbl = (PXEX_IMPORT_TABLE)(impName + imps->NameTableSize);
	for(i = 0; i < (int)(imps->ModuleCount); i++)
	{
		// use import descriptor strings to refine table
		DbgPrint("checking table %s of %s for address %08x\n", impName, modname, orgAddr);
		for(j = 0; j < impTbl->ImportCount; j++)
		{
			PDWORD add = (PDWORD)impTbl->ImportStubAddr[j];
			//DbgPrint("i: %d j: %x of %x destAddr: %08x add: %08x %08x\n", i, j, impTbl->ImportCount, impTbl->ImportStubAddr[j], add[0], add[1]);
			if(add[0] == orgAddr)
			{
				//DbgPrint("%s %s tbl %d has ord %x at tstub %d location %08x\n", modname, impName, i, ord, j, impTbl->ImportStubAddr[j+1]);
				patchInJump((u32*)patchAddr, impTbl->ImportStubAddr[j+1], linked);
				j = impTbl->ImportCount;
				ret = TRUE;
			}
		}

		//impTbl = (PXEX_IMPORT_TABLE)((u32)impTbl+impTbl->TableSize);
		impTbl = (PXEX_IMPORT_TABLE)((BYTE*)impTbl+impTbl->TableSize);
		impName = impName+strlen(impName);
		while((impName[0]&0xFF) == 0x0)
			impName++;
	}
	return ret;
}

// returns 0 on success, creates symbolic link on it's own to the new szDrive
u32 mountCon(CHAR* szDrive, CHAR* szDevice, CHAR* szPath)
{
	CHAR szMountPath[MAX_PATH];
	RtlSnprintf(szMountPath,MAX_PATH,"\\??\\%s\\%s", szDevice, szPath);
	//DbgPrint("mounting: '%s' to: '%s'\n", szMountPath, szDrive);
	return XamContentOpenFile(0xFE, szDrive, szMountPath, 0x4000043,0,0,0);
}

// returns 0 on success, destroys symbolic link as well
u32 unmountCon(CHAR* szDrive)
{
	CHAR szMountPath[MAX_PATH];
	RtlSnprintf(szMountPath,MAX_PATH,"\\??\\%s",szDrive);
	//DbgPrint("umt path: %s\n", szMountPath);
	return  XamContentClose(szMountPath,0);
}

DWORD HookOrd(DWORD ord, DWORD dstFun, PIMAGE_EXPORT_ADDRESS_TABLE expbase)
{
	DWORD modOffset = (expbase->ImageBaseAddress)<<16;
	DWORD origOffset = (expbase->ordOffset[ord-1])+modOffset;
	expbase->ordOffset[ord-1] = dstFun-modOffset;
	doSync(&expbase->ordOffset[ord-1]);
	return origOffset;
}
