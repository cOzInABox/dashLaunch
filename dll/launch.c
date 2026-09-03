/*
	Dash Launch 2 main core code
	[cOz]
*/

#include <xtl.h>
#include <stdio.h>
#include <string.h>
#include <ppcintrinsics.h>
#include "xkelib.h"
#include "../_common.h"
#include "utility.h"
#include "iniparse.h"
#include "tasks/_all_tasks.h"
#include "hooks/_all_hooks.h"
#include "launch.h"
#include "opts.h"
#include "optsHandlers.h"

#define  _HAS_EXCEPTIONS  0

#define XEXLOAD_DASH	"\\Device\\Flash\\dash.xex"
#define XEXLOAD_DASH2	"\\SystemRoot\\dash.xex"
static char dashHdd[MAX_PATH];
static char dashStartup[MAX_PATH];

// a place to export plugin path pointer
extern PLUGIN_LOAD_PATH dlaunchPluginPath = {0, NULL, NULL};

// yeah this method sucks, but it also makes constants more easily accessible
#include "launch_constants.h"

DWORD getButtons(DWORD reps, BOOL stall, WORD debounce)
{
	BOOL bUnk;
	DWORD buttons = 0;
	DWORD cnt = 0;
	DWORD devcontx = 0;
	DWORD packet;
	XINPUT_GAMEPAD Gamepad;
	NTSTATUS contx = -1;
	//ULARGE_INTEGER tbStart;
	//ULARGE_INTEGER tbEnd;
	//DbgPrint("\nstart reps %d stall: %d bounce: %04x\n", reps, stall, bounce);
	//tbStart.QuadPart = __mftb();
	while(cnt < reps)
	{
		if(contx < 0)
		{
			contx = XamUserGetDeviceContext(0, 0, &devcontx);
			cnt++;
		}
		else if(XInputdReadState(devcontx, &packet, &Gamepad, &bUnk) >= 0)
		{
			if(Gamepad.wButtons != 0)
			{
				buttons = (DWORD)(Gamepad.wButtons&0xFFFF);
				if(stall == FALSE)
					return buttons;
			}
			cnt++;
		}
		Sleep(35);
	}
	//tbEnd.QuadPart = __mftb();
	//DbgPrint("rep cnt %d\n", reps);
	//DbgPrint("start tb: %08x %08x\n", tbStart.HighPart, tbStart.LowPart);
	//DbgPrint("end tb  : %08x %08x diff: %08x %08x\n", tbEnd.HighPart, tbEnd.LowPart, (tbEnd.HighPart-tbStart.HighPart), (tbEnd.LowPart-tbStart.LowPart));
	return buttons;
}
//DWORD getButtons(DWORD reps, BOOL stall, WORD debounce)
//{
//	DWORD buttons = 0;
//	WORD bounce;
//	DWORD cnt = 0;
//	PDWORD devcontx = NULL;
//	DWORD packet;
//	XINPUT_GAMEPAD Gamepad;
//	NTSTATUS contx = -1;
//	bounce = debounce; // gonna use this to filter held buttons
//	//ULARGE_INTEGER tbStart;
//	//ULARGE_INTEGER tbEnd;
//	//DbgPrint("\nstart reps %d stall: %d bounce: %04x\n", reps, stall, bounce);
//	//tbStart.QuadPart = __mftb();
//	while(cnt < reps)
//	{
//		if(contx < 0)
//		{
//			contx = XamUserGetDeviceContext(0, 0, &devcontx);
//			cnt++;
//		}
//		else if(XInputdReadState(devcontx, &packet, &Gamepad) >= 0)
//		{
//			if(bounce == 0)
//			{
//				if(Gamepad.wButtons != 0)
//				{
//					buttons = (DWORD)(Gamepad.wButtons&0xFFFF);
//					if(stall == 0)
//						return buttons;
//				}
//				cnt++;
//			}
//			else if((Gamepad.wButtons&bounce) == 0)
//			{
//				bounce = 0;
//				cnt++;
//			}
//		}
//		//YieldProcessor(); // db16cyc
//		//KeStallExecutionProcessor(32500);
//		Sleep(35);
//	}
//	//tbEnd.QuadPart = __mftb();
//	//DbgPrint("rep cnt %d\n", reps);
//	//DbgPrint("start tb: %08x %08x\n", tbStart.HighPart, tbStart.LowPart);
//	//DbgPrint("end tb  : %08x %08x diff: %08x %08x\n", tbEnd.HighPart, tbEnd.LowPart, (tbEnd.HighPart-tbStart.HighPart), (tbEnd.LowPart-tbStart.LowPart));
//	return buttons;
//}

