#include <xtl.h>
#include <stdio.h>
#include <string.h>
#include "xkelib.h"
#include "swap.h"
#include "util.h"
#include "SimpleIni.h"

void titleTerminateHandler(void);

CSimpleIniA g_ini(1, 0, 0);
BOOL isXex = FALSE;
BOOL iniLoaded = FALSE;
BOOL godNeedsUnmount = FALSE;
CHAR godpath[MAX_PATH];
EX_TITLE_TERMINATE_REGISTRATION terminateRegistraion = {titleTerminateHandler, 0 }; // xam uses 0x7C800000 for early and 0x0 for late

extern "C" const TCHAR szModuleName[] = TEXT( "swap.dll" ); // probably don't need to export this

void titleTerminateHandler(void)
{
	if(godNeedsUnmount)
	{
		DWORD ret;
		ret = mountPath("UMT:", godpath, OBJ_USR_STRING);
		//DbgPrint("mount UMT returns %08x\n", ret);
		if(ret == 0)
		{
			ret = unmountCon("UMT");
			//DbgPrint("unmountCon UMT returns %08x\n", ret);
			deleteLink("UMT:", OBJ_SYS_STRING);
		}
		godNeedsUnmount = FALSE;
	}
}

void DeleteLinks(const char* szDrive)
{
	HRESULT res;
	res = deleteLink(szDrive, OBJ_SYS_STRING);
	//DbgPrint("del sys link (%s): %08x\n", szDrive, res);
	res = deleteLink(szDrive, OBJ_USR_STRING);
	//DbgPrint("del usr link (%s): %08x\n", szDrive, res);
}

// find and load multi.ini
BOOL loadIni(void)
{
	int i;
	for(i=0; i < (MOUNT_MAX_ITEMS); i++)
	{
		if(mountPath(MOUNT_NAME, mountPaths[i], OBJ_USR_STRING) == STATUS_SUCCESS)
		{
			//DbgPrint("ini check %s for %s\n", mountPaths[i], INI_NAME);
			if(fileExists(INI_NAME))
			{
				SI_Error rc = g_ini.LoadFile(INI_NAME);
				if(rc >= 0)
				{
					//DbgPrint("ini found at %s and loaded OK\n", mountPaths[i]);
					return TRUE;
				}
			}
		}
		//else
		//DbgPrint("mount %s failed\n", mountPaths[i], INI_NAME);
	}
	//DbgPrint("ini not found!\n");
	return FALSE;
}

// returns true if info found in ini OK
BOOL getInfoFromIni(DWORD ucNextDisc, DWORD titleId, char* diskpath)
{
	const CHAR* iniPath;
	DWORD devNum = 0xF, testNum = 0, i;
	char dtemp[10];
	char tidtemp[20];
	isXex = FALSE;
	if(!iniLoaded)
	{
		if(loadIni())
			iniLoaded = TRUE;
		else
			return FALSE;
	}
	// try and find the right game in the ini
	RtlSnprintf(dtemp, 25, "disk%d", ucNextDisc);
	RtlSnprintf(tidtemp, 25, "%08x", titleId);
	//DbgPrint("ini: seek [%s] for %s\n", tidtemp, dtemp);
	iniPath = g_ini.GetValue(tidtemp, dtemp, NULL);
	if(iniPath == NULL)
	{
		//DbgPrint("tid %08x not found in ini!\n", tidtemp);
		return FALSE;
	}

	//DbgPrint("path found in ini: %s\n", iniPath);

	// make sure it exists on some device at that path and use that info, if not return FALSE
	// figure out which mount point was used
	for(i = 0; i < DEVS_MAX; i++)
	{
		if(strnicmp(iniPath, numDevs[i].mountPoint, (numDevs[i].stlen+1)) == 0)
		{
			devNum = numDevs[i].startdev;
			testNum = numDevs[i].numdev;
			strcpy_s(diskpath, MAX_PATH, "swp");
			strcat_s(diskpath, MAX_PATH, (iniPath+numDevs[i].stlen));
			if(iniPath[(strlen(iniPath)-1)] == '\\')
			{
				isXex = TRUE;
				strcat_s(diskpath, MAX_PATH, "default.xex");
			}
			//DbgPrint("built path %s\n", diskpath);
			i = DEVS_MAX;
		}
	}
	if(devNum != 0xF)
	{
		for(i = 0; i < testNum; i++)
		{
			mount(MOUNT_NAME, mountPaths[(devNum+i)]);
			if(fileExists(diskpath) == TRUE)
			{
				//DbgPrint("file %s found at %s\n", iniPath, mountPaths[(devNum+i)]);
				DeleteLinks(MOUNT_NAME);
				// build paths for mounting replacement disk
				if(isXex)
				{
					strcpy_s(diskpath, MAX_PATH, mountPaths[(devNum+i)]);
					strcat_s(diskpath, MAX_PATH, (iniPath+numDevs[i].stlen+2));	
					//DbgPrint("xex path built: %s\n", diskpath);
				}
				else
				{
					mountPath(MOUNT_NAME, mountPaths[(devNum+i)], OBJ_USR_STRING);
					strcpy_s(diskpath, MAX_PATH, (iniPath+numDevs[i].stlen+2));	
					//DbgPrint("con path built: %s at %s\n", diskpath, mountPaths[(devNum+i)]);
				}
				return TRUE;
			}
		}
	}
	return FALSE;
}

BOOL findNextGod(DWORD ucNextDisk, DWORD titleId, char* curname)
{
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile("swp:\\*", &findData);
	if(hFind == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	do {
		if((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			if(strcmp(curname, findData.cFileName) != 0)
			{
				DWORD ret = mountCon("gswp", MOUNT_NAME, findData.cFileName);
				if(ret == 0)
				{
					PXCONTENT_MOUNTED_PACKAGE pxc;
					DWORD ret = XamContentGetMountedPackageByRootName("gswp", 1, &pxc);
					if(ret == 0)
					{
						//DbgPrint("mounted %s\n", findData.cFileName);
						//DbgPrint("szFsDeviceName   : %s\n", pxc->szFsDeviceName);
						//DbgPrint("szPackageFilePath: %s\n", pxc->szPackageFilePath);
						if((pxc->ContentMetaData.ExecutionId.DiscNum == ucNextDisk)&&(pxc->ContentMetaData.ExecutionId.TitleID == titleId))
						{
							strcpy(godpath, findData.cFileName);
							ObDereferenceObject(pxc->pvFsDeviceObject);
							FindClose(hFind);
							return TRUE;
						}
						ObDereferenceObject(pxc->pvFsDeviceObject);
					}
					unmountCon("gswp");
				}
			}
		}
	} while (FindNextFile(hFind, &findData));
	FindClose(hFind);
	return FALSE;
}

DWORD processAutoGod(DWORD ucNextDisc, DWORD titleId, char* diskpath)
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
				char bpath[MAX_PATH];
				ZeroMemory(bpath, MAX_PATH);
				curGod++;
				strncpy(bpath, pxc->szPackageFilePath, curGod-pxc->szPackageFilePath);
				//DbgPrint("szFsDeviceName   : %s\n", pxc->szFsDeviceName);
				//DbgPrint("szPackageFilePath: %s\n", pxc->szPackageFilePath);
				//DbgPrint("bpath            : %s\n", bpath);
				//DbgPrint("curGod           : %s\n", curGod);
				if(mount(MOUNT_NAME, bpath) == 0)
				{
					if(findNextGod(ucNextDisc, titleId, curGod))
					{
						//DbgPrint("next GOD found at %s\n", godpath);
						fret = SWAP_GOD_READY;
					}
					else
					{
						DeleteLinks(MOUNT_NAME);
					}
				}
				//else
				//	DbgPrint("mount swp failed!\n");
			}
		}
		ObDereferenceObject(pxc->pvFsDeviceObject);
	}
	return fret;
}

//  '\??\GAME:' linked to: '\Device\Harddisk0\Partition1\Gamez\DeadSpace2\Disk1'
DWORD processAutoXex(DWORD ucNextDisk, char* diskpath, char* curPath)
{
	char nextDisk[10];
	sprintf(nextDisk, "disk%d", ucNextDisk);
	if(strstr(curPath, nextDisk) != NULL)
		return SWAP_SAME_DISK;
	else
	{
		char* cprt;
		strcpy(diskpath, curPath);
		//DbgPrint("diskpath0: %s\n", diskpath);
		cprt = strrchr(diskpath, '\\'); // take off the folder path
		cprt[1] = 0;
		strcat(diskpath, nextDisk);
		//DbgPrint("diskpathf: %s\n", diskpath);
		mount(MOUNT_NAME, diskpath);
		if(fileExists("swp:\\default.xex"))
		{
			//DbgPrint("swap dest found OK\n");
			isXex = TRUE;
			return SWAP_READY;
		}
		//DbgPrint("swap dest not found!\n");
	}
	return SWAP_FIND_FAILED;
}

DWORD getNextDisk(DWORD ucNextDisc, DWORD titleId, char* diskpath, char* curPath, BOOL isGod)
{
	if(getInfoFromIni(ucNextDisc, titleId, diskpath))
		return SWAP_READY;
	//DbgPrint("disk not in ini\n");
	if(isGod)
	{
		isXex = FALSE;
		//DbgPrint("process auto GOD\n");
		return processAutoGod(ucNextDisc, titleId, diskpath);
	}
	else
	{
		//DbgPrint("process auto XEX\n");
		return processAutoXex(ucNextDisc, diskpath, curPath);
	}
	return SWAP_FIND_FAILED;
}

