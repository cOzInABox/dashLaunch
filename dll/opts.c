#include <xtl.h>
#include <stdio.h>
#include <string.h>
#include "xkelib.h"
#include "../_common.h"
#include "opts.h"
#include "optsHandlers.h"
#include "hooks/_all_hooks.h"
#include "iniparse.h"
#include "launch.h"

// #define DL_NO_OPTIONS 1

DL_OPTS_INFO dlopts[] = {
#ifndef DL_NO_OPTIONS
	// these don't need functions to modify the val, just the bit settings are enough
	{"pingpatch", DL_OPT_TYPE_BOOL, FALSE, OPT_PING_PATCH, optsHandlePingPatch, OPT_CAT_NETWORK},
	{"contpatch", DL_OPT_TYPE_BOOL, FALSE, OPT_CONT_PATCH, NULL, OPT_CAT_BEHAVIOR},
	{"xblapatch", DL_OPT_TYPE_BOOL, FALSE, OPT_XBLA_PATCH, NULL, OPT_CAT_BEHAVIOR},
	{"licpatch", DL_OPT_TYPE_BOOL, FALSE, OPT_LIC_PATCH, NULL, OPT_CAT_BEHAVIOR},

	{"nxemini", DL_OPT_TYPE_BOOL, TRUE, OPT_NXEMINI, NULL, OPT_CAT_BEHAVIOR},
	{"dvdexitdash", DL_OPT_TYPE_BOOL, FALSE, OPT_DVDEXIT, NULL, OPT_CAT_BEHAVIOR},
	{"xblaexitdash", DL_OPT_TYPE_BOOL, FALSE, OPT_XBLAEXIT, NULL, OPT_CAT_BEHAVIOR},
	{"nosysexit", DL_OPT_TYPE_BOOL, FALSE, OPT_NOSYSEXIT, NULL, OPT_CAT_BEHAVIOR},
	{"nohud", DL_OPT_TYPE_BOOL, FALSE, OPT_NONE, optsHandleNoHud, OPT_CAT_BEHAVIOR},
	{"nohealth", DL_OPT_TYPE_BOOL, TRUE, OPT_NONE, optsHandleKhealth, OPT_CAT_BEHAVIOR},
	{"nooobe", DL_OPT_TYPE_BOOL, TRUE, OPT_NONE, optsHandleOobe, OPT_CAT_BEHAVIOR},
	{"autoswap", DL_OPT_TYPE_BOOL, FALSE, OPT_NONE, optsHandleMultidisk, OPT_CAT_BEHAVIOR},
	{"regionspoof", DL_OPT_TYPE_BOOL, FALSE, OPT_NONE, optsHandleRegionSpoof, OPT_CAT_BEHAVIOR}, // depends on region
	{"region", DL_OPT_TYPE_WORDREGION, 0x7FFF, OPT_NONE, optsHandleRegion, OPT_CAT_BEHAVIOR}, // depends on regionspoof
	{"signnotice", DL_OPT_TYPE_BOOL, FALSE, OPT_SIGN_NOTICE_DISABLE, optsHandleSignNotice, OPT_CAT_NETWORK},
	{"liveblock", DL_OPT_TYPE_BOOL, TRUE, OPT_LIVE_BLOCK, optsHandleLiveBlock, OPT_CAT_NETWORK},
	{"livestrong", DL_OPT_TYPE_BOOL, TRUE, OPT_LIVE_STRONG, optsHandleLiveStrong, OPT_CAT_NETWORK},
	{"autoshut", DL_OPT_TYPE_BOOL, FALSE, OPT_SELECT_SHUTDOWN, optsHandleAutoShut, OPT_CAT_BEHAVIOR}, // can't have both autoshut and autooff
	{"autooff", DL_OPT_TYPE_BOOL, FALSE, OPT_FORCE_SHUTDOWN, optsHandleAutoOff, OPT_CAT_BEHAVIOR},
	{"shuttemps", DL_OPT_TYPE_BOOL, FALSE, OPT_SHUTDOWN_SHOWTEMP, optsHandleShutdownTemps, OPT_CAT_BEHAVIOR},
	{"xhttp", DL_OPT_TYPE_BOOL, TRUE, OPT_XHTTP_PATCHED, optsHandleXhttp, OPT_CAT_NETWORK},
	{"nonetstore", DL_OPT_TYPE_BOOL, TRUE, OPT_NONE, optsHandleHideNetStore, OPT_CAT_NETWORK},
	{"devprof", DL_OPT_TYPE_BOOL, FALSE, OPT_NONE, optsHandleDevProfiles, OPT_CAT_BEHAVIOR},
	{"devlink", DL_OPT_TYPE_BOOL, FALSE, OPT_NONE, optsHandleDevSyslink, OPT_CAT_NETWORK},

	// tasks - keep these with the task enabler last !
	{"hddtimer", DL_OPT_TYPE_DWORDTIME, 210, OPT_NONE, optsHandleHddAliveTimer, OPT_CAT_TIMERS},
	{"hddalive", DL_OPT_TYPE_BOOL, FALSE, OPT_NONE, optsHandleHddAlive, OPT_CAT_TIMERS},
	{"temptime", DL_OPT_TYPE_DWORDTIME, 10, OPT_NONE, optsHandleTempTime, OPT_CAT_TIMERS},
	{"tempport", DL_OPT_TYPE_WORDPORT, 7030, OPT_NONE, optsHandleTempPort, OPT_CAT_TIMERS},
	{"tempbcast", DL_OPT_TYPE_BOOL, FALSE, OPT_NONE, optsHandleTempBcast, OPT_CAT_TIMERS},

	{"fatalreboot", DL_OPT_TYPE_BOOL, FALSE, OPT_FATAL_REBOOT, optsHandleFatalReboot, OPT_CAT_BEHAVIOR}, // depends on fatalfreeze
	{"fatalfreeze", DL_OPT_TYPE_BOOL, FALSE, OPT_FATAL_FREEZE, optsHandleFatalFreeze, OPT_CAT_BEHAVIOR},
	{"safereboot", DL_OPT_TYPE_BOOL, TRUE, OPT_SAFE_REBOOT, optsHandleSafeReboot, OPT_CAT_BEHAVIOR},
	{"exchandler", DL_OPT_TYPE_BOOL, TRUE, OPT_NONE, optsHandleExchandler, OPT_CAT_BEHAVIOR},
	{"debugout", DL_OPT_TYPE_BOOL, TRUE, OPT_DEBUG_OUTPUT, NULL, OPT_CAT_BEHAVIOR}, /* FIXME*/
	{"sockpatch", DL_OPT_TYPE_BOOL, FALSE, OPT_SOCKPATCH, NULL, OPT_CAT_NETWORK},
	{"passlaunch", DL_OPT_TYPE_BOOL, FALSE, OPT_PASS_LAUNCHD, NULL, OPT_CAT_BEHAVIOR},
	{"fakelive", DL_OPT_TYPE_BOOL, FALSE, OPT_FAKELIVE, optsHandleFakeLive, OPT_CAT_NETWORK}, // fakelive and autofake cannot both be set
	{"autofake", DL_OPT_TYPE_BOOL, FALSE, OPT_AUTOFAKE, optsHandleAutoFake, OPT_CAT_NETWORK}, // fakelive and autofake cannot both be set
	{"autocont", DL_OPT_TYPE_BOOL, FALSE, OPT_AUTO_CONT_FAKE, optsHandleAutoCont, OPT_CAT_NETWORK}, // fakelive and autofake cannot both be set
	{"autofake0", DL_OPT_TYPE_DWORD, 0, OPT_NONE, NULL, OPT_CAT_NETWORK},
	{"autofake1", DL_OPT_TYPE_DWORD, 0, OPT_NONE, NULL, OPT_CAT_NETWORK},
	{"autofake2", DL_OPT_TYPE_DWORD, 0, OPT_NONE, NULL, OPT_CAT_NETWORK},
	{"autofake3", DL_OPT_TYPE_DWORD, 0, OPT_NONE, NULL, OPT_CAT_NETWORK},
	{"autofake4", DL_OPT_TYPE_DWORD, 0, OPT_NONE, NULL, OPT_CAT_NETWORK},
	{"autofake5", DL_OPT_TYPE_DWORD, 0, OPT_NONE, NULL, OPT_CAT_NETWORK},
	{"autofake6", DL_OPT_TYPE_DWORD, 0, OPT_NONE, NULL, OPT_CAT_NETWORK},
	{"autofake7", DL_OPT_TYPE_DWORD, 0, OPT_NONE, NULL, OPT_CAT_NETWORK},
	{"autofake8", DL_OPT_TYPE_DWORD, 0, OPT_NONE, NULL, OPT_CAT_NETWORK},
	{"autofake9", DL_OPT_TYPE_DWORD, 0, OPT_NONE, NULL, OPT_CAT_NETWORK},

	// can't be changed live / doesn't matter if you could change it
	{"noupdater", DL_OPT_TYPE_BOOL, TRUE, OPT_NONE, optsHandleNoUpdater, OPT_CAT_BEHAVIOR},
	//{"bootdelay", DL_OPT_TYPE_DWORD, AUT_BOOT_REP, OPT_NONE, NULL, OPT_CAT_BEHAVIOR},
	{"remotenxe", DL_OPT_TYPE_BOOL, FALSE, OPT_NONE, NULL, OPT_CAT_BEHAVIOR},
	//{"previewmd", DL_OPT_TYPE_BOOL, FALSE, OPT_NONE, optsHandlePreview, OPT_CAT_BEHAVIOR},
#endif

	// paths, the path struct is given to the app that wants to modify them
	{"dumpfile", DL_OPT_TYPE_PATH, DUMPFILE, OPT_NONE, NULL, OPT_CAT_PATHS},
	{"plugin1", DL_OPT_TYPE_PATHPLUGIN, PLUGIN1, OPT_NONE, NULL, OPT_CAT_PLUGINS},
	{"plugin2", DL_OPT_TYPE_PATHPLUGIN, PLUGIN2, OPT_NONE, NULL, OPT_CAT_PLUGINS},
	{"plugin3", DL_OPT_TYPE_PATHPLUGIN, PLUGIN3, OPT_NONE, NULL, OPT_CAT_PLUGINS},
	{"plugin4", DL_OPT_TYPE_PATHPLUGIN, PLUGIN4, OPT_NONE, NULL, OPT_CAT_PLUGINS},
	{"plugin5", DL_OPT_TYPE_PATHPLUGIN, PLUGIN5, OPT_NONE, NULL, OPT_CAT_PLUGINS},
	{"Default", DL_OPT_TYPE_PATHQLB, DEFAULT, OPT_NONE, NULL, OPT_CAT_PATHS},
	{"BUT_A", DL_OPT_TYPE_PATHQLB, BUT_A, OPT_NONE, NULL, OPT_CAT_PATHS},
	{"BUT_B", DL_OPT_TYPE_PATHQLB, BUT_B, OPT_NONE, NULL, OPT_CAT_PATHS},
	{"BUT_X", DL_OPT_TYPE_PATHQLB, BUT_X, OPT_NONE, NULL, OPT_CAT_PATHS},
	{"BUT_Y", DL_OPT_TYPE_PATHQLB, BUT_Y, OPT_NONE, NULL, OPT_CAT_PATHS},
	{"Start", DL_OPT_TYPE_PATHQLB, START, OPT_NONE, NULL, OPT_CAT_PATHS},
	{"Back", DL_OPT_TYPE_PATHQLB, BACK, OPT_NONE, NULL, OPT_CAT_PATHS},
	{"LBump", DL_OPT_TYPE_PATHQLB, LBUMP, OPT_NONE, NULL, OPT_CAT_PATHS},
	{"LThumb", DL_OPT_TYPE_PATHQLB, LTHUMB, OPT_NONE, NULL, OPT_CAT_PATHS},
	{"RThumb", DL_OPT_TYPE_PATHQLB, RTHUMB, OPT_NONE, NULL, OPT_CAT_PATHS},

	{"configapp", DL_OPT_TYPE_PATHQLB, CONFIGFILE, OPT_NONE, NULL, OPT_CAT_PATHS},
	{"Guide", DL_OPT_TYPE_PATHQLB, GUIDEPWR, OPT_NONE, NULL, OPT_CAT_PATHS},
	{"Power", DL_OPT_TYPE_PATHQLB, CONSPWR, OPT_NONE, NULL, OPT_CAT_PATHS},
	{"Fakeanim", DL_OPT_TYPE_PATHQLB, BOOTANIM, OPT_NONE, NULL, OPT_CAT_PATHS},

	{"ftpserv", DL_OPT_TYPE_BOOL, FALSE, OPT_NONE, NULL, OPT_CAT_EXTERNAL},
	{"ftpport", DL_OPT_TYPE_WORDPORT, 21, OPT_NONE, NULL, OPT_CAT_EXTERNAL},
	{"updserv", DL_OPT_TYPE_BOOL, FALSE, OPT_NONE, NULL, OPT_CAT_EXTERNAL},
	{"calaunch", DL_OPT_TYPE_BOOL, FALSE, OPT_NONE, NULL, OPT_CAT_EXTERNAL},
	{"fahrenheit", DL_OPT_TYPE_BOOL, FALSE, OPT_SHOW_FAREN, NULL, OPT_CAT_EXTERNAL},
};