char* enumButtons(DWORD pad, char* xex)
{
// 	char* xexname = xex;
	DWORD item = 0;
// 	if(xex != NULL)
// 		dbgPrintFake("enumbuttons pad %08x g_bootSubvert %08x xex %s firstbut %d\n", pad, g_bootSubvert, xex, ((ldat.lhelpOpts&LHELPOPT_FIRSTBUT) != 0));
// 	else
// 		dbgPrintFake("enumbuttons pad %08x g_bootSubvert %08x firstbut %d\n", pad, g_bootSubvert, ((ldat.lhelpOpts&LHELPOPT_FIRSTBUT) != 0));
	if(g_bootSubvert != 0)
	{
		if((ldat.lhelpOpts&LHELPOPT_FIRSTBUT) == 0)
		{
			if((pad == 0) || (pad == 0xFFFF))
				pad = g_bootSubvert;
			g_bootSubvert = 0;
		}
	}
	if((pad == 0) || (pad == 0xFFFF)) // no controller or no button pressed
		item = DEFAULT;
	else if(pad & XINPUT_GAMEPAD_A) // 0x1000
		item = BUT_A;
	else if(pad & XINPUT_GAMEPAD_B) // 0x2000
		item = BUT_B;
	else if(pad & XINPUT_GAMEPAD_X) // 0x4000
		item = BUT_X;
	else if(pad & XINPUT_GAMEPAD_Y) // 0x8000
		item = BUT_Y;
	else if(pad & XINPUT_GAMEPAD_START) // 0x10
		item = START;
	else if(pad & XINPUT_GAMEPAD_BACK) // 0x20
		item = BACK;
	else if(pad & XINPUT_GAMEPAD_LEFT_SHOULDER) // 0x100
		item = LBUMP;
	else if(pad & XINPUT_GAMEPAD_LEFT_THUMB) // 0x40
		item = LTHUMB;
	else if(pad & XINPUT_GAMEPAD_RIGHT_THUMB) // 0x80
		item = RTHUMB;
	else if(pad & XINPUT_DUMMY_CONSPWR) // 0x10000
		item = CONSPWR;
	else if(pad & XINPUT_DUMMY_GUIDEPWR) // 0x20000
		item = GUIDEPWR;
	else if(pad & XINPUT_DUMMY_CONFIG) // 0x40000
		item = CONFIGFILE;
	else if(pad & XINPUT_DUMMY_FAKEANIM) // 0x80000
		item = BOOTANIM;
	else if(pad & XINPUT_DUMMY_FORCE_DASH) // 0x100000
	{
		ldat.ltype = LHELPER_XEX;
		strcpy_s(ldat.link, MAX_PATH, MOUNT_NAME);
		strcat_s(ldat.link, MAX_PATH, "\\dash.xex");
		strcpy_s(ldat.dev, MAX_PATH, dashStartup);
		return helper;
	}
	// in case user was holding an unconfigured button
	if((launch[item].launchpath[0] == 0) || (launch[item].flags == INVALID_ITEM))
		item = DEFAULT;
	if(launch[item].launchpath[0] != 0)
	{
		if(mountToPath(launch[item].dev, MOUNT_NAME))
		{
			strcpy_s(ldat.link, MAX_PATH, MOUNT_NAME);
			strcat_s(ldat.link, MAX_PATH, launch[item].launchpath);
			//dbgPrintFake("checking exists %s\n", ldat.link);
			if(fileExists(ldat.link))
			{
				if(launch[item].flags == CON_PATH)
				{
					ldat.ltype = LHELPER_CON;
					strcpy_s(ldat.link, MAX_PATH, MOUNT_OBJECT);
					strcat_s(ldat.link, MAX_PATH, launch[item].launchpath);
					strcpy_s(ldat.dev, MAX_PATH, drives[launch[item].dev].mountPath);
					return helper;
				}
				else if(launch[item].flags == XEX_PATH)
				{
					ldat.ltype = LHELPER_XEX;
					strcpy_s(ldat.dev, MAX_PATH, drives[launch[item].dev].mountPath);
					return helper;
				}
			}
			//else
			//	dbgPrintFake("file %s does not exist\n", ldat.link);
		}
		//else
		//	DbgPrint("could not mount %s\n", launch[item].dev);
	}
	return xex;
}

BOOL isXexDash(const char* xexName)
{
	if(xexName != NULL)
	{
		if(stricmp(xexName, XEXLOAD_DASH2) == 0)
			return TRUE;
		else if(stricmp(xexName, XEXLOAD_DASH) == 0)
			return TRUE;
		else if(stricmp(xexName, dashHdd) == 0)
			return TRUE;
	}
	return FALSE;
}

// returns 1 on success
DWORD mountToPath(DWORD mountItem, char* mountName)
{
	DWORD sta;
	if(mountItem >= MOUNT_MAX_ITEMS)
		return 0;
	if(mountItem == MOUNT_HDD)
	{
		if((XboxHardwareInfo->Flags && XBOX_HW_FLAG_HDD) == 0)
			return 0;
	}
	sta = (MountPath(mountName, drives[mountItem].mountPath, FALSE) == S_OK);
	if(sta == 0)
		sta = (MountPath(mountName, drives[mountItem].mountPath, FALSE) == S_OK);
	//DbgPrint("launch.xex: mount %s to %s (%s)\n", mountPath[mountItem], mountName, (sta == 0 ? "Failed" : "Success"));
	return sta;
}

const char* getMountPathItem(int num)
{
	return drives[num].mountPath;
}

void showLaunchData(void)
{
	DWORD dwLaunchDataSize = 0;
	DWORD dwStatus = XGetLaunchDataSize(&dwLaunchDataSize);
	dbgPrintFake("ldata sz: 0x%x ret: 0x%08x\n", dwLaunchDataSize, dwStatus);
	if((dwStatus == ERROR_SUCCESS)&&(dwLaunchDataSize != 0))
	{
		BYTE* pLaunchData = (BYTE*)ExAllocatePoolTypeWithTag(((dwLaunchDataSize+0xFFFF)&~0xFFFF), 'DLND', PoolTypeSystem);
		if(pLaunchData != NULL)
		{
			dwStatus = XGetLaunchData(pLaunchData, dwLaunchDataSize);
			if(dwStatus == ERROR_SUCCESS)
			{
				DWORD i;
				for(i = 0; i < dwLaunchDataSize; i++)
				{
					if(pLaunchData[i] != 0x0)
						dbgPrintFake("i:%x %02x\n", i, pLaunchData[i]);
				}
				dwStatus = XSetLaunchData(pLaunchData, dwLaunchDataSize);
				if(dwStatus != ERROR_SUCCESS)
					dbgPrintFake("set launchdata fail 0x%x!\n", dwStatus);
			}
			else
				dbgPrintFake("get launchdata failed!\n");
			ExFreePool(pLaunchData);
		}
	}
}

BOOL getLaunchData(void)
{
	DWORD dwLaunchDataSize = 0;
	DWORD dwStatus = XGetLaunchDataSize(&dwLaunchDataSize);
	BOOL ret = TRUE, sysset = FALSE;
//	dbgPrintFake("getLaunchData ret %x size %x\n", dwStatus, dwLaunchDataSize);
	if(dwStatus == ERROR_SUCCESS)
	{
		if(dwLaunchDataSize != 0)
		{
			BYTE* pLaunchData = (BYTE*)ExAllocatePoolTypeWithTag(((dwLaunchDataSize+0xFFFF)&~0xFFFF), 'DLND', PoolTypeSystem);// = new(std::nothrow) BYTE[dwLaunchDataSize];
			if(pLaunchData != NULL)
			{
				dwStatus = XGetLaunchData(pLaunchData, dwLaunchDataSize);
				if(dwStatus == ERROR_SUCCESS)
				{
					BYTE cmd = pLaunchData[7]&0xFF;
					/* system settings
					ld: 00000000 00000000 00000000 00000000
					ldata sz: 0x3fc
					i:7 2f
					*/
					//DWORD i;
					//dbgPrintFake("ldata sz: 0x%x\n", dwLaunchDataSize);
					//for(i = 0; i < dwLaunchDataSize; i++)
					//{
					//	if(pLaunchData[i] != 0x0)
					//		dbgPrintFake("i:%x %02x\n", i, pLaunchData[i]);
					//}
					//when dash loads, it sets byte 0x7 of launch data for the specific mode/screen to return to
					//byte 0x4 of launch data is set to 0x10 when returning from dvd to dash (and from nxemini to dash)
					if(cmd != 0x0)
					{
						//dbgPrintFake("ldata command %x\n", cmd);
						if(!getOpt(OPT_NOSYSEXIT))
						{
							ret = FALSE; // causes it to go to dash settings options
							if(cmd == XDASHLAUNCHDATA_COMMAND_SYSTEM_SETTINGS)
							{
								if(launch[CONFIGFILE].flags != INVALID_ITEM)
								{
									sysset = TRUE;
								}
							}
						}
					}
					//else if(pLaunchData[4] == 0x10)  //  livepack.xex and dashnui.xex aren't running when a dvd is playing
					//{
						// dvd was played from NXE, so exit to NXE if user wants that
					if(g_DvdLastLaunch)
					{
						if((getOpt(OPT_DVDEXIT))&&(!getOpt(OPT_NOSYSEXIT)))
						{
							ret = FALSE;
						}
						g_DvdLastLaunch = FALSE;
					}
					//}
					//if(!g_LaunchDashNoExcept) // only set launchdata when going to dash
					// only set launchdata when option is set to true OR going to dash.xex
					if((getOpt(OPT_PASS_LAUNCHD))||(!ret))
					{
						dwStatus = XSetLaunchData(pLaunchData, dwLaunchDataSize);
					}
				}
				ExFreePool(pLaunchData);
			}
		}
	}
	if(XamGetCurrentTitleId() != INSTALLER_TID)
		g_DashSystemSettings = sysset;
	else
		g_DashSystemSettings = FALSE;
	doLightSync(&g_DashSystemSettings);

	return ret;
}

BOOL checkLaunchData(const char * szLaunchPath, const char * szMountPath, const char * szCmdLine, DWORD flags)
{
	g_LaunchDashNoExcept = TRUE;
	if(ldat.lhelpOpts&LHELPOPT_FIRSTBUT)
	{
		ldat.lhelpOpts=ldat.lhelpOpts&~LHELPOPT_FIRSTBUT;
		g_LaunchDashNoExcept = FALSE;
		return FALSE;
	}
//	dbgPrintFake("lpath: %s\nmpath: %s\ncmd: %s\nflag: 0x%08x\n\n", (szLaunchPath != NULL)?szLaunchPath:"NULL", (szMountPath != NULL)?szMountPath:"NULL", (szCmdLine != NULL)?szCmdLine:"NULL", flags);
	// when straight dash loads, all three args are NULL
	// when a natal or other games load, flags is 0x4 szLaunchPath is a pointer, szMountPath if a con is used, szCmdLine 0x0
	// launching a normal game or dvd video, flags set to 0x12, other 0x0
	// eject game, szLaunchPath set and flags 0x2000, other 0x0
	// eject dvd vid all args 0x0
	// when exiting arcade to dashboard, flags is set to 0x104, other 0x0
// 	dbgPrintFake("ld: %08x %08x %08x %08x\n", szLaunchPath, szMountPath, szCmdLine, flags);
// 	showLaunchData();
	//if(szLaunchPath != NULL)
	//	dbgPrintFake("szLaunchPath: %s %02x %02x %02x\n", szLaunchPath, szLaunchPath[0], szLaunchPath[1], szLaunchPath[2]);
	// system settings case - does not set launchdata when done internally in dash
	//ld: 00000000 00000000 00000000 00000000
	//ldata sz: 0x3fc
	//i:7 2f

	// media center case
	//ld: fdd53000 00699000 92031004 00000001
	//szLaunchPath: XEX2 58 45 58
	if((flags == 1)&&(szCmdLine != NULL))
	{
		g_LaunchDashNoExcept = FALSE;
	}
	// check for kinect game launches which call on dash.xex
	else if((flags == 0x4)&&(szLaunchPath != NULL))
	{
		if(szLaunchPath[0] != 0) // when homebrew exits it went to NXE, szLaunchPath[0]=0x0
		{
			g_LaunchDashNoExcept = FALSE;
		}
	}
	else if(szMountPath != NULL)
	{
		if(strnicmp(szMountPath, "XSYSLAUNCH", strlen("XSYSLAUNCH")) == 0)
			g_LaunchDashNoExcept = FALSE;
// 		DbgPrint("szMountPath: %s\n", szMountPath);
	}
	else if((szLaunchPath == NULL)&&(szCmdLine == NULL)) // &&(szMountPath == NULL)
	{
		// when dvd player loads a dvd (game or movie), flags is 0x12 and 3 args are NULL
		if((flags&0x2) != 0) // ((flags == 0x12)||(flags == 0x2))
		{
// 			dbgPrintFake("dvd load detected\n");
			g_LaunchDashNoExcept = FALSE;
			g_DvdLastLaunch = TRUE;
		}
		else
		{
			// arcade exit
			if((flags == 0x104)&&(getOpt(OPT_XBLAEXIT)))
			{
				g_LaunchDashNoExcept = FALSE;
			}
			else
			{
				g_LaunchDashNoExcept = getLaunchData();
// 				dbgPrintFake("launch no execpt: %d\n", g_LaunchDashNoExcept);
			}
		}
	}
	return g_LaunchDashNoExcept;
}

