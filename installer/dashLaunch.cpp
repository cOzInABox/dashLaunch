#include "dashLaunch.h"
#include <stdio.h>
#include "XboxUtil.h"
#include "Resource.h"
#include "Nand.h"
#include "dlDrives.h"
#include "Strings.h"
#include "logging.h"

#define XELL_PHY_BUFSZ		(16*1024*1024)
#define XELL_MAX_SZ			(16384)
#define ELF_MAX_SZ			(XELL_PHY_BUFSZ-XELL_MAX_SZ)

DashLaunch::DashLaunch(VOID)
{
	ResolveEntities();
}

VOID DashLaunch::ResolveEntities(VOID)
{
	m_isLoaded = FALSE;
	m_importsGood = FALSE;
	m_requiresReboot = TRUE;
	m_ldat = NULL;
	if(CheckLoad())
	{
		m_ldat = (pldata)XboxUtil::GetInstance().ResolveFunction(MODULE_LAUNCH, DL_ORDINALS_LDAT);
		if(m_ldat == NULL) return;
		else if(m_ldat->versionMaj == VER_MAJ)
		{
			m_requiresReboot = FALSE;
			dlaunchStartSysModule = (DLAUNCHSTARTSYSMODULE)XboxUtil::GetInstance().ResolveFunction(MODULE_LAUNCH, DL_ORDINALS_STARTSYSMOD);
			dlaunchShutdown = (DLAUNCHSHUTDOWN)XboxUtil::GetInstance().ResolveFunction(MODULE_LAUNCH, DL_ORDINALS_SHUTDOWN);
			dlaunchForceIniLoad = (DLAUNCHFORCEINILOAD)XboxUtil::GetInstance().ResolveFunction(MODULE_LAUNCH, DL_ORDINALS_FORCEINILOAD);
			dlaunchGetNumOpts = (DLAUNCHGETNUMOPTS)XboxUtil::GetInstance().ResolveFunction(MODULE_LAUNCH, DL_ORDINALS_GETNUMOPTS);
			dlaunchGetOptInfo = (DLAUNCHGETOPTINFO)XboxUtil::GetInstance().ResolveFunction(MODULE_LAUNCH, DL_ORDINALS_GETOPTINFO);
			dlaunchGetOptVal = (DLAUNCHGETOPTVAL)XboxUtil::GetInstance().ResolveFunction(MODULE_LAUNCH, DL_ORDINALS_GETOPTVAL);
			dlaunchSetOptVal = (DLAUNCHSETOPTVAL)XboxUtil::GetInstance().ResolveFunction(MODULE_LAUNCH, DL_ORDINALS_SETOPTVAL);
			dlaunchGetOptValByName = (DLAUNCHGETOPTVALBYNAME)XboxUtil::GetInstance().ResolveFunction(MODULE_LAUNCH, DL_ORDINALS_GETOPTVALBYNAME);
			dlaunchSetOptValByName = (DLAUNCHSETOPTVALBYNAME)XboxUtil::GetInstance().ResolveFunction(MODULE_LAUNCH, DL_ORDINALS_SETOPTVALBYNAME);
			dlaunchGetDriveList = (DLAUNCHGETDRIVELIST)XboxUtil::GetInstance().ResolveFunction(MODULE_LAUNCH, DL_ORDINALS_GETDRIVELIST);
			dlaunchGetDriveInfo = (DLAUNCHGETDRIVEINFO)XboxUtil::GetInstance().ResolveFunction(MODULE_LAUNCH, DL_ORDINALS_GETDRIVEINFO);

			if(dlaunchGetDriveInfo != NULL)
			{
				m_importsGood = TRUE;
			}
		}
	}
	else
		m_requiresReboot = FALSE;
	dlDrives::GetInstance().RefreshList();
	CheckVersion();
}

BOOL DashLaunch::CheckLoad(VOID)
{
	if(NT_SUCCESS(XexGetModuleHandle(MODULE_LAUNCH, &m_handle)))
		m_isLoaded = TRUE;
	else
		m_isLoaded = FALSE;
	return m_isLoaded;
}

VOID DashLaunch::Unload(VOID)
{
	if(m_isLoaded && m_importsGood)
	{
		//lDbgPrint("telling dash launch to unhook %08x\n", m_handle);
		dlaunchShutdown();
		XexUnloadImage(m_handle);
		m_importsGood = FALSE;
		m_isLoaded = FALSE;
		m_ldat = NULL;
	}
	CheckVersion();
}

