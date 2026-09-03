#include "_hook_includes.h"
#include <stdio.h>

//hookFunctionStartOrd(MODULE_XAM, XAM_APP_LOAD_ORD, (PDWORD)XamAppLoadSaveVar, (DWORD)XamAppLoadHook);

extern BYTE tempdata[0x10];

#define XAM_APP_LOAD_ORD				0x244 // 280 XamAppLoad E_FAIL 0x80004005 "hud.xex"
#define XAM_SHOWMESSAGEBOX_ORD			0x2E9 // 745 XamShowMessageBox

typedef HRESULT (*XAMSHOWMESSAGEBOXFUN)(UINT64 r3, LPCWSTR wszTitle, LPCWSTR wszText, DWORD cButtons, LPCWSTR* pwszButtons, DWORD dwFocusButton, PFNMSGBOXRETURN resFun, DWORD dwFlags);
typedef HRESULT (*XAMAPPLOADFUN)(char const* appName, DWORD dwUserIndex, PVOID params, DWORD sz, PXOVERLAPPED xov, PVOID CSystemAppEntry);

XAMSHOWMESSAGEBOXFUN signinUiOrg;

static BOOL skipNextMessagebox = FALSE;
static XHUDOPENSTATE xhud = XHUDOPENSTATE_NONE;

PFNMSGBOXRETURN msgOrgCallback;

#ifdef DEBUG_XAMAPPLOAD_OUT
VOID msgRetHook(INT iButtonPressed, PXHUDOPENSTATE pHudRestoreState)
{
	DbgPrint("iButtonPressed: %08x PXHUDOPENSTATE: %08x\n", iButtonPressed, pHudRestoreState);
	msgOrgCallback(iButtonPressed, pHudRestoreState);
}
#endif

HRESULT signinUiHook(UINT64 r3, LPCWSTR wszTitle, LPCWSTR wszText, DWORD cButtons, LPCWSTR* pwszButtons, DWORD dwFocusButton, PFNMSGBOXRETURN resFun, DWORD dwFlags)
{
#ifdef DEBUG_XAMAPPLOAD_OUT
	//DbgPrint("hello from signinUiHook\n");
	DbgPrint("signinUiHook r3: %08x cbut: %x r9 %x flags %x skip %x\n", r3, cButtons, resFun, dwFlags, skipNextMessagebox);
#endif
	if(skipNextMessagebox == TRUE)
	{
		skipNextMessagebox = FALSE;
 #ifdef DEBUG_XAMAPPLOAD_OUT
		DbgPrint("I'd skip that yo!\n");
		msgOrgCallback = resFun;
		return signinUiOrg(r3,wszTitle,wszText,cButtons,pwszButtons,dwFocusButton,(PFNMSGBOXRETURN)&msgRetHook,dwFlags);
 #else
		if(cButtons == 0)
			resFun(1, &xhud);
		else
			resFun(0, &xhud);
		//if((XboxKrnlVersion->Build == 14717)||(XboxKrnlVersion->Build == 14719)||(XboxKrnlVersion->Build == 16537))
		//	resFun(1, &xhud);
		//else
		//	resFun(0, &xhud);
		return ERROR_SUCCESS;
#endif
	}
	//DbgPrint("ret: %x\n", ret);
	return signinUiOrg(r3,wszTitle,wszText,cButtons,pwszButtons,dwFocusButton,resFun,dwFlags);
}