void loadPlugins(void)
{
	DWORD i;
	for(i = 0; i < NUM_PLUGINS_SUPPORTED; i++)
	{
		DWORD keyItem = PLUGIN1+i;
		if(launch[keyItem].flags == ITEM_FOUND)
		{
			if(mountToPath(launch[keyItem].dev, MOUNT_NAME))
			{
				DWORD sta;
				char tempName[MAX_PATH] = MOUNT_NAME;
				strcat_s(tempName, MAX_PATH, launch[keyItem].launchpath);
				if(fileExists(tempName))
				{
					dlaunchPluginPath.devicePath = drives[(launch[keyItem].dev)].mountPath;
					dlaunchPluginPath.iniPath = launch[keyItem].launchpath;
					dlaunchPluginPath.magic = PLUGIN_LOAD_PATH_MAGIC;
					sta = XexLoadImage(tempName, 8, 0, NULL);
					dlaunchPluginPath.magic = 0;
					dlaunchPluginPath.devicePath = NULL;
					dlaunchPluginPath.iniPath = NULL;
					if(sta == 0)
					{
						launch[keyItem].flags = PLUGIN_LOADED;
					}
// 					else
// 						dbgPrintFake("loading '%s%s' returned %08x\n", drives[launch[keyItem].dev].iniName, launch[keyItem].launchpath, sta);
				}
				//else
				//	dbgPrintFake("plugin file not exist: %s\n", tempName);
			}
			//else
			//	dbgPrintFake("plugin drive mount failed!\n");
		}
	}
}

void doPatches(void)
{
	u32 * addr;
	u32 len;
	DWORD i=1, j;
	BOOL doPatches = TRUE;

	if(g_patches != NULL)
	{
		while(doPatches)
		{
			if((g_patches[i] == 0x0)||(g_patches[i] == 0xFFFFFFFF)) // invalid address, end of file marker
			{
				doPatches = FALSE;
			}
			else
			{
				addr = (u32*)(g_patches[i]);
				if(MmIsAddressValid(addr))
				{
					i++;
					len = g_patches[i];
					i++;
					for(j = 0; j < len; j++)
					{
						addr[j] = g_patches[i];
						i++;
					}
					doSync(addr);
				}
				else // abort on invalid address
				{
					doPatches = FALSE;
				}
			}
		}
		ExFreePool(g_patches);
	}
}

BOOL loadPatches(char* path)
{
	HANDLE file;
	if(!(fileExists(path)))
		return FALSE;
	file = CreateFile(path,GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file != INVALID_HANDLE_VALUE)
	{
		g_patches = (PDWORD)ExAllocatePoolTypeWithTag(((MAX_PATCH_SZ+0xFFFF)&~0xFFFF), 'DLNP', PoolTypeSystem);
		if(g_patches != NULL)
		{
			DWORD bRead = 0;
			ZeroMemory(g_patches, MAX_PATCH_SZ*4);
			ReadFile(file, g_patches, 0x1000*4, &bRead, NULL);
			CloseHandle(file);

			if(bRead > 12)
			{
				if(((bRead %4) == 0))
				{
					if(g_patches[0] == ldat.targetKernel)
					{
						//DbgPrint("Patches loaded, version 0x%08x == 0x%08x(%d)\n", g_patches[0], ldat.targetKernel, ldat.targetKernel);
						return TRUE;
					}
					//else
					//	DbgPrint("Patches not loaded, version 0x%08x != 0x%08x(%d)\n", g_patches[0], ldat.targetKernel, ldat.targetKernel);
				}
			}

			//if(bRead < 12) // not enough data for a patch
			//	ExFreePool(g_patches);
			//else if((bRead %4) != 0) // unaligned, not valid patch file
			//	ExFreePool(g_patches);
			ExFreePool(g_patches);
			g_patches = NULL;
		}
	}
	return FALSE;
}

DWORD checkXexCon(char* fileName)
{
	DWORD typeBuf, bRead;
	HANDLE hFile = CreateFile(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		ReadFile(hFile, &typeBuf, 0x4, &bRead, NULL);
		CloseHandle(hFile);
		if(typeBuf == 'XEX2')
			return XEX_PATH;
		else if(typeBuf == 'LIVE')
			return CON_PATH;
	}
	return INVALID_ITEM;
}

