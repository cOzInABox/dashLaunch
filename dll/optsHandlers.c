#include <xtl.h>
#include <stdio.h>
#include <string.h>
#include "xkelib.h"
#include "../_common.h"
#include "opts.h"
#include "launch.h"
#include "utility.h"
#include "tasks/_all_tasks.h"
#include "hooks/_all_hooks.h"

extern ldata ldat;
extern keydata launch[MAX_NUM_BUTTONS];
extern PATCH_ADDRS pats[KVERS_SUPPORTED];
extern KERN_PATCH_ADDR kernpat[];
extern int g_usePatch;

void optsHandleAutoShut(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			DWORD dwTemp = FALSE;
			dlaunchSetOptValByName("autooff", &dwTemp);
			setOpt(OPT_SELECT_SHUTDOWN);
		}
		else
		{
			unSetOpt(OPT_SELECT_SHUTDOWN);
		}
	}
}

void optsHandleAutoOff(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			DWORD dwTemp = FALSE;
			dlaunchSetOptValByName("autoshut", &dwTemp);
			dlaunchSetOptValByName("shuttemps", &dwTemp);
			setOpt(OPT_FORCE_SHUTDOWN);
		}
		else
		{
			unSetOpt(OPT_FORCE_SHUTDOWN);
		}
	}
}

void optsHandlePingPatch(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			genericPingPatch((PDWORD)pats[g_usePatch].PingLimit);
			setOpt(OPT_PING_PATCH);
		}
		else
		{
			unSetOpt(OPT_PING_PATCH);
			genericPingUnpatch();
		}
	}
}

// when set to false freezes attempt to not be fatal
void optsHandleFatalFreeze(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			DWORD dwTemp = FALSE;
			dlaunchSetOptValByName("fatalreboot", &dwTemp);
			setOpt(OPT_FATAL_FREEZE);
			exceptBugcheckUnhook();
		}
		else
		{
			unSetOpt(OPT_FATAL_FREEZE);
			exceptBugcheckHook();
		}
	}
}

// this can't be true if fatalfreeze is true
void optsHandleFatalReboot(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			DWORD dwTemp = FALSE;
			dlaunchSetOptValByName("fatalfreeze", &dwTemp);
			setOpt(OPT_FATAL_REBOOT);
		}
		else
		{
			unSetOpt(OPT_FATAL_REBOOT);
		}
	}
}

void optsHandleSafeReboot(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			setOpt(OPT_SAFE_REBOOT);
			HalRebootUnhook();
		}
		else
		{
			HalRebootHook();
			unSetOpt(OPT_SAFE_REBOOT);
		}
	}
}

void optsHandleRegionSpoof(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			loadPrepRegionHook();
		}
		else
		{
			loadPrepRegionUnhook();
		}
	}
}

void optsHandleRegion(PDWORD val)
{
	if(val != NULL)
	{
		WORD wval = (*val)&0xFFFF;
		loadPrepSetRegion(wval);
	}
}

void optsHandleNoHud(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			genericShowHudPatch((PDWORD)(pats[g_usePatch].HudDisable));
		}
		else
		{
			genericShowHudUnpatch();
		}
	}
}

void optsHandleNoUpdater(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			patchXamString("$SystemUpdate", "$$", FALSE);
			genericNoNewUpdatePatch();
		}
		else
		{
			patchXamString("$$ystemUpdate","$S", FALSE);
			genericNoNewUpdateUnpatch();
		}
	}
}

void optsHandleExchandler(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			exceptTrapHook((PDWORD)kernpat[pats[g_usePatch].kpatch].DebugTrap); // KdpTrap
		}
		else
		{
			exceptTrapUnhook();
		}
	}
}

void optsHandleLiveBlock(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			patchXamString("XBOX360SVC", "CVS063XOBX", FALSE);
			setOpt(OPT_LIVE_BLOCK);
		}
		else
		{
			if(getOpt(OPT_FAKELIVE))
			{
				DWORD bFalse = 0;
				dlaunchSetOptValByName("fakelive", &bFalse);
			}
			unSetOpt(OPT_LIVE_BLOCK);
			patchXamString("CVS063XOBX", "XBOX360SVC", FALSE);
		}
	}
}

void optsHandleLiveStrong(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			XNetDnsSetRules(TRUE);
			setOpt(OPT_LIVE_STRONG);
		}
		else
		{
			unSetOpt(OPT_LIVE_STRONG);
			XNetDnsSetRules(FALSE);
		}
	}
}

void optsHandleSignNotice(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			xamAppLoadHookUi();
			setOpt(OPT_SIGN_NOTICE_DISABLE);
		}
		else
		{
			unSetOpt(OPT_SIGN_NOTICE_DISABLE);
			xamAppLoadUiUnhook();
		}
	}
}