#define OPTS_TOTALNUM			(sizeof(dlopts)/sizeof(DL_OPTS_INFO))

extern ldata ldat;
extern keydata launch[MAX_NUM_BUTTONS];
extern BOOL g_kernSupported;
static DWORD currOptions = 0;
static DWORD currOptVal[OPTS_TOTALNUM];


BOOL getOpt(DWORD omask)
{
	return ((currOptions&omask)!=0);
}

void setOpt(DWORD omask)
{
	currOptions = (currOptions|omask);
	ldat.options = currOptions;
}

void unSetOpt(DWORD omask)
{
	currOptions = (currOptions&~omask);
	ldat.options = currOptions;
}

extern int dlaunchGetNumOpts(int* totalOpts)
{
	if(g_kernSupported == TRUE)
	{
		if(totalOpts)
			*totalOpts = OPTS_TOTALNUM;
		return (OPTS_TOTALNUM);
	}
	if(totalOpts)
		*totalOpts = 0;
	return 0;
}

extern int dlaunchGetOptInfo(int opt, PDWORD optType, PCHAR outStr, PDWORD currVal, PDWORD defValue, PDWORD optCategory)
{
	if((g_kernSupported == TRUE) && (opt < OPTS_TOTALNUM))
	{
		if(optType)
			*optType = dlopts[opt].optType;
		if(outStr)
			strcpy(outStr, dlopts[opt].optName);
		if(currVal)
			*currVal = currOptVal[opt];
		if(defValue)
			*defValue = dlopts[opt].defVal;
		if(optCategory)
			*optCategory = dlopts[opt].optCategory;
		return 0;
	}
	return -1;
}

static keydata dummyKey = {
	"\\invalid\\path\\kernel\\not\\supported",
	INVALID_ITEM, // flags
	0, // dev
	0 // rootDev
};

extern BOOL dlaunchGetOptVal(int opt, PDWORD val)
{
	pkeydata* pkey;

	if(opt > OPTS_TOTALNUM)
		return FALSE;
	if(val == NULL)
		return FALSE;

	switch(dlopts[opt].optType)
	{
		case DL_OPT_TYPE_BOOL: // stuffs bool into the pointer
			*val = (currOptVal[opt]&1);
			break;
		case DL_OPT_TYPE_WORD: // stuffs word into the pointer
		case DL_OPT_TYPE_WORDPORT:
		case DL_OPT_TYPE_WORDREGION:
			*val = (currOptVal[opt]&0xFFFF);
			break;
		case DL_OPT_TYPE_DWORD: // stuff dword into the pointer
		case DL_OPT_TYPE_DWORDTIME:
			*val = currOptVal[opt];
			break;
		case DL_OPT_TYPE_PATH: // stuffs a pointer to keydata into the pointer
		case DL_OPT_TYPE_PATHQLB:
		case DL_OPT_TYPE_PATHPLUGIN:
			pkey = (pkeydata*)val;
			if(currOptVal[opt] != 0) // to protect morons from themselves, we must never return NULL here!
				*pkey = (pkeydata)(currOptVal[opt]);
			else
				*pkey = &dummyKey;
			break;
		default:
			return FALSE;
			break;
	}
	return TRUE;
}