void setupPkeyPath(const char* inistr, pkeydata pkey, DWORD type)
{
	//typedef struct _keydata {
	//	char launchpath[MAX_PATH];
	//	DWORD flags;
	//	DWORD dev;
	//} keydata, *pkeydata;
	pkey->flags = INVALID_ITEM;
	pkey->rootDev = INVALID_ITEM;
	if((inistr != NULL)&&(inistr[0] != 0))
	{
		//if(stricmp(inistr, "noplugins") == 0)
		//{
		//	pkey->dev = INVALID_ITEM;
		//	strcpy(pkey->launchpath, inistr);
		//	dbgPrintFake("setting skipplugins\n");
		//	g_SkipPlugins = TRUE;
		//}
		//else
		//{
			DWORD j;
			DWORD devNum = INVALID_ITEM;
			DWORD testNum = 0;
			// figure out which mount point was used
			for(j = 0; j < MOUNT_MAX_ITEMS;)
			{
				int namelen = strlen(drives[j].iniName);
				if(strnicmp(inistr, drives[j].iniName, namelen) == 0)
				{
					devNum = j;
					testNum = drives[j].nextCat;
					pkey->rootDev = j;
					strcpy_s(pkey->launchpath, MAX_PATH, (inistr+namelen));
					j = MOUNT_MAX_ITEMS;
				}
				else
					j+=drives[j].nextCat;
			}
			// check if the file exists
			if(devNum != INVALID_ITEM)
			{
				char tempName[MAX_PATH];
				strcpy(tempName, MOUNT_NAME);
				strcat_s(tempName, MAX_PATH, pkey->launchpath);
				for(j = 0; j < testNum; j++)
				{
					mountToPath(devNum, MOUNT_NAME);
					if(driveExists(MOUNT_DRIVE))
					{
						pkey->dev = devNum;
						if(fileExists(tempName))
						{
							if(type == DL_OPT_TYPE_PATHQLB)
								pkey->flags = checkXexCon(tempName);
							else
								pkey->flags = ITEM_FOUND;
							j = testNum;
						}
						else
						{
							if(type != DL_OPT_TYPE_PATH) // PATH type items need not exist for them to be valid
								pkey->dev = INVALID_ITEM;
						}
					}
					devNum++;
				}
			}
		//}
	}
}

void seekIniPatch(PBOOL config, PBOOL patch)
{
	char tmp[MAX_PATH];
	DWORD i;
	BOOL patchLoaded = *patch;
	BOOL configLoaded = *config;
	for(i=0; i < MOUNT_MAX_INI; i++)
	{
		if((!patchLoaded)||(!configLoaded))
		{
			if(mountToPath(i, MOUNT_NAME))
			{
				if(!patchLoaded)
				{
					strcpy_s(tmp, MAX_PATH, MOUNT_NAME);
					strcat_s(tmp, MAX_PATH, PATCH_NAME);
					patchLoaded = loadPatches(tmp);
				}
				if(!configLoaded)
				{
					strcpy_s(tmp, MAX_PATH, MOUNT_NAME);
					strcat_s(tmp, MAX_PATH, INI_NAME);
					configLoaded = iniStart(tmp);
					if(configLoaded)
					{
						ldat.iniPathSel = i;
					}
				}
			}
		}
	}
	*patch = patchLoaded;
	*config = configLoaded;
}

BOOL seekIniOnPath(PCHAR path)
{
	char tmp[MAX_PATH];
	if(MountPath(MOUNT_NAME, path, FALSE) == S_OK)
	{
		strcpy_s(tmp, MAX_PATH, MOUNT_NAME);
		strcat_s(tmp, MAX_PATH, INI_NAME);
		return iniStart(tmp);
	}
	return FALSE;
}

void runtimeRunTasks(PCHAR path)
{
	BOOL configLoaded = FALSE;
	BOOL patchLoaded = TRUE; // dummy so it doesn't look for it
	ldat.iniPathSel = INVALID_ITEM;
	g_FirstRun = FALSE;

	// look for patches and ini on usb/hdd/flash in that order
	if(path != NULL)
	{
		if(seekIniOnPath(path))
		{
			int i;
			configLoaded = TRUE;
			ldat.iniPathSel = FORCED_ITEM;
			for(i = 0; i < MOUNT_MAX_ITEMS; i++)
			{
				if(strcmp(drives[i].mountPath, path) == 0)
				{
					ldat.iniPathSel = i;
					i = MOUNT_MAX_ITEMS;
				}
			}
		}
	}
	if(!configLoaded)
		seekIniPatch(&configLoaded, &patchLoaded);
	// parse and unload config ini if it was found
	if(configLoaded)
	{
		initOpts(); // reset all the options
		setOptsFromIni();
		iniEnd(); // deallocate everything ini parser used
	}
}

void getPowerOnCause(void)
{
	BYTE smcresp[0x10];
	ZeroMemory(smcresp, 0x10);
	HalGetPowerUpCause(smcresp);
	g_PowerOnReason = smcresp[1]; 
}

void firstRunAutoSignin(void)
{
	DWORD flag;
	WORD fsz;
	NTSTATUS ret;
	ret = ExGetXConfigSetting(XCONFIG_USER_CATEGORY, XCONFIG_USER_RETAIL_FLAGS, &flag, 4, &fsz);
	if(ret >= 0)
	{
		if(flag&0x40)
		{
			XUID usr;
// 			dbgPrintFake("0x40 flag set\n");
			ret = ExGetXConfigSetting(XCONFIG_USER_CATEGORY, XCONFIG_USER_DEFAULT_PROFILE, &usr, 8, &fsz);
			if(ret >= 0)
			{
				ldat.lhelpOpts = ldat.lhelpOpts|LHELPOPT_SIGNIN;
			}
		}
// 		else
// 			dbgPrintFake("0x40 flag not set\n");
	}
// 	else
// 		dbgPrintFake("attempt to get user flags failed, ret %08x\n", ret);
}