void optsHandleXhttp(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			if(pats[g_usePatch].pxhttpPat != NULL)
			{
				genericXhttpPatch(pats[g_usePatch].pxhttpPat);
				setOpt(OPT_XHTTP_PATCHED);
			}
		}
		else
		{
			if(pats[g_usePatch].pxhttpPat != NULL)
			{
				genericXhttpUnpatch();
				unSetOpt(OPT_XHTTP_PATCHED);
			}
		}
	}
}

void optsHandleFakeLive(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			DWORD bFalse = FALSE;
			signinStateHook();
			if(!getOpt(OPT_LIVE_BLOCK))
			{
				DWORD bTrue = TRUE;
				dlaunchSetOptValByName("liveblock", &bTrue);
			}
			dlaunchSetOptValByName("autofake", &bFalse);
			setOpt(OPT_FAKELIVE);
		}
		else
		{
			unSetOpt(OPT_FAKELIVE);
			signinStateUnhook();
		}
	}
}

void optsHandleAutoFake(PDWORD val)
{
	if(val != NULL)
	{
		DWORD bFalse = FALSE;
		if((*val)&1)
		{
			dlaunchSetOptValByName("fakelive", &bFalse);
			setOpt(OPT_AUTOFAKE);
		}
		else
		{
			dlaunchSetOptValByName("autocont", &bFalse);
			unSetOpt(OPT_AUTOFAKE);
		}
	}
}

void optsHandleAutoCont(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			DWORD bTrue = TRUE;
			dlaunchSetOptValByName("autofake", &bTrue);
			setOpt(OPT_AUTO_CONT_FAKE);
		}
		else
		{
			unSetOpt(OPT_AUTO_CONT_FAKE);
		}
	}
}

//{"hddalive", DL_OPT_TYPE_BOOL, FALSE, OPT_HDALIVE, NULL},
//{"hddtimer", DL_OPT_TYPE_WORD, 210, OPT_NONE, NULL},

void optsHandleHddAlive(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			DWORD tim;
			dlaunchGetOptValByName("hddtimer", &tim);
			scheduleHdAliveTask(tim);
		}
		else
		{
			endHdAliveTask();
		}
	}
}

void optsHandleHddAliveTimer(PDWORD val)
{
	if(val != NULL)
	{
		modifyHdAliveTimer(*val);
		//dlaunchSetOptValByName("hddtimer", &dwval);
	}
}

void optsHandleTempBcast(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			modifyTempBroadcast(TRUE);
		}
		else
		{
			modifyTempBroadcast(FALSE);
		}
	}
}

void optsHandleTempTime(PDWORD val)
{
	if(val != NULL)
	{
		modifyTempTimer(*val);
	}
}

void optsHandleTempPort(PDWORD val)
{
	if(val != NULL)
	{
		modifyTempPort(*val);
	}
}

void optsHandleHideNetStore(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			genericNetworkStorageHidePatch();
		}
		else
		{
			genericNetworkStorageHideUnpatch();
		}
	}
}

void optsHandleShutdownTemps(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			if(getOpt(OPT_FORCE_SHUTDOWN))
			{
				DWORD bFalse = 0;
				dlaunchSetOptValByName("autooff", &bFalse);
			}
			setOpt(OPT_SHUTDOWN_SHOWTEMP);
		}
		else
		{
			unSetOpt(OPT_SHUTDOWN_SHOWTEMP);
		}
	}

}

void optsHandleDevProfiles(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			xamObfuscateHook();
		}
		else
		{
			xamObfuscateUnhook();
		}
	}
}

void optsHandleDevSyslink(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			xamObfuscateSyslinkHook();
		}
		else
		{
			xamObfuscateSyslinkUnhook();
		}
	}
}

void optsHandleMultidisk(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			multidiskHook();
		}
		else
		{
			multidiskUnhook();
		}
	}
}

void optsHandlePreview(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			XamSetStagingMode(STAGING_MODE_STAGING);
		}
		else
		{
			XamSetStagingMode(STAGING_MODE_PRODUCTION);
		}
	}
}

void optsHandleOobe(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			XamOobeHook();
		}
		else
		{
			XamOobeUnhook();
		}
	}
}

void optsHandleKhealth(PDWORD val)
{
	if(val != NULL)
	{
		if((*val)&1)
		{
			genericKHealthPatch();
		}
		else
		{
			genericKHealthUnpatch();
		}
	}
}


/*
void optsHandle(PDWORD val)
{
	if(val != NULL)
	{
		BOOL bval = (*val)&1;
		WORD wval = (*val)&0xFFFF;
		DWORD dwval = (*val);

		if(bval == TRUE)
		{
			XNetDnsSetRules(TRUE);
			setOpt(OPT_LIVE_STRONG);
		}
		else
		{
			unSetOpt(OPT_LIVE_STRONG);
			XNetDnsSetRules(FALSE);
		}
	}
}
*/