extern BOOL dlaunchGetOptValByName(char* optName, PDWORD val)
{
	int i;
	if((optName == NULL)||(val == NULL))
		return FALSE;
	if(optName[0] == 0)
		return FALSE;

	for(i = 0; i < OPTS_TOTALNUM; i++)
	{
		if(stricmp(dlopts[i].optName, optName) == 0)
		{
			return dlaunchGetOptVal(i, val);
		}
	}
	return FALSE;
}

extern BOOL dlaunchSetOptVal(int opt, PDWORD val)
{
	if(opt > OPTS_TOTALNUM)
		return FALSE;
	if(val == NULL)
		return FALSE;

	if(dlopts[opt].optHandler != NULL)
		dlopts[opt].optHandler(val);
	switch(dlopts[opt].optType)
	{
		case DL_OPT_TYPE_BOOL: // stuffs bool into the pointer
			if((dlopts[opt].optHandler != NULL)&&(dlopts[opt].optMask != OPT_NONE)) // opt mask would have already been handled
				currOptVal[opt] = getOpt(dlopts[opt].optMask);
			else
			{
				currOptVal[opt] = (*val)&1;
				if(dlopts[opt].optMask != OPT_NONE)
				{
					if(currOptVal[opt] == TRUE)
						setOpt(dlopts[opt].optMask);
					else
						unSetOpt(dlopts[opt].optMask);
				}
			}
			break;
		case DL_OPT_TYPE_WORD: // stuffs word into the pointer
		case DL_OPT_TYPE_WORDPORT:
		case DL_OPT_TYPE_WORDREGION:
			currOptVal[opt] = (*val)&0xFFFF;
			break;
		case DL_OPT_TYPE_DWORD: // stuff dword into the pointer
		case DL_OPT_TYPE_DWORDTIME:
			currOptVal[opt] = (*val);
			break;
		//case DL_OPT_TYPE_PATH: // stuffs a pointer to keydata into the pointer
		//	pkey = (pkeydata*)val;
		//	*pkey = (pkeydata)(currOptVal[opt]);
		//	break;
		default:
			return FALSE;
			break;
	}
	return TRUE;
}

extern BOOL dlaunchSetOptValByName(char* optName, PDWORD val)
{
	int i;
	if((optName == NULL)||(val == NULL))
		return FALSE;
	if(optName[0] == 0)
		return FALSE;

	for(i = 0; i < OPTS_TOTALNUM; i++)
	{
		if(stricmp(dlopts[i].optName, optName) == 0)
		{
			return dlaunchSetOptVal(i, val);
		}
	}
	return FALSE;
}

void initOpts(void)
{
	DWORD dwval;
	int i;
	for(i = 0; i < OPTS_TOTALNUM; i++)
	{
		switch(dlopts[i].optType)
		{
			case DL_OPT_TYPE_BOOL:
				//dbgPrintFake("initOptBool %d '%s' val: %x\n", i, dlopts[i].optName, dlopts[i].defVal);
				dwval = (dlopts[i].defVal&1);
				dlaunchSetOptVal(i, &dwval);
				break;
			case DL_OPT_TYPE_WORD:
			case DL_OPT_TYPE_WORDPORT:
			case DL_OPT_TYPE_WORDREGION:
				//dbgPrintFake("initOptWord %d '%s' val: %x\n", i, dlopts[i].optName, dlopts[i].defVal);
				dwval = (dlopts[i].defVal&0xFFFF);
				dlaunchSetOptVal(i, &dwval);
				break;
			case DL_OPT_TYPE_DWORD:
			case DL_OPT_TYPE_DWORDTIME:
				//dbgPrintFake("initOptDword %d '%s' val: %x\n", i, dlopts[i].optName, dlopts[i].defVal);
				dwval = dlopts[i].defVal;
				dlaunchSetOptVal(i, &dwval);
				break;
			case DL_OPT_TYPE_PATH:
			case DL_OPT_TYPE_PATHQLB:
			case DL_OPT_TYPE_PATHPLUGIN:
				//dbgPrintFake("initOptPath %d '%s' val: %x\n", i, dlopts[i].optName, dlopts[i].defVal);
				currOptVal[i] = (DWORD)&launch[dlopts[i].defVal]; // sets the val to a pointer to the keydata
				memset((PVOID)currOptVal[i], 0, sizeof(keydata));
				launch[dlopts[i].defVal].dev = INVALID_ITEM;
				launch[dlopts[i].defVal].flags = INVALID_ITEM;
				launch[dlopts[i].defVal].rootDev = INVALID_ITEM;
				break;
			default:
				break;
		}
	}
}