//static DWORD patchSocketStartKey[] = {0x897E0001, 0x716B00F6, 0x997E0001};
//static DWORD patchSocketStartData[] = {ASM_NOP, ASM_NOP, ASM_NOP};
char* firstRunTasks(const char* xex)
{
	BOOL configLoaded = FALSE;
	BOOL patchLoaded = FALSE;
	DWORD pad = 0;
	const char * cptr = strrchr(xex, '\\');
	g_bootSubvert = 0;
	ldat.iniPathSel = INVALID_ITEM;
	doSync(&ldat.lhelpOpts);
	//dbgPrintFake("startup xex is %s\n", xex);
	strncpy(dashStartup, xex, ((DWORD)(cptr-xex)+1));
	// always first run helper to ensure button catch and login
	ldat.lhelpOpts = ldat.lhelpOpts|LHELPOPT_FIRSTBUT;
	//dbgPrintFake("startup is %s\n", dashStartup);
	if(g_PowerOnReason == SMC_PWR_REAS_24_WINBTN)
		pad = XINPUT_DUMMY_FORCE_DASH;
	// debugging powerup cause
	//dbgPrintFake("powerup cause: %02x\n", g_PowerOnReason);
	//for(i=0; i<0x10; i++)
	//{
	//	DbgPrint("%02x ", smcresp[i]);
	//}
	//DbgPrint("\n");
	
	//if(mountToPath(MOUNT_HDD, MOUNT_NAME)) // XBDMLOAD this shouldn't be here
	//{
	//	DWORD sta;
	//	sta = XexLoadImage("dlaunch:\\xbdm.xex", 8, 0, NULL);
	//	dbgPrintFake("attempt to load xbdm.xex from hdd ret 0x%08x\n", sta);
	//}
	//else
	//	dbgPrintFake("mount hdd failed!\n");
// 	dbgPrintFake("firstRunTasks\n");
	// wait for usb to come up... UsbdDriverLoadRequiredEvent 753
	KeWaitForSingleObject(UsbdBootEnumerationDoneEvent, UserRequest, PROC_USER, 0, NULL);
	Sleep(1000);
	// look for patches and ini on usb/hdd/flash in that order
	seekIniPatch(&configLoaded, &patchLoaded);

	// check to see if there was patchdata and execute patches if there were
	if(patchLoaded)
		doPatches();

	// parse and unload config ini if it was found
	if(configLoaded == TRUE)
	{
		DWORD remNxe = 0;
		setOptsFromIni();
		dlaunchGetOptValByName("remotenxe", &remNxe);

		if(remNxe)
		{
			if((g_PowerOnReason == SMC_PWR_REAS_20_REMOPWR)||(g_PowerOnReason == SMC_PWR_REAS_22_REMOX))
				pad = XINPUT_DUMMY_FORCE_DASH;
		}

		// last but not least, load the parsed plugins
		if(!g_SkipPlugins)
			loadPlugins();
		iniEnd(); // deallocate everything ini parser used
	}
	g_FirstRun = FALSE;

	if(pad == 0)
	{
		// console power button pushed
		if((g_PowerOnReason==SMC_PWR_REAS_11_PWRBTN)||(g_PowerOnReason==SMC_PWR_REAS_20_REMOPWR))
		{
			if(launch[CONSPWR].flags != INVALID_ITEM)
				pad = XINPUT_DUMMY_CONSPWR;
		}
		// guide button pushed, controller or remote
		else if((g_PowerOnReason==SMC_PWR_REAS_22_REMOX)||((g_PowerOnReason>=SMC_PWR_REAS_55_WIRELESS)&&(g_PowerOnReason<=SMC_PWR_REAS_5A_WIRED_R1)))
		{
			if(launch[GUIDEPWR].flags != INVALID_ITEM)
				pad = XINPUT_DUMMY_GUIDEPWR;
		}
	}
	//if(pad == XINPUT_DUMMY_FORCE_DASH)
	//	ldat.lhelpOpts |= LHELPOPT_FORCEDASH;
		g_bootSubvert = pad;

	if(launch[BOOTANIM].flags != INVALID_ITEM)
	{
		pad = XINPUT_DUMMY_FAKEANIM;
		ldat.lhelpOpts |= LHELPOPT_FAKEANIM;
	}
	//dbgPrintFake("CONSPWR %d GUIDEPWR %d BOOTANIM %d g_bootSubvert %08x pad %08x\n", launch[CONSPWR].flags, launch[GUIDEPWR].flags, launch[BOOTANIM].flags, g_bootSubvert, pad);
	// verify auto signin will complete...
	firstRunAutoSignin();
	enumButtons(pad, NULL);
	//if(!patchModuleSearchkeyByName(MODULE_XAM, patchSocketStartKey, 3, 0, patchSocketStartData, 3))
	//	dbgPrintFake("warning could not patch socket startup in xam!!\n");
// 	dbgPrintFake("lhelpOpts are %x\n", ldat.lhelpOpts);
	return helper;
}

extern void dlaunchBootParseButtons(DWORD buttons)
{
	if(ldat.lhelpOpts & LHELPOPT_FAKEANIM)
	{
		ldat.lhelpOpts &= ~LHELPOPT_FAKEANIM;
	}
	else
	{
		ldat.link[0] = 0;
		if(ldat.lhelpOpts & LHELPOPT_FORCEDASH)
		{
			ldat.lhelpOpts &= ~LHELPOPT_FORCEDASH;
		}
		else
		{
			if(ldat.lhelpOpts & LHELPOPT_FIRSTBUT)
				ldat.lhelpOpts &= ~LHELPOPT_FIRSTBUT;
			if((buttons&XINPUT_GAMEPAD_RIGHT_SHOULDER) == 0)
				enumButtons(buttons, NULL);
			else
				g_bootSubvert = 0; // in case held right shoulder at boot
		}
		if((ldat.link[0] == 0)||(ldat.dev[0] == 0))
		{
			enumButtons(XINPUT_DUMMY_FORCE_DASH, NULL);
		}
	}
//  	dbgPrintFake("dlaunchBootParseButtons pad %08x\n", pad);
}

extern void dlaunchForceIniLoad(PCHAR path)
{
	runtimeRunTasks(path);
}

// ** for export to load system dll **
void launchSysModThread(char* modPath)
{
	g_status[0] = XexLoadImage(modPath,8,0,NULL);
	g_status[1] = 1;
	//DbgPrint("launch: load returns %x", g_status[0]);
	doLightSync(g_status);
}