DWORD (*XamSwapDiscOrg)(DWORD, PHANDLE, PXSWAPDISC_ERROR_TEXT);
DWORD XamSwapDiscHook(DWORD ucNextDisc, PHANDLE hSwapComplete, PXSWAPDISC_ERROR_TEXT XSwapDiscErrorText)
{
	DWORD ret, titleId, res;
	CHAR diskpath[MAX_PATH];
	CHAR curpath[MAX_PATH];
	BOOL swapComplete = FALSE;
	BOOL isGod = FALSE;
	titleId = XamGetCurrentTitleId();
	getPath("\\??\\GAME:", curpath);
	if(strnicmp(curpath, "\\Device\\Package_", strlen("\\Device\\Package_")) == 0)
		isGod = TRUE;
	//DbgPrint("swap disc from tid:%08x called to disk %d proc type %d\n", titleId, ucNextDisc, KeGetCurrentProcessType());
	res = getNextDisk(ucNextDisc, titleId, diskpath, curpath, isGod);
	if(res != SWAP_FIND_FAILED)
	{
		if(res != SWAP_SAME_DISK)
		{
			// cleanup the game link path environment
			if(isGod)
			{
				//DbgPrint("CON detected as mounted\n");
				ret = unmountCon("GAME");
				//DbgPrint("unmount con GAME: returns %08x\n", ret);
			}
			DeleteLinks("GAME:");
			DeleteLinks("D:");

			if(isXex)
			{
				ret = mountPath("D:", diskpath, OBJ_USR_STRING);
				//DbgPrint("sysmount D: ret %x\n", ret);
				ret |= mountPath("GAME:", diskpath, OBJ_USR_STRING);
				//DbgPrint("umount GAME: ret %x\n", ret);
				ret |= mountPath("GAME:", diskpath, OBJ_SYS_STRING);
				//DbgPrint("smount GAME: ret %x\n", ret);
				if(ret == 0)
					swapComplete = TRUE;
			}
			else
			{
				if(res == SWAP_GOD_READY) // already mounted to gswp
				{
					getPath("\\??\\gswp:", godpath);
					ret = mountPath("D:", godpath, OBJ_USR_STRING);
					DeleteLinks("gswp:");
				}
				else
				{
					ret = mountCon("D", MOUNT_NAME, diskpath);
					if(ret == 0)
					{
						//DbgPrint("conmount succeeded!\n");
						getPath("\\??\\D:", godpath);
					}
				}
				if(ret == 0)
				{
					// needs to be mounted to "\\??\\D:" , "\\??\\GAME:",  "\\System??\\GAME:"
					getPath("\\??\\D:", godpath);
					ret = mountPath("GAME:", godpath, OBJ_USR_STRING);
					//DbgPrint("umount D: ret %x\n", ret);
					ret |= mountPath("GAME:", godpath, OBJ_SYS_STRING);
					//DbgPrint("sysmount GAME: ret %x\n", ret);
					if(ret == 0)
					{
						godNeedsUnmount = TRUE;
						swapComplete = TRUE;
					}
				}
				else
				{
					//DbgPrint("CON mount failed! %x (%x)\n", ret, GetLastError());
					swapComplete = FALSE;
				}
			}
		}
		else
		{
			swapComplete = TRUE;
		}

		//DbgPrint("\nPath Audit:\n");
		//char mountedTo[256];
		//getPath("\\??\\D:", mountedTo);
		//getPath("\\??\\GAME:", mountedTo);
		//getPath("\\system??\\GAME:", mountedTo);

		if(swapComplete)
		{
			//DbgPrint("setting swap complete\n");
			ret = KeSetEvent(hSwapComplete, 1, FALSE);
			//DbgPrint("setevent returns: %x\n", ret);
			ObDereferenceObject(hSwapComplete);
		}
		else
		{
			ret = XamSwapDiscOrg(ucNextDisc, hSwapComplete, XSwapDiscErrorText);
			//DbgPrint("swap no good, calling original function ret: %x\n", ret);
			return ret;
		}
	}
	else
		return XamSwapDiscOrg(ucNextDisc, hSwapComplete, XSwapDiscErrorText);
	return 0;
}

BOOL APIENTRY DllMain(HANDLE hInstDLL, DWORD reason, LPVOID lpReserved)
{
	DbgPrint("swapper started!\n");
	switch(reason)
	{
		case DLL_PROCESS_ATTACH:
			XamSwapDiscOrg = (DWORD (*)(DWORD, PHANDLE, PXSWAPDISC_ERROR_TEXT))HookOrd(2600, (DWORD)XamSwapDiscHook, getModuleEat("xam.xex"));
			ExRegisterTitleTerminateNotification(&terminateRegistraion, TRUE);
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