static PVOID loadAddr;
static DWORD loadXsz;
static volatile BOOL loadFinished;
static VOID LoadDashLaunchThread(VOID)
{
	HANDLE hTemp;
	NTSTATUS ret = XexLoadImageFromMemory(loadAddr, loadXsz, "launch.xex", 8, 0, &hTemp);
	if(ret != 0)
		lDbgPrint("loading launch.xex failed! ret: 0x%08x\n", ret);
	loadFinished = TRUE;
}

VOID DashLaunch::Load(VOID)
{
	loadFinished = FALSE;
	if(Resource::GetInstance().GetEmbeddedFile("launch", &loadAddr, &loadXsz))
	{
		HANDLE pthread;
		DWORD pthreadid;
		DWORD sta;
		PBYTE dat = (BYTE*)VirtualAlloc(NULL, loadXsz, MEM_COMMIT|MEM_LARGE_PAGES, PAGE_READWRITE);
		if(dat != NULL)
		{
			XMemCpy(dat, loadAddr, loadXsz);
			loadAddr = (PVOID)dat;
			sta = ExCreateThread(&pthread, 0, &pthreadid, (PVOID) XapiThreadStartup , (LPTHREAD_START_ROUTINE)LoadDashLaunchThread, NULL, 0x2);
			XSetThreadProcessor(pthread, 4);
			SetThreadPriority(pthread, THREAD_PRIORITY_TIME_CRITICAL);
			ResumeThread(pthread);
			CloseHandle(pthread);

			while(loadFinished == FALSE)
				Sleep(60);
			ResolveEntities();
			VirtualFree(dat, 0, MEM_RELEASE);
			if(m_importsGood)
			{
				dlaunchForceIniLoad(NULL);
				//lDbgPrint("resolve entities: m_isloaded %d m_importsgood %d m_ldat %x\n", m_isLoaded, m_importsGood, m_ldat);
			}
		}
	}
}

BOOL DashLaunch::CheckVersion(VOID)
{
	BOOL isVerMatch = FALSE;
	m_isUpToDate = FALSE;
	if(m_ldat)
	{
		wsprintfW(m_currVerString, L"%d.%02d (%d)", m_ldat->versionMaj, m_ldat->versionMin, m_ldat->svnVer);
		if((m_ldat->versionMaj == VER_MAJ)&&(m_ldat->versionMin == VER_MIN)&&(m_ldat->svnVer == VER_SVN))
		{
			isVerMatch = TRUE;
			m_isUpToDate = TRUE;
		}
		else if((m_ldat->versionMaj >= VER_MAJ)&&(m_ldat->versionMin >= VER_MIN)&&(m_ldat->svnVer >= VER_SVN))
		{
			m_isUpToDate = TRUE;
		}
	}
	else
		wsprintfW(m_currVerString, Strings::GetInstance().Lookup(L"misc_norun"));
	return isVerMatch;
}

BOOL DashLaunch::kernelVersionCheck(VOID)
{
	DWORD ctype = 0;
	if(XeKeysGetConsoleType(&ctype) >= 0)
	{
		if(ctype == XEKEY_CONSOLETYPE_RETAIL)
		{
			if(XboxHardwareInfo->BldrMagic == 0x4e4e) // devkit is 0x5E4E
			{
				if(XboxKrnlVersion->Qfe == 0) // devkit is -1 or 0xFF
				{
					int i;
					USHORT vers[KVERS_SUPPORTED] = {_KVER_MACRO};
					for(i = 0; i < KVERS_SUPPORTED; i++)
					{
						if(XboxKrnlVersion->Build == vers[i])
						{
							return TRUE;
						}
					}
				}
			}
		}
	}
	return FALSE;
}

DWORD DashLaunch::GetIniPathValue(VOID)
{
	if(m_importsGood)
	{
		if(m_ldat != NULL)
		{
			return m_ldat->iniPathSel;
		}
	}
	return INVALID_ITEM;
}

BOOL DashLaunch::IsUseFarenheit(VOID)
{
	if(canUseImports())
	{
		DWORD topt = 0;
		dlaunchGetOptValByName("farenheit", &topt);
		if(topt)
			return TRUE;
	}
	return FALSE;
}

PWCHAR DashLaunch::getCurrIni(VOID)
{
	DWORD path = GetIniPathValue();
	if(path == INVALID_ITEM)
		wsprintfW(m_currIniPath, L"%s", Strings::GetInstance().Lookup(L"none"));
	else if (path == FORCED_ITEM)
		wsprintfW(m_currIniPath, L"%s", Strings::GetInstance().Lookup(L"forced"));
	else
	{
		wsprintfW(m_currIniPath, L"%Slaunch.ini", dlDrives::GetInstance().GetDrivePath(path));
	}
	return m_currIniPath;
}

/*
type: con
dev : \Device\Harddisk0\Partition1\
link: \??\dlaunch:\Content\0000000000000000\FFFF0055\00080000\FFFF00550F586558

type: xex
dev : \Device\Mass0\
link: dlaunch:\xexloader_testing\default.xex

launch dev 3 flag 2 path \test\youtube\00007000\B540FBD06726852788F808D6B25FE87F42
launch dev 3 flag 1 path \xexmenu\default.xex
\\Content\\0000000000000000\\584E07D2\\00000002\\
*/
#define INDIE_PATHLEN		44
static const char IndiePath[] = "\\Content\\0000000000000000\\584E07D2\\00000002\\";
static const char IndieLauncher[] = "xsep:\\32000100\\Xna_TitleLauncher.xex";

VOID DashLaunch::LaunchItem(DWORD dev, DWORD flag, PCHAR path)
{
	//lDbgPrint("launch dev %d flag %d path %s\n",dev, flag, path);
	if(dlDrives::GetInstance().MountDrive(dev, "dli:"))
	{
		char lpath[MAX_PATH] = "dli:";
		strcat(lpath, path);
		if(XboxUtil::GetInstance().IsFileExist(lpath))
		{
			if(flag == XEX_PATH)
			{
// 				lDbgPrint("xex launch: %s\n", lpath); 
				XLaunchNewImage(lpath, XLAUNCH_FLAG_CLEAR_LAUNCH_DATA);
			}
			else if(flag == CON_PATH)
			{
				if(strnicmp(IndiePath, path, INDIE_PATHLEN) == 0)
				{
					if(XboxUtil::GetInstance().IsFileExist((PCHAR)IndieLauncher))
					{
						XLAUNCHDATA_CAFEBABE xdl;
						ZeroMemory(&xdl, sizeof(XLAUNCHDATA_CAFEBABE));
						xdl.dwID = 0xcafebabe;
						xdl.deviceId = dlDrives::GetInstance().GetDriveDeviceId(dev);
						if(xdl.deviceId != 0xFFFFFFFF)
						{
							strcpy_s((char*)xdl.Param, 0x2C, &path[INDIE_PATHLEN]);
							//lDbgPrint("launching %s with launchdata 0x%08x %s\n", IndieLauncher, xdl.dwID, xdl.Param);
							if(XSetLaunchData(&xdl, sizeof(XLAUNCHDATA_CAFEBABE)) == S_OK)
							{
								if(m_importsGood)
								{
									DWORD test = 0;
									DWORD bTrue = TRUE;
									if(!dlaunchGetOptValByName("autofake", &test))
									{
										dlaunchSetOptValByName("fakelive", &bTrue);
									}
									else if(test == 0)
										dlaunchSetOptValByName("fakelive", &bTrue);
								}
								XLaunchNewImage(IndieLauncher, 0);
							}
						}
					}
				}
				else
				{
					strcpy(lpath, OBJ_USR_PATH);
					strcat(lpath, "dli:");
					strcat(lpath, path);
					DWORD cl = XamContentLaunchImageFromFileInternal(lpath, DEFAULT_XEX);
					//DWORD cl = XamContentLaunchImageFromFileInternal(lpath, "game.xex");
					lDbgPrint("con launch: %s ret 0x%08x\n", lpath, cl);
				}
			}
			else if(flag == ELF_PATH)
			{
				HANDLE fElf = CreateFile(lpath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if(fElf != INVALID_HANDLE_VALUE)
				{
					PVOID pvxellData;
					DWORD xellSz;
					DWORD elfRead;
					DWORD elfSz = GetFileSize(fElf, NULL);
					DWORD bufSz = (XELL_MAX_SZ+elfSz+0x1F)&~0xF; // add 10 and 16byte align it
					DWORD allocSz = (bufSz+0xFFFF)&~0xFFFF; // 64k align it
					PBYTE Buffer = (PBYTE)XPhysicalAlloc(allocSz, MAXULONG_PTR, 0, MEM_LARGE_PAGES|PAGE_READWRITE|PAGE_NOCACHE);
					PBYTE elfBuf = &Buffer[XELL_MAX_SZ];
					memset(Buffer, 0, allocSz);
					memset(&Buffer[XELL_MAX_SZ-0x10], 'x', 0x10);
					Resource::GetInstance().GetEmbeddedFile("lxell", &pvxellData, &xellSz);
					memcpy(Buffer, pvxellData, xellSz);
					ReadFile(fElf, elfBuf, elfSz, &elfRead, NULL);
					XboxUtil::GetInstance().StartXell(Buffer, bufSz, XELL_DEST_JTAG);
					//XPhysicalFree(Buffer);
				}
			}
		}
// 		else
// 			lDbgPrint("file not found: %s\n", lpath);
	}
}