// ** for export to load system dll **
extern DWORD dlaunchStartSysModule(char* modPath)
{
	HANDLE pthread;
	DWORD pthreadid;
	g_status[0] = 0;
	g_status[1] = 0;
	doLightSync(g_status);
	//DbgPrint("launch: attempting to load %s", modPath);
	// DISABLED WARNING ABOUT FUNCTION CAST (4057) for XapiThreadStartup
	ExCreateThread(&pthread, 0, &pthreadid, (PVOID) XapiThreadStartup , (LPTHREAD_START_ROUTINE)launchSysModThread, modPath, 0x2);
	XSetThreadProcessor(pthread, 4);
	SetThreadPriority(pthread, THREAD_PRIORITY_TIME_CRITICAL);
	ResumeThread(pthread);
	CloseHandle(pthread);

	// wait for thread to run it's course
	while(g_status[1] == 0)
	{
		Sleep(100);
	}

	return g_status[0];
}

//#define _EARLY_DEBUG_OUT 1
#ifdef _EARLY_DEBUG_OUT
#define XEX_LOAD_EXECUTEABLE_ORD		0x198 // 408
#define XEX_LOAD_IMAGE_ORD				0x199 // 409
static IMPORT_HOOK_SAVE loadExecSave;
DWORD XexLoadExecutableHook(PCHAR xexName, PHANDLE handle, DWORD typeInfo, DWORD ver)
{
	DWORD ret;
	ret = XexLoadExecutable(xexName, handle, typeInfo, ver);
	dbgPrintFake("xam loadexec: %s h:%08x t:%08x v:%08x ret: %08x\n", xexName, handle, typeInfo, ver, ret);
	return ret;
}

static IMPORT_HOOK_SAVE loadImageSave;
DWORD XexLoadImageHook(PCHAR xexName, DWORD typeInfo, DWORD ver, PHANDLE modHandle)
{
	DWORD ret;
	ret = XexLoadImage(xexName, typeInfo, ver, modHandle);
	dbgPrintFake("xam loadimage: %s t:%08x v:%08x h:%08x ret: %08x\n", xexName, typeInfo, ver, modHandle, ret);
	return ret;
}
#endif

//if(sta == 0xC0000102)
//{
//	HvxSetState(SYSCALL_KEY, SET_DEV_KEY, 0, 0, 0);
//	sta = XexLoadImage(plugpath, 8, 0, NULL);
//	DbgPrint("loading plugin %s (dev) returned %08x\n", plugpath, sta);
//	HvxSetState(SYSCALL_KEY, SET_RETAIL_KEY, 0, 0, 0);
//}

static DWORD dlaunchShutdownSta;
void dlaunchDoShutdown(void)
{
	PLDR_DATA_TABLE_ENTRY self;
	if(g_usePatch < KVERS_SUPPORTED)
	{
		DWORD pbNeg = 0, pbPos = 1;
		genericRevokeCheckUnpatch();
		genericNxeInstallUnpatch();

		optsHandlePingPatch(&pbNeg);
		optsHandleNoHud(&pbNeg);
		optsHandleNoUpdater(&pbNeg);
		optsHandleXhttp(&pbNeg);

		/* ********* SHUTDOWN TASKS ********* */
		// hdalive.c
		endHdAliveTask();
		// tempbcast.c
		endTempBroadcast();

		/* ********* UNHOOK EVERYTHING ********* */
		// except.c
		optsHandleExchandler(&pbNeg); // exceptTrapUnhook();
		optsHandleFatalFreeze(&pbNeg); // exceptBugcheckUnhook
		// XexHook.h
		XexUnhook();
		// LaunchTitleEx.c
		launchTitleExUnhook();
		// loadPrep.h
		loadPrepUnhook();
		optsHandleRegionSpoof(&pbNeg); //loadPrepRegionUnhook();
		// privs.h
		privsUnhook();
		// signin_state.h
		optsHandleFakeLive(&pbNeg); // signinStateUnhook();
		// XamAppLoad.h
		optsHandleSignNotice(&pbNeg); // xamAppLoadUiUnhook();
		xamAppLoadUnhook();
		// xamLicense.h
		xamLicenseUnhook();
		// xnetDns.h
		XNetDnsUnhook();
		// XNotifyBroadcast.h
		XNotifyBroadcastUnhook();
		// XSecurity.h
		XSecurityUnhook();
		// HalReboot.h
		optsHandleSafeReboot(&pbPos); // HalRebootUnhook();
		// XamObfuscate.h
		optsHandleDevProfiles(&pbNeg); // xamObfuscateUnhook();
		optsHandleDevSyslink(&pbNeg); // xamObfuscateSyslinkUnook();
		// multidisk.h
		optsHandleMultidisk(&pbNeg); // multidiskHook();
		// XamExGetConfig.h
		optsHandleOobe(&pbNeg); // XamOobeUnhook();
		// misc
		optsHandlePreview(&pbNeg); // XamSetStagingMode();
		disableFakeLive();
#ifdef _EARLY_DEBUG_OUT
		unhookImpStub(&loadExecSave);
		unhookImpStub(&loadImageSave);
#endif
	}
	deleteLink(MOUNT_DUMP, TRUE);
	deleteLink(MOUNT_NAME, TRUE);
	deleteLink(MOUNT_ALIVE, TRUE);
	self = (PLDR_DATA_TABLE_ENTRY)GetModuleHandle(MODULE_LAUNCH);
	if(self != NULL)
	{
		self->LoadCount = 1;
		//*(WORD*)((DWORD)self + 64) = 1;
	}
	dbgPrintFake("DashLaunch V%d.%02d.%d shutdown!\n", VER_MAJ, VER_MIN, VER_SVN);
	dlaunchShutdownSta = 1;
	doLightSync(&dlaunchShutdownSta);
}

extern void dlaunchShutdown(void)
{
	HANDLE pthread;
	DWORD pthreadid;
	DWORD sta;
	dlaunchShutdownSta = 0;
	doLightSync(&dlaunchShutdownSta);
	sta = ExCreateThread(&pthread, 0, &pthreadid, (PVOID) XapiThreadStartup , (LPTHREAD_START_ROUTINE)dlaunchDoShutdown, NULL, 0x2);
	XSetThreadProcessor(pthread, 4);
	ResumeThread(pthread);
	CloseHandle(pthread);

	// wait for thread to run it's course
	while(dlaunchShutdownSta == 0)
	{
		Sleep(50);
	}
}

