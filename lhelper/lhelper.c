//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
#include <xtl.h>
#include <stdio.h>
#include "xkelib.h"
#include "../_common.h"

#define BOOTPARS_ORD		13 
typedef void (*DLAUNCHBOOTPARSE)(DWORD pad);
#define INILOAD_ORD			4
typedef VOID (*DLAUNCHFORCEINILOAD)(PCHAR path);

HRESULT Mount(const char* szDrive, const char* szDevice)
{
	STRING DeviceName, LinkName;
	CHAR szDestinationDrive[MAX_PATH];
	RtlSnprintf(szDestinationDrive, MAX_PATH, "\\??\\%s",szDrive);
	RtlInitAnsiString(&DeviceName, szDevice);
	RtlInitAnsiString(&LinkName, szDestinationDrive);
	ObDeleteSymbolicLink(&LinkName);
	return (HRESULT)ObCreateSymbolicLink(&LinkName, &DeviceName);
}

DWORD resolveFunct(PCHAR modname, DWORD ord)
{
	DWORD ret=0, ptr2=0;
	HANDLE hand;
	ret = XexGetModuleHandle(modname, &hand); //xboxkrnl.exe xam.dll?
	if(ret == 0)
	{
		ret = XexGetProcedureAddress(hand, ord, &ptr2 );
		if(ptr2 != 0)
			return ptr2;
	}
	return 0; // function not found
}

DWORD getDigitalInput(void)
{
	DWORD ret = 0;
	int i;
	XINPUT_STATE InputState;
	for(i = 0; i < XUSER_MAX_COUNT; i++)
	{
		if(XInputGetState(i, &InputState) == ERROR_SUCCESS )
		{
// 			DbgPrint("contr %d but 0x%04x\n", i, InputState.Gamepad.wButtons);
			ret |= (InputState.Gamepad.wButtons&0xFFFF);
		}
	}
	return ret;
}

DWORD getPad(void)
{
	DWORD pad = 0;
	int i = 0;
// 	DbgPrint("getpad\n");
	for(; i < 16; i++)
	{
		pad = getDigitalInput();
		//DbgPrint("%d pad: 0x%08x\n", i, pad);
		if(pad != 0)
			break;
		else
			Sleep(100);
	}
// 	DbgPrint("%d pad ret: %08x\n", i, pad);
	return pad;
}

// global so it's automagically 0x0 filled
static XUID usrs[4];
void doSignin(void)
{
	DWORD flag;
	WORD fsz;
	NTSTATUS ret;
	ret = ExGetXConfigSetting(XCONFIG_USER_CATEGORY, XCONFIG_USER_RETAIL_FLAGS, &flag, 4, &fsz);
	if(ret >= 0)
	{
		if(flag&0x40)
		{
// 			DbgPrint("LH: 0x40 flag set\n");
			ret = ExGetXConfigSetting(XCONFIG_USER_CATEGORY, XCONFIG_USER_DEFAULT_PROFILE, usrs, 8, &fsz);
			if(ret >= 0)
			{
				HRESULT hret;
				//DWORD ret;
				//ret = XNotifyDelayUI(12000);
				//DbgPrint("XNotifyDelayUI ret %d\n", ret);
				//ret = XamNotifyDelayUIInternal(12000);
				//DbgPrint("XamNotifyDelayUIInternal ret %d\n", ret);
				hret = XamUserLogon(usrs, 0x80004, NULL);
// 				DbgPrint("LH: logon %016I64x ret %08x\n", usr, hret);
			}
		}
// 		else
// 			DbgPrint("LH: 0x40 flag not set\n");
	}
// 	else
// 		DbgPrint("LH: attempt to get user flags failed, ret %08x\n", ret);
}


/*
type: 0
dev : \Device\Harddisk0\Partition1\
link: \??\dlaunch:\Content\0000000000000000\FFFF0055\00080000\FFFF00550F586558

type: 1
dev : \Device\Mass0\
link: dlaunch:\xexloader_testing\default.xex
*/
VOID __cdecl main()
{
	pldata ldat = (pldata)resolveFunct("launch.xex", 1);
// 	DbgPrint("lhelper\n");
	if(ldat != NULL)
	{
//		DbgPrint("lhelper started opts %x\n", ldat->lhelpOpts);
		if(ldat->ID == LAUNCH_DATA_ID)
		{
			if(ldat->lhelpOpts & LHELPOPT_SIGNIN)
			{
				doSignin();
				ldat->lhelpOpts = ldat->lhelpOpts&~LHELPOPT_SIGNIN;
			}
			if(ldat->lhelpOpts & LHELPOPT_FIRSTBUT)
			{
				DLAUNCHBOOTPARSE dlaunchBootParseButtons = (DLAUNCHBOOTPARSE)resolveFunct("launch.xex", BOOTPARS_ORD);
				if(dlaunchBootParseButtons != NULL)
				{
					//DbgPrint("forcing boot parse 0x%04x\n", ldat->lhelpOpts);
					if(ldat->lhelpOpts & LHELPOPT_FAKEANIM)
						dlaunchBootParseButtons(0);
					else
						dlaunchBootParseButtons(getPad());
				}
			}
			if(Mount(MOUNT_NAME, ldat->dev) >=0)
			{
				//DbgPrint("type: %d\n", ldat->ltype);
				//DbgPrint("dev : %s\n", ldat->dev);
				//DbgPrint("link: %s\n", ldat->link);
				if(ldat->ltype == LHELPER_CON)
					XamContentLaunchImageFromFileInternal(ldat->link, DEFAULT_XEX);
				else
					XLaunchNewImage(ldat->link, 0);
			}
		//	else
			//{
		//		DbgPrint("mounting %s to dlaunch: failed\n", ldat->dev);
			//	DbgPrint("type: %d\n", ldat->ltype);
			//	if(ldat->dev[0] != 0)
			//		DbgPrint("dev : %s\n", ldat->dev);
			//	if(ldat->link[0] != 0)
			//		DbgPrint("link: %s\n", ldat->link);
			//}
		}
	}
}
