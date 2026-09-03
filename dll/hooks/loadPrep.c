#include "_hook_includes.h"
#include "loadPrep.h"

typedef DWORD (*LOADPREPSAVEFUN)(DWORD argR3, char *xex, DWORD argR5, PVOID handle, DWORD typeinfo, DWORD ver, DWORD argR9, DWORD argR10, DWORD argSt1);

extern BOOL g_LaunchDashNoExcept;
extern BOOL g_DashSystemSettings;
extern BOOL g_FirstRun;

static BOOL g_overrideRegion = FALSE;
static BOOL isRegionHooked = FALSE;
static DWORD g_Region = 0x7FFF;
static BOOL g_Protection = FALSE;

DWORD XGetGameRegionNew(void)
{
	if(g_overrideRegion)
	{
		return g_Region;
	}
	else
	{
		// we can call the original function because it was linked before xam hook was put in place
		return XGetGameRegion();
	}
}

void loadPrepSetRegion(DWORD region)
{
	g_Region = region;
}

DWORD loadPrepGetRegion(void)
{
	return g_Region;
}

void __declspec(naked) loadPrepSaveVar(void)
{
	__asm{
		li r3, LOADPREPSAVE_VAL
		nop
		nop
		nop
		nop
		nop
		nop
		blr
	}
}
LOADPREPSAVEFUN loadPrepSave = (LOADPREPSAVEFUN)loadPrepSaveVar;
// compat only checked if st1 is not 0
DWORD loadPrep(DWORD argR3, const char *xex, DWORD argR5, PVOID handle, DWORD typeinfo, DWORD ver, DWORD argR9, DWORD argR10, DWORD argSt1)
{
	BOOL isDash;
	DWORD pad;
	char* xexname = (char*)xex;
	//BOOL isDash = (strcmp(xex, DASH_XEX) == 0);//if(strstr(xex, "\\dash.xex") != NULL)
	g_overrideRegion = FALSE;
	isDash = isXexDash(xex);
// 	dbgPrintFake("loadPrep r3: %08x r4:'%s' r5: %08x hand: %08x typ: %08x ver: %08x r9: %08x r10: %08x st1: %08x\n", argR3, xexname, argR5, handle, typeinfo, ver, argR9, argR10, argSt1);
// 	dbgPrintFake("loadPrep isdash: %d\n", isDash);
	// loadPrep r3: 781e0900 r4:'\Device\Harddisk0\SystemExtPartition\32000100\Xna_TitleLauncher.xex' r5: 781e11a4 hand: 781e12a4 typ: 00000000 ver: 00000000 r9: 00000001 r10: 00000000 st1: 00000000
	if(getOpt(OPT_AUTOFAKE)) // starting indie, auto magically manage fakelive
	{
		if(isDash) // starting dashboard
			enableFakeLive(FALSE);
		else if(strstr(xex, "\\Xna_TitleLauncher.xex") != NULL) // starting indie
			enableFakeLive(TRUE);
		else // otherwise disable fakelive
			disableFakeLive();
	}
	if(strncmp(xex, XBOX_XEX, strlen(XBOX_XEX)) == 0) // checking for xbox1 title
	{
		if(!g_Protection)
		{
			HvxSetState(SYSCALL_KEY, SET_PROT_ON, 0, 0, 0);
			g_Protection = TRUE;
			doLightSync(&g_Protection);
		}
	}
	else
	{
		if(g_Protection)
		{
			HvxSetState(SYSCALL_KEY, SET_PROT_OFF, 0, 0, 0);
			g_Protection = FALSE;
			doLightSync(&g_Protection);
		}
		if(strncmp(xex, HELPER_XEX, strlen(HELPER_XEX)) != 0)
		{
			if(isDash)
			{
// 				dbgPrintFake("loadprep g_LaunchDashNoExcept: %d g_DashSystemSettings: %d g_FirstRun: %d\n", g_LaunchDashNoExcept, g_DashSystemSettings, g_FirstRun);
				if(g_LaunchDashNoExcept)
				{
					if(g_FirstRun)
					{
						//Sleep(1000); // sleep for one second to allow external hdd to spin up
						xexname = firstRunTasks(xex);
					}
					else
					{
						pad = getButtons(15, FALSE, XINPUT_GAMEPAD_A);
						if((pad & XINPUT_GAMEPAD_RIGHT_SHOULDER) == 0)
						{
							xexname = enumButtons(pad, xexname);
						}
					}
				}
				else if(g_DashSystemSettings) // XamLoaderGetPriorTitleId XamGetCurrentTitleId XamLoaderGetState
				{
					//dbgPrintFake("previous: %08x\n", XamLoaderGetPriorTitleId());
					pad = getButtons(15, FALSE, XINPUT_GAMEPAD_A);
					if((pad & XINPUT_GAMEPAD_RIGHT_SHOULDER) == 0)
						xexname = enumButtons(XINPUT_DUMMY_CONFIG, xexname);
				}
				g_DashSystemSettings = FALSE;
				doLightSync(&g_DashSystemSettings);
			}
			else if(isRegionHooked)
			{
				pad = getButtons(10, FALSE, 0);
				if((pad & XINPUT_GAMEPAD_RIGHT_SHOULDER))
				{
					g_overrideRegion = TRUE;
				}
			}
		}
	}

// 	dbgPrintFake("launching '%s'\n", xexname);
	if(xexname != xex)
		return loadPrepSave(argR3, xexname, argR5, handle, 0, 0, argR9, argR10, argSt1);
	else
		return loadPrepSave(argR3, xexname, argR5, handle, typeinfo, ver, argR9, argR10, argSt1);
}

static BOOL isLoadPrepHooked = FALSE;
static PDWORD loadPrepAddr;
static DWORD loadPrepOld[4];
#pragma optimize( "", off )
void loadPrepHook(PDWORD addr)
{
	if(!isLoadPrepHooked)
	{
		loadPrepAddr = addr;
		hookFunctionStart(loadPrepAddr, (PDWORD)loadPrepSaveVar, loadPrepOld, (DWORD)loadPrep);
		isLoadPrepHooked = TRUE;
	}
}
#pragma optimize( "", on ) 

void loadPrepUnhook(void)
{
	if(isLoadPrepHooked)
	{
		unhookFunctionStart(loadPrepAddr, loadPrepOld);
		isLoadPrepHooked = TRUE;
	}

}

static DWORD loadPrepRegionOrig;
void loadPrepRegionHook(void)
{
	if(!isRegionHooked)
	{
		loadPrepRegionOrig = hookExportOrd(MODULE_XAM, xamExp_XGetGameRegion, (DWORD)XGetGameRegionNew);
		isRegionHooked = TRUE;
	}
}

void loadPrepRegionUnhook(void)
{
	if(isRegionHooked)
	{
		unhookExportOrd(MODULE_XAM, xamExp_XGetGameRegion, loadPrepRegionOrig);
		isRegionHooked = FALSE;
	}
}