void setOptsFromIni(void)
{
	DWORD dwval;
	int i;
	for(i = 0; i < OPTS_TOTALNUM; i++) // the unchangeable options must be handled separately !!
	{
		switch(dlopts[i].optType)
		{
			case DL_OPT_TYPE_BOOL:
				dwval = iniGetBool(dlopts[i].optName, dlopts[i].defVal);
				dlaunchSetOptVal(i, &dwval);
				break;
			case DL_OPT_TYPE_WORD:
			case DL_OPT_TYPE_WORDPORT:
			case DL_OPT_TYPE_WORDREGION:
				dwval = iniGetDword(dlopts[i].optName, dlopts[i].defVal) & 0xFFFF;
				dlaunchSetOptVal(i, &dwval);
				break;
			case DL_OPT_TYPE_DWORD:
			case DL_OPT_TYPE_DWORDTIME:
				dwval = iniGetDword(dlopts[i].optName, dlopts[i].defVal);
				dlaunchSetOptVal(i, &dwval);
				break;
			case DL_OPT_TYPE_PATH:
			case DL_OPT_TYPE_PATHQLB:
			case DL_OPT_TYPE_PATHPLUGIN:
					setupPkeyPath(iniGetString(dlopts[i].optName), &launch[dlopts[i].defVal], dlopts[i].optType);
					//dbgPrintFake("result: flag %x dev %d path '%s'\n", launch[dlopts[i].defVal].flags, launch[dlopts[i].defVal].dev, launch[dlopts[i].defVal].launchpath);
				break;
			default:
				break;
		}

	}
}

// set up to save and restore settings for autofake
static BOOL autoLiveBlock = FALSE;
static BOOL autoLiveFake = FALSE;
static BOOL autoContFake = FALSE;
void enableFakeLive(BOOL isXna)
{
	if(!autoLiveFake)
	{
// 		dbgPrintFake("enabling fakelive\n");
		signinStateHook();
		autoLiveBlock = getOpt(OPT_LIVE_BLOCK);
		if(!autoLiveBlock)
		{
			DWORD bTrue = TRUE;
			dlaunchSetOptValByName("liveblock", &bTrue);
		}
		autoLiveFake = TRUE;
		if((isXna)&&(autoContFake == FALSE)) // for autocont
		{
			if(getOpt(OPT_AUTO_CONT_FAKE) == TRUE)
			{
				if(getOpt(OPT_CONT_PATCH) == FALSE) // contpatch is not already enabled
				{
					setOpt(OPT_CONT_PATCH);
					autoContFake = TRUE;
				}
			}
		}
	}
}

void disableFakeLive(void)
{
	if(autoLiveFake)
	{
// 		dbgPrintFake("disabling fakelive\n");
		signinStateUnhook();
		if(!autoLiveBlock)
		{
			DWORD bFalse = FALSE;
			dlaunchSetOptValByName("liveblock", &bFalse);
		}
		autoLiveFake = FALSE;
		if(autoContFake)
		{
			unSetOpt(OPT_CONT_PATCH);
			autoContFake = FALSE;
		}
	}
}

static char optName[] = "autofake0";
BOOL procTitleFakeLive(DWORD tid)
{
	if(tid != 0)
	{
		int i;
// 		DbgPrint("autofake processing tid 0x%x\n", tid);
		for(i = 0; i < 10; i++)
		{
			DWORD itid;
			optName[8] = (i+0x30)&0xFF;
			if(dlaunchGetOptValByName(optName, &itid))
			{
// 				DbgPrint("%d:0x%x\n", i, itid);
				if(tid == itid)
				{
// 					DbgPrint("match! fakelive via auto enabled!\n");
					enableFakeLive(FALSE);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}
