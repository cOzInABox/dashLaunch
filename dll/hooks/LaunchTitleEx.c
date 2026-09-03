#include "_hook_includes.h"
//#include "LaunchTitleEx.h"

typedef DWORD (*LAUNCHTITLEEXSAVEFUN)(const char * arg1, const char * arg2, const char * arg3, DWORD flags);

void __declspec(naked) LaunchTitleExSaveVar(void)
{
	__asm{
		li r3, LAUNCHTITLEEX_VAL
		nop
		nop
		nop
		nop
		nop
		nop
		blr
	}
}
LAUNCHTITLEEXSAVEFUN LaunchTitleExSave = (LAUNCHTITLEEXSAVEFUN)LaunchTitleExSaveVar;
DWORD LaunchTitleEx(const char * szLaunchPath, const char * szMountPath, const char * szCmdLine, DWORD flags)
{
	checkLaunchData(szLaunchPath, szMountPath, szCmdLine, flags);
// 	dbgPrintFake("LaunchTitleEx: %08x %08x %08x %08x\n", arg1, arg2, arg3, flags);
	return LaunchTitleExSave(szLaunchPath, szMountPath, szCmdLine, flags);
}

DWORD LaunchTitleExSaveCall(const char * arg1, const char * arg2, const char * arg3, DWORD flags)
{
	return LaunchTitleExSave(arg1, arg2, arg3, flags);
}

static BOOL isTitleExHooked = FALSE;
static DWORD launchTitleExOld[4];
void launchTitleExHook(void)
{
	if(!isTitleExHooked)
	{
		hookFunctionStartOrd(MODULE_XAM, xamExp_XamLoaderLaunchTitleEx, (PDWORD)LaunchTitleExSaveVar, launchTitleExOld, (DWORD)LaunchTitleEx);
		isTitleExHooked = TRUE;
	}
}

void launchTitleExUnhook(void)
{
	if(isTitleExHooked)
	{
		unhookFunctionStartOrd(MODULE_XAM, xamExp_XamLoaderLaunchTitleEx, launchTitleExOld);
		isTitleExHooked = FALSE;
	}
}
