#include "_hook_includes.h"
#include <stdio.h>
#include <string.h>

enum{
	SWAP_FIND_FAILED = 0,
	SWAP_SAME_DISK,
	SWAP_READY,
	SWAP_GOD_READY,
};

static CHAR swpdiskpath[MAX_PATH];
static CHAR swpcurpath[MAX_PATH];
static BOOL swapComplete = FALSE;

#define SWAP_CONT		"swp"
#define SWAP_NAME		"swp:"
#define SWAP_DRIVE		"swp:\\"
#define SWAP_OBJECT		"\\??\\swp:"
#define TSWAP_NAME		"tswp:" // for mounting GOD paths
static const CHAR TSWAP_CHR[] = TSWAP_NAME;
static const CHAR TSWAP_CONT[] = SWAP_CONT;

#define XAMSWAPDISK		2600 // XamSwapDisc
typedef DWORD (*XAMSWAPDISKFUN)(DWORD ucNextDisc, PKEVENT hSwapComplete, PXSWAPDISC_ERROR_TEXT XSwapDiscErrorText);
XAMSWAPDISKFUN XamSwapDiskSave;

//  '\??\GAME:' linked to: '\Device\Harddisk0\Partition1\Gamez\DeadSpace2\Disk1'
DWORD processAutoXex(DWORD ucNextDisk, char* diskPath, char* curPath)
{
	char nextDisk[10];
	RtlSprintf(nextDisk, "disk%d", ucNextDisk);
	if(strstr(curPath, nextDisk) != NULL)
		return SWAP_SAME_DISK;
	else
	{
		char* cprt;
		strcpy(diskPath, curPath);
#ifdef DEBUG_MULTIDISK_OUT
		DbgPrint("diskpath0: %s\n", diskPath);
#endif
		cprt = strrchr(diskPath, '\\'); // take off the folder path
		cprt[1] = 0;
		strcat(diskPath, nextDisk);
#ifdef DEBUG_MULTIDISK_OUT
		DbgPrint("diskpathf: %s\n", diskPath);
#endif
		MountPath(SWAP_NAME, diskPath, FALSE);
		if(fileExists("swp:\\default.xex"))
		{
#ifdef DEBUG_MULTIDISK_OUT
			DbgPrint("swap dest found OK\n");
#endif
			return SWAP_READY;
		}
#ifdef DEBUG_MULTIDISK_OUT
		DbgPrint("swap dest not found!\n");
#endif
	}
	return SWAP_FIND_FAILED;
}

BOOL findNextGod(DWORD ucNextDisk, DWORD titleId, char* curname, char* diskPath)
{
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile("tswp:\\*", &findData);
	if(hFind == INVALID_HANDLE_VALUE)
	{
#ifdef DEBUG_MULTIDISK_OUT
		DbgPrint("FindFirstFile failed!\n");
#endif
		return FALSE;
	}
	do {
		if((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
#ifdef DEBUG_MULTIDISK_OUT
			DbgPrint("about to strcmp\n");
			DbgPrint("%s to %s\n", curname, findData.cFileName);
#endif
			if(strcmp(curname, findData.cFileName) != 0)
			{
				DWORD ret = MountContainer(TSWAP_CONT, TSWAP_CHR, findData.cFileName);
				if(ret == 0)
				{
					PXCONTENT_MOUNTED_PACKAGE pxc;
					DWORD ret = XamContentGetMountedPackageByRootName(TSWAP_CONT, 1, &pxc);
					if(ret == 0)
					{
#ifdef DEBUG_MULTIDISK_OUT
						DbgPrint("mounted %s\n", findData.cFileName);
						DbgPrint("szFsDeviceName   : %s\n", pxc->szFsDeviceName);
						DbgPrint("szPackageFilePath: %s\n", pxc->szPackageFilePath);
#endif
						if((pxc->ContentMetaData.ExecutionId.DiscNum == ucNextDisk)&&(pxc->ContentMetaData.ExecutionId.TitleID == titleId))
						{
							strcpy(diskPath, findData.cFileName);
							ObDereferenceObject(pxc->pvFsDeviceObject);
							FindClose(hFind);
							return TRUE;
						}
						ObDereferenceObject(pxc->pvFsDeviceObject);
					}
					UnmountContainer(TSWAP_CONT);
				}
			}
		}
	} while (FindNextFile(hFind, &findData));
	FindClose(hFind);
	return FALSE;
}

DWORD processAutoGod(DWORD ucNextDisc, DWORD titleId, char* curPath, char* diskPath)
{
	DWORD fret = SWAP_FIND_FAILED;
	PXCONTENT_MOUNTED_PACKAGE pxc;
	DWORD ret = XamContentGetMountedPackageByRootName("GAME", 1, &pxc);
	//szFsDeviceName:    \Device\Package_CC23BE0F4F387DB6208D0ED940041FEC
	//szPackageFilePath: \Device\Harddisk0\Partition1\Content\0000000000000000\454108DF\00007000\B8605B3BAFB748ACD08A
	if(ret == 0)
	{
		if(pxc->ContentMetaData.ExecutionId.DiscNum == ucNextDisc)
		{
			fret = SWAP_SAME_DISK;
		}
		else
		{
			char* curGod = strrchr(pxc->szPackageFilePath, '\\');
			if(curGod != NULL)
			{
				ZeroMemory(curPath, MAX_PATH);
				curGod++;
#ifdef DEBUG_MULTIDISK_OUT
				DbgPrint("curGod %08x pxc->szPackageFilePath %08x\n", curGod, pxc->szPackageFilePath);
#endif
				strncpy(curPath, pxc->szPackageFilePath, curGod-pxc->szPackageFilePath);
#ifdef DEBUG_MULTIDISK_OUT
				DbgPrint("szFsDeviceName   : %s\n", pxc->szFsDeviceName);
				DbgPrint("szPackageFilePath: %s\n", pxc->szPackageFilePath);
				DbgPrint("curPath          : %s\n", curPath);
				DbgPrint("curGod           : %s\n", curGod);
				DbgPrint("mounting %s to %s\n", curPath, TSWAP_CHR);
#endif
				if(MountPath(TSWAP_CHR, curPath, FALSE) == 0)
				{
#ifdef DEBUG_MULTIDISK_OUT
					DbgPrint("tswp: MountPath succeeded\n");
#endif
					if(findNextGod(ucNextDisc, titleId, curGod, diskPath))
					{
#ifdef DEBUG_MULTIDISK_OUT
						DbgPrint("next GOD found at %s\n", diskPath);
#endif
						fret = SWAP_GOD_READY;
					}
#ifdef DEBUG_MULTIDISK_OUT
					else
						DbgPrint("findNextGod failed!\n");
#endif
					deleteLink(TSWAP_CHR, FALSE);
				}
#ifdef DEBUG_MULTIDISK_OUT
				else
					DbgPrint("mount swp failed!\n");
#endif
			}
		}
		ObDereferenceObject(pxc->pvFsDeviceObject);
	}
	return fret;
}

DWORD getNextDisk(DWORD ucNextDisc, DWORD titleId, char* diskPath, char* curPath, BOOL isGod)
{
	DWORD ret = SWAP_FIND_FAILED;
	if(isGod)
	{
#ifdef DEBUG_MULTIDISK_OUT
		DbgPrint("process auto GOD\n");
#endif
		ret = processAutoGod(ucNextDisc, titleId, curPath, diskPath);
	}
	else
	{
#ifdef DEBUG_MULTIDISK_OUT
		DbgPrint("process auto XEX\n");
#endif
		ret = processAutoXex(ucNextDisc, diskPath, curPath);
	}
	return ret;
}

DWORD DoSwapDisk(DWORD ucNextDisc)
{
	DWORD ret, titleId, res;
	BOOL isGod = FALSE;
	ZeroMemory(swpcurpath, MAX_PATH);
	ZeroMemory(swpdiskpath, MAX_PATH);
	titleId = XamGetCurrentTitleId();
	getSymbolicPath("\\??\\GAME:", swpcurpath);
	if(strnicmp(swpcurpath, "\\Device\\Package_", strlen("\\Device\\Package_")) == 0)
		isGod = TRUE;
#ifdef DEBUG_MULTIDISK_OUT
	DbgPrint("swap disc from tid:%08x called to disk %d proc type %d isGod %d\n", titleId, ucNextDisc, KeGetCurrentProcessType(), isGod);
#endif
	res = getNextDisk(ucNextDisc, titleId, swpdiskpath, swpcurpath, isGod);
	if(res != SWAP_FIND_FAILED)
	{
		if(res != SWAP_SAME_DISK)
		{
			// cleanup the game link path environment
			if(isGod)
			{
#ifdef DEBUG_MULTIDISK_OUT
				DbgPrint("CON detected as mounted\n");
#endif
				ret = UnmountContainer("GAME");
#ifdef DEBUG_MULTIDISK_OUT
				DbgPrint("unmount con GAME: returns %08x\n", ret);
#endif
			}
			deleteLink("GAME:", TRUE);
			deleteLink("D:", TRUE);

			if(isGod)
			{
				getSymbolicPath(SWAP_OBJECT, swpcurpath);
				ret = MountPath("D:", swpcurpath, FALSE);
				ret |= MountPath("GAME:", swpcurpath, TRUE);
				deleteLink(SWAP_NAME, TRUE);
				if(ret == 0)
					swapComplete = TRUE;
#ifdef DEBUG_MULTIDISK_OUT
				else
					DbgPrint("CON mount failed! %x (%x)\n", ret, GetLastError());
#endif
			}
			else
			{
				ret = MountPath("D:", swpdiskpath, FALSE); // this is in a title thread already...
				//DbgPrint("sysmount D: ret %x\n", ret);
				ret |= MountPath("GAME:", swpdiskpath, TRUE);
				if(ret == 0)
					swapComplete = TRUE;
			}
		}
		else
		{
			swapComplete = TRUE;
		}

	}
	doSync(&swapComplete);
	return 0;
}

DWORD XamSwapDiscHook(DWORD ucNextDisc, PKEVENT hSwapComplete, PXSWAPDISC_ERROR_TEXT XSwapDiscErrorText)
{
	HANDLE hThread;
	DWORD dwThreadId, ret;
	swapComplete = FALSE;
	doSync(&swapComplete);
	hThread = CreateThread( 0, 0, (LPTHREAD_START_ROUTINE)DoSwapDisk, (LPVOID)ucNextDisc, CREATE_SUSPENDED, &dwThreadId );
	XSetThreadProcessor(hThread, 4);
	ResumeThread(hThread);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	if(swapComplete)
	{
#ifdef DEBUG_MULTIDISK_OUT
		DbgPrint("setting swap complete\n");
#endif
		ret = KeSetEvent(hSwapComplete, 1, FALSE);
#ifdef DEBUG_MULTIDISK_OUT
		DbgPrint("setevent returns: %x\n", ret);
#endif
		ObDereferenceObject(hSwapComplete);
	}
	else
	{
		if(XamSwapDiskSave != NULL)
		{
			ret = XamSwapDiskSave(ucNextDisc, hSwapComplete, XSwapDiscErrorText);
#ifdef DEBUG_MULTIDISK_OUT
		DbgPrint("swap no good, calling original function ret: %x\n", ret);
#endif
		}
		else
		{
#ifdef DEBUG_MULTIDISK_OUT
			DbgPrint("swap not hooked, ret: 0x65B\n");
#endif
			ret = 0x65B;
		}
	}
	return ret;
}

static BOOL isMultiHooked = FALSE;
void multidiskHook(void)
{
	if(!isMultiHooked)
	{
		XamSwapDiskSave = (XAMSWAPDISKFUN)hookExportOrd(MODULE_XAM, XAMSWAPDISK, (DWORD)XamSwapDiscHook);
		isMultiHooked = TRUE;
	}
}

void multidiskUnhook(void)
{
	if(isMultiHooked)
	{
		unhookExportOrd(MODULE_XAM, XAMSWAPDISK, (DWORD)XamSwapDiskSave);
		isMultiHooked = FALSE;
	}
}