void insertCurrentTemp(PWCHAR txt) // max is 0x17a wchar
{
	int i, j = 0;
	char ttem[0x40];
	double cpu, gpu, edram, mb;
	cpu = (tempdata[1] | (tempdata[2] << 8)) / 256.0;
	gpu = (tempdata[3] | (tempdata[4] << 8)) / 256.0;
	edram = (tempdata[5] | (tempdata[6] << 8)) / 256.0;
	mb = (tempdata[7] | (tempdata[8] << 8)) / 256.0;
	if(getOpt(OPT_SHOW_FAREN))
	{
		cpu = (cpu*1.8)+32;
		gpu = (gpu*1.8)+32;
		edram = (edram*1.8)+32;
		mb = (mb*1.8)+32;
		RtlSnprintf(ttem, 0x40, "\n\nCPU  : %3.1f°F\nGPU  : %3.1f°F\nEDRAM: %3.1f°F\nMOBO : %3.1f°F", cpu, gpu, edram, mb);
	}
	else
		RtlSnprintf(ttem, 0x40, "\n\nCPU  : %3.1f°C\nGPU  : %3.1f°C\nEDRAM: %3.1f°C\nMOBO : %3.1f°C", cpu, gpu, edram, mb);
	for(i = 0; i < 0x17a; i++)
	{
		if(j != 0)
		{
			if((ttem[j]&0xFF) == 0)
			{
				txt[i] = 0;
				i = 0x17a;
			}
			else
			{
				txt[i] = (WCHAR)(ttem[j]&0xFF);
				j++;
			}
		}
		else if(txt[i] == 0)
		{
			txt[i] = (WCHAR)(ttem[j]&0xFF);
			j++;
		}
	}

}

void __declspec(naked) XamAppLoadSaveVar(void)
{
	__asm{
		li r3, XAMAPPLOAD_VAL
		nop
		nop
		nop
		nop
		nop
		nop
		blr
	}
}
XAMAPPLOADFUN XamAppLoadSave = (XAMAPPLOADFUN)XamAppLoadSaveVar;
HRESULT XamAppLoadHook(char const* appName, DWORD dwUserIndex, PVOID params, DWORD sz, PXOVERLAPPED xov, PVOID CSystemAppEntry)
{
#ifdef DEBUG_XAMAPPLOAD_OUT
	if(appName != NULL)
		DbgPrint("app: %s user: %08x param: %08x sz: %08x xov: %x 3: %08x\n", appName, dwUserIndex, params, sz, (DWORD)xov, CSystemAppEntry);
	else
		DbgPrint("app: noname 1: %08x param: %08x sz: %08x xov: %x 3: %08x\n", dwUserIndex, params, sz, (DWORD)xov, CSystemAppEntry);
	if(params != NULL)
	{
		if(sz == 0x47c)
		{
			PMESSAGEBOX_PARAMS par = (PMESSAGEBOX_PARAMS) params;
			DbgPrint("messagebox eHudType    : %08x cButtons   : %08x dwFocusButton: %08x dwFlags: %08x\n", par->eHudType, par->cButtons, par->dwFocusButton, par->dwFlags);
			DbgPrint("           dwTrackingID: %08x dwUserIndex: %08x\n", par->dwTrackingID, par->dwUserIndex);
		}
		else if(sz == 0x14)
		{
			PXSHOWSIGNINUI_PARAMS par = (PXSHOWSIGNINUI_PARAMS)params;
			DbgPrint("signing track %08x uidx: %08x cpane: %08x flag: %08x\n", par->dwTrackingID, par->dwUserIndex, par->cPanes, par->dwFlags);
		}
	}
#endif
	if(appName != NULL)
	{
		if(strnicmp(appName, MODULE_HUD, strlen(MODULE_HUD)) == 0) // place to modify HUD calls
		{
			if(sz == sizeof(MESSAGEBOX_PARAMS) && (params != NULL))
			{
				if(((DWORD)params > 0x77000000) && ((DWORD)params < 0x80000000)) // xam stack...?
				{
					PMESSAGEBOX_PARAMS par = (PMESSAGEBOX_PARAMS) params;
					if((par->eHudType == 1) && (par->cButtons == 3) && (par->dwFocusButton == 2) && (par->dwFlags == 3))
					{
						if(getOpt(OPT_FORCE_SHUTDOWN))
							HalReturnToFirmware(HalPowerDownRoutine);
						else
						{
							if(getOpt(OPT_SHUTDOWN_SHOWTEMP))
							{
								insertCurrentTemp(par->szText);
							}
							if(getOpt(OPT_SELECT_SHUTDOWN) || getOpt(OPT_FORCE_SHUTDOWN))
							{
								par->dwFocusButton = 0;
							}
						}
					}
				}
			}
		}
		else if(strnicmp(appName, MODULE_SIGNIN, strlen(MODULE_SIGNIN)) == 0) // place to modify signin.xex calls
		{
			skipNextMessagebox = FALSE;

			if(params != NULL)
			{
				if(sz == sizeof(XSHOWSIGNINUI_PARAMS)) // signin message
				{
					if(getOpt(OPT_SIGN_NOTICE_DISABLE))
					{
#ifdef DEBUG_XAMAPPLOAD_OUT
						DbgPrint("skipping next mbox\n");
#endif
						skipNextMessagebox = TRUE;
					}
				}
			}
		}
	}
	return XamAppLoadSave(appName, dwUserIndex, params, sz, xov, CSystemAppEntry);
}

static BOOL isXamAppLoadUiHooked = FALSE;
void xamAppLoadHookUi(void)
{
	if(!isXamAppLoadUiHooked)
	{
		signinUiOrg = (XAMSHOWMESSAGEBOXFUN)hookExportOrd(MODULE_XAM, XAM_SHOWMESSAGEBOX_ORD, (DWORD)signinUiHook); // 745 XamShowMessageBox
		isXamAppLoadUiHooked = TRUE;
	}
}

void xamAppLoadUiUnhook(void)
{
	if(isXamAppLoadUiHooked)
	{
		unhookExportOrd(MODULE_XAM, XAM_SHOWMESSAGEBOX_ORD, (DWORD)signinUiOrg);
		isXamAppLoadUiHooked = FALSE;
	}
}

static BOOL isXamAppLoadHooked = FALSE;
static DWORD xamAppLoadOld[4];
void xamAppLoadHook(void)
{
	if(!isXamAppLoadHooked)
	{
		hookFunctionStartOrd(MODULE_XAM, XAM_APP_LOAD_ORD, (PDWORD)XamAppLoadSaveVar, xamAppLoadOld, (DWORD)XamAppLoadHook); // 580 XamAppLoad
		isXamAppLoadHooked = TRUE;
	}
}

void xamAppLoadUnhook(void)
{
	if(isXamAppLoadHooked)
	{
		unhookFunctionStartOrd(MODULE_XAM, XAM_APP_LOAD_ORD, xamAppLoadOld); // 580 XamAppLoad
		isXamAppLoadHooked = FALSE;
	}
}

/*
13604
	app: signin.xex user: 00000000 param: 0010125c sz: 00000014 xov: 0 3: 00000000
	signing track 00000000 uidx: 00000000 cpane: 00000001 flag: 04030000
	skipping next mbox
	signinUiHook r3: 000203d6 cbut: 2 r9 90107a68 flags 1 skip 1
	I'd skip that yo!
=======================================================
14719
	app: signin.xex user: 00000000 param: 0010119c sz: 00000014 xov: 0 3: 00000000
	signing track 00000000 uidx: 00000000 cpane: 00000001 flag: 24030000
	skipping next mbox
	signinUiHook r3: 00030523 cbut: 0 r9 90112258 flags 200101 skip 1
	I'd skip that yo!
=======================================================
16203
	app: signin.xex user: 00000000 param: 00102c3c sz: 00000014 xov: 0 3: 00000000
	signing track 00000000 uidx: 00000000 cpane: 00000001 flag: 24030000
	skipping next mbox
	signinUiHook r3: 000304aa cbut: 2 r9 901142c0 flags 101 skip 1
	I'd skip that yo!
=======================================================
16517
	app: signin.xex user: 00000000 param: 0010f5fc sz: 00000014 xov: 0 3: 00000000
	signing track 00000000 uidx: 00000000 cpane: 00000001 flag: 24030000
	skipping next mbox
	signinUiHook r3: 000202b9 cbut: 0 r9 90115238 flags 200101 skip 1
	I'd skip that yo!
*/