extern DWORD dlaunchGetDriveInfo(PDWORD maxIniDrives, PDWORD maxDevLen)
{
	if(maxIniDrives)
		*maxIniDrives = MOUNT_MAX_INI;
	if(maxDevLen)
	{
		DWORD len = 0;
		int i;
		for(i = 0; i < MOUNT_MAX_ITEMS; i++)
		{
			if(strlen(drives[i].mountPath) > len)
				len = strlen(drives[i].mountPath);
		}
		*maxDevLen = len;
	}
	return MOUNT_MAX_ITEMS;
}

extern DWORD dlaunchGetDriveList(DWORD dev, PCHAR devDest, PCHAR mountName, PCHAR friendlyName)
{
	if(dev < MOUNT_MAX_ITEMS)
	{
		if(devDest)
			strcpy(devDest, drives[dev].mountPath);
		if(mountName)
			strcpy(mountName, drives[dev].iniName);
		if(friendlyName)
			strcpy(friendlyName, drives[dev].friendlyName);
	}
	else
		return 0;
	return MOUNT_MAX_ITEMS;
}

BOOL LhelperExists(VOID)
{
	BOOL ret = FALSE;
	MountPath("tmt:", "\\Device\\Flash\\", FALSE);
	if(fileExists("tmt:\\lhelper.xex"))
		ret = TRUE;
	else
		dbgPrintFake("DashLaunch V%d.%02d.%d !NOT! starting, lhelper.xex missing!\n", VER_MAJ, VER_MIN, VER_SVN);
	deleteLink("tmt:", FALSE);
	return ret;
}

VOID launchStartup(int i)
{
	//HANDLE hListen;
	//BOOL bPos = TRUE; //, bNeg = FALSE;
	g_usePatch = i;
	g_DvdLastLaunch = FALSE;
	ldat.targetKernel = pats[g_usePatch].kernelBuild;
	ldat.DebugRoutine = kernpat[pats[g_usePatch].kpatch].DebugRoutine;
	ldat.DebugStepPatch = kernpat[pats[g_usePatch].kpatch].DebugStepPatch;
	genericKHealthSetAddr(pats[g_usePatch].XamKinectHealth);

	//dbgPrintFake("launch entry!\nusing patches for %d:%d!\n", pats[g_usePatch].kernelBuild, XboxKrnlVersion->Build);
	initOpts(); // reset all the options
	// using this to catch exit from miniblade while in NXE, first hook because it needs to be done before hud loads up
	XNotifyBroadcastHook();

	// using this to catch dash.xex and xbox emu loading
	loadPrepHook((PDWORD)pats[g_usePatch].LoaderPrepare); // patch hv relocate
	// using this to catch launch data to find out if we are returning to dash or not
	// by the time LoaderPrepare fires launchdata has already been dealt with by xam
	launchTitleExHook();
	XexHook();

	// removes xex header flag revoke check, hv patch required!
	genericRevokeCheckPatch((PDWORD)(pats[g_usePatch].XamRevokePatch)); // patch hv

	XNetDnsHook();

#ifdef _EARLY_DEBUG_OUT
	hookImpStub(MODULE_XAM, MODULE_KERNEL, XEX_LOAD_EXECUTEABLE_ORD, (DWORD)XexLoadExecutableHook, &loadExecSave);
	//				hookImpStub(MODULE_XAM, MODULE_KERNEL, XEX_LOAD_IMAGE_ORD, (DWORD)XexLoadImageHook, &loadImageSave);
	//DbgPrint("DashLaunch V%d.%02d.%d started!\n", VER_MAJ, VER_MIN, VER_SVN);
#endif

	// to modify privileges more easily...
	privsHook();
	xamAppLoadHook(); // unpatch relocation

	// turning off CIV...
	XSecurityHook();
	scheduleTempBroadcast(); // start up the temp monitor with defaults
	genericNxeInstallPatch(pats[g_usePatch].XamContLivePirs, pats[g_usePatch].XamContDevice);

	// some logging functions
	getPowerOnCause();
	RtlSnprintf(dashHdd, 0x100,"\\Device\\Harddisk0\\SystemExtPartition\\%08X\\dash.xex", XamUpdateGetCurrentSystemVersion());
	genericNoNewUpdateSetAddr(pats[g_usePatch].NoNewUpdate);
	xamLicenseHook((PDWORD)pats[g_usePatch].LicenseCheck);

	dbgPrintFake("DashLaunch V%d.%02d.%d started!\n", VER_MAJ, VER_MIN, VER_SVN);

	//dbgPrintFake("dashHdd %s\n", dashHdd);
	g_kernSupported = TRUE;
	//hListen = XNotifyCreateListener()
}

BOOL APIENTRY DllMain(HANDLE hInstDLL, DWORD reason, LPVOID lpReserved)
{
	if(reason == DLL_PROCESS_ATTACH)
	{
		int i;
		BOOL mainAtch = FALSE;

		for(i = 0; i < KVERS_SUPPORTED; i++)
		{
//  		dbgPrintFake("checking %d == %d\n", pats[i].kernelBuild, XboxKrnlVersion->Build);
			if(pats[i].kernelBuild == XboxKrnlVersion->Build)
			{
				if(LhelperExists())
				{
					launchStartup(i);
					i = KVERS_SUPPORTED;
					mainAtch = TRUE;
				}
			}
		}
		if(mainAtch == FALSE)
			dbgPrintFake("DashLaunch V%d.%02d.%d failed to match kernel %d\n", VER_MAJ, VER_MIN, VER_SVN, XboxKrnlVersion->Build);
	}
	else if(reason == DLL_PROCESS_DETACH)
	{
		dbgPrintFake("DashLaunch V%d.%02d.%d unloading!\n", VER_MAJ, VER_MIN, VER_SVN);
	}
	return TRUE;
}

