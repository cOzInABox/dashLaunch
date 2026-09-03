#include "_hook_includes.h"
#include "LaunchTitleEx.h"

#define XAM_XNOTIFY_BROADCAST_ORD		0x28E // 654 XamNotifyBroadcast
typedef DWORD (*XNOTIFYBROADCASTFUN)(DWORD command, PVOID data);

extern BOOL g_DashSystemSettings;
extern BOOL g_SkipLaunchNotify;
BOOL g_IsHudOpen = FALSE;

void __declspec(naked) XNotifyBroadcastOldVar(void)
{
	__asm{
		li r3, XNOTIFYBROADCAST_VAL
		nop
		nop
		nop
		nop
		nop
		nop
		blr
	}
}
/*

XNotify: 0x00000009 data: 0x00000001 << hud show        XNotifyBroadcast(0x9, (PVOID)1)
XNotify: 0x00000009 data: 0x00000000 << hud not show    XNotifyBroadcast(0x9, (PVOID)0)
XNotifyBroadcast(0xB, NULL) << device change
XexpLoadImageHook load hud.xex ret 0x00000000
XNotify: 0x80040016 data: 0x00000000
XNotify: 0x8000000c data: 0x00000000
XNotify: 0x00000012 data: 0x00000000 << system shutting down
0x8000000D 0x80010014 = title startup notify, media info
0x8000000D == XNotifyTitleStartup
0x80000006 == XampLateTitleTerminateNotification
0x80000010 == XampEarlyTitleTerminateNotification
*/
XNOTIFYBROADCASTFUN XNotifyBroadcastOld = (XNOTIFYBROADCASTFUN)XNotifyBroadcastOldVar;
DWORD XNotifyBroadcastNew(DWORD command, PVOID data)
{
	//XN_SYS_SIGNINCHANGED
	DWORD ret;
	if(command == 9)
		g_IsHudOpen = (BOOL)data;
	ret = XNotifyBroadcastOld(command, data); // let the miniblades close
// 	dbgPrintFake("XNotify: 0x%08x data: 0x%08x ret: 0x%08x g_IsHudOpen: %d\n", command, data, ret, g_IsHudOpen);
	if(command == 0x80040016)
	{
		if((g_IsHudOpen == 1)&&(g_SkipLaunchNotify == FALSE))
		{
			WORD cmd = (WORD)(((DWORD)data)&0xFFFF);
	// 		WORD index = (WORD)(((DWORD)data>>16)&0xFFFF);
	// 		dbgPrintFake("XNotify: 0x%08x index: 0x%04x command: 0x%04x data: 0x%08x\n", command, index, cmd, data);
			//dbgPrintFake("Hud context %x ui: %x\n", XamGetHudContext(), XamIsUIActive());

			if(getOpt(OPT_NXEMINI))
			{
				if(checkLaunchData(NULL, NULL, NULL, 0)||g_DashSystemSettings)
				{
					g_DashSystemSettings = FALSE;
					doLightSync(&g_DashSystemSettings);

					if(data == NULL) // anything else and it's sending a message to dash, like login
					{
						// debounce Y button...
						DWORD but = getButtons(20, FALSE, XINPUT_GAMEPAD_Y);
						if((but & XINPUT_GAMEPAD_RIGHT_SHOULDER) == 0)
						{
							char* xexname = enumButtons(but, NULL);
							if(xexname != NULL)
							{
								// we need to launch helper now...
								LaunchTitleExSaveCall(HELPER_XEX, NULL, NULL, 0);
							}
						}
					}
					else if(cmd == XDASHLAUNCHDATA_COMMAND_SYSTEM_SETTINGS) // system settings
					{
						DWORD but = getButtons(20, FALSE, XINPUT_GAMEPAD_Y);
						if((but & XINPUT_GAMEPAD_RIGHT_SHOULDER) == 0)
						{
							char* xexname = enumButtons(XINPUT_DUMMY_CONFIG, NULL);
							if(xexname != NULL)
							{
								// we need to launch helper now...
								//dbgPrintFake("XNotifyBroadcastNew starting configapp\n");
								LaunchTitleExSaveCall(HELPER_XEX, NULL, NULL, 0);
							}
						}
					}
				}
// 				else
// 					dbgPrintFake("check launchdata returned false, command %x data %x\n", command, data);
			}
		}
	}
	return ret;
}

static BOOL isXnotifyHooked = FALSE;
static DWORD xnotifyBcastOld[4];
void XNotifyBroadcastHook(void)
{
	if(!isXnotifyHooked)
	{
		hookFunctionStartOrd(MODULE_XAM, XAM_XNOTIFY_BROADCAST_ORD, (PDWORD)XNotifyBroadcastOldVar, xnotifyBcastOld, (DWORD)XNotifyBroadcastNew);
		isXnotifyHooked = TRUE;
	}
}

void XNotifyBroadcastUnhook(void)
{
	if(isXnotifyHooked)
	{
		unhookFunctionStartOrd(MODULE_XAM, XAM_XNOTIFY_BROADCAST_ORD, xnotifyBcastOld);
		isXnotifyHooked = FALSE;
	}

}