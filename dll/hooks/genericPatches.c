#include "_hook_includes.h"


/* ************* PING ************* */
static BOOL isPingPatched = FALSE;
static PDWORD pingPatchAdr = NULL;
static DWORD pingPatchSave = 0;
void genericPingPatch(PDWORD adr)
{
	if(!isPingPatched)
	{
		pingPatchAdr = adr;
		if(pingPatchAdr != NULL)
		{
			// patch by FBDev
			pingPatchSave = patchInNop(pingPatchAdr);
			isPingPatched = TRUE;
		}
	}
}

void genericPingUnpatch(void)
{
	if(isPingPatched)
	{
		if(pingPatchAdr != NULL)
		{
			pingPatchAdr[0] = pingPatchSave;
			isPingPatched = FALSE;
		}
	}
}

/* ************* HUD show ************* */
static BOOL isShowHudPatched = FALSE;
static PDWORD showHudAdr = NULL;
static DWORD showHudSave[2];

void genericShowHudPatch(PDWORD adr)
{
	if(!isShowHudPatched)
	{
		showHudAdr = adr;
		if(showHudAdr != NULL)
		{
			showHudSave[0] = showHudAdr[0];
			showHudSave[1] = showHudAdr[1];
			showHudAdr[0] = LI_R3_0;
			showHudAdr[1] = ASM_NOP;
			doSync(showHudAdr);
			isShowHudPatched = TRUE;
		}
	}
}

void genericShowHudUnpatch(void)
{
	if(isShowHudPatched)
	{
		if(showHudAdr != NULL)
		{
			showHudAdr[0] = showHudSave[0];
			showHudAdr[1] = showHudSave[1];
			doSync(showHudAdr);
			isShowHudPatched = FALSE;
		}
	}
}

/* ************* revoke check ************* */
static BOOL isRevokeCheckPatched = FALSE;
static PDWORD revokeCheckAdr = NULL;
static DWORD revokeCheckSave;

#pragma optimize( "", off )
void genericRevokeCheckPatch(PDWORD adr)
{
	if(!isRevokeCheckPatched)
	{
		revokeCheckAdr = adr;
		if(revokeCheckAdr != NULL)
		{
			revokeCheckSave = patchInNop(revokeCheckAdr);
			isRevokeCheckPatched = TRUE;
		}
	}
}
#pragma optimize( "", on ) 

void genericRevokeCheckUnpatch(void)
{
	if(isRevokeCheckPatched)
	{
		if(revokeCheckAdr != NULL)
		{
			patchInDword(revokeCheckAdr, revokeCheckSave);
			isRevokeCheckPatched = FALSE;
		}
	}
}

/* ************* xhttp patch ************* */
static BOOL isXhttpPatched = FALSE;
static PXHTTP_PATCH_ADDR xhttpAdr = NULL;
static DWORD xhttpSave[6];

void genericXhttpPatch(PXHTTP_PATCH_ADDR pats)
{
	if(!isXhttpPatched)
	{
		xhttpAdr = pats;
		if(xhttpAdr != NULL)
		{
			PDWORD dest;
			DWORD dwTemp;
			dest = (PDWORD)xhttpAdr->XampXAuthStartup;
			dwTemp = makeBranch((xhttpAdr->XampXAuthStartup+4), xhttpAdr->XampXAuthStartupDest, FALSE);
			xhttpSave[0] = patchInDword(dest, 0x3BA00000);  // li %r29, 0
			xhttpSave[1] = patchInDword(&dest[1], dwTemp);
			xhttpSave[2] = patchInDword(xhttpAdr->XamWaitForNSAL, LI_R3_1);
			xhttpSave[3] = patchInDword(xhttpAdr->XamRequestToken, LI_R3_1);
			xhttpSave[4] = patchInDword(xhttpAdr->LookupAppliesTo, LI_R3_1);
			xhttpSave[5] = patchInDword(xhttpAdr->XAuthValidateURL, LI_R3_1);
			isXhttpPatched = TRUE;
		}
	}
}

void genericXhttpUnpatch(void)
{
	if(isXhttpPatched)
	{
		if(xhttpAdr != NULL)
		{
			PDWORD dest;
			dest = (PDWORD)xhttpAdr->XampXAuthStartup;
			patchInDword(dest, xhttpSave[0]);
			patchInDword(&dest[1], xhttpSave[1]);
			patchInDword(xhttpAdr->XamWaitForNSAL, xhttpSave[2]);
			patchInDword(xhttpAdr->XamRequestToken, xhttpSave[3]);
			patchInDword(xhttpAdr->LookupAppliesTo, xhttpSave[4]);
			patchInDword(xhttpAdr->XAuthValidateURL, xhttpSave[5]);
			isXhttpPatched = FALSE;
		}
	}
}

/* ************* network storage hide ************* */
static BOOL isNetworkStorageHidePatched = FALSE;
static PDWORD networkStorageHideAdr = NULL;
static DWORD networkStorageHideSave[2];

void genericNetworkStorageHidePatch(void)
{
	if(!isNetworkStorageHidePatched)
	{
		networkStorageHideAdr = (PDWORD)resolveFunct(MODULE_XAM, 1590); // XamNetworkStorageShouldHideFromTitle
		if(networkStorageHideAdr != NULL)
		{
			networkStorageHideSave[0] = patchInDword(&networkStorageHideAdr[0], LI_R3_1);
			networkStorageHideSave[1] = patchInDword(&networkStorageHideAdr[1], ASM_BLR);
			isNetworkStorageHidePatched = TRUE;
		}
	}
}

void genericNetworkStorageHideUnpatch(void)
{
	if(isNetworkStorageHidePatched)
	{
		if(networkStorageHideAdr != NULL)
		{
			patchInDword(&networkStorageHideAdr[0], networkStorageHideSave[0]);
			patchInDword(&networkStorageHideAdr[1], networkStorageHideSave[1]);
			isNetworkStorageHidePatched = FALSE;
		}
	}
}

/* ************* xam xcontent NXE install patch ************* */
PDWORD xamNxeInstAddr[2];
DWORD xamNxeInstSave[2];
BOOL isXamNxeInsPatched = FALSE;

void genericNxeInstallPatch(PDWORD XamContLivePirs, PDWORD XamContDevice)
{
	if(!isXamNxeInsPatched)
	{
		if((XamContLivePirs != NULL)&&(XamContDevice != NULL))
		{
			xamNxeInstAddr[0] = XamContLivePirs;
			xamNxeInstAddr[1] = XamContDevice;
			xamNxeInstSave[0] = *XamContLivePirs;
			xamNxeInstSave[1] = *XamContDevice;
			*XamContLivePirs = ASM_NOP;
			*XamContDevice = ASM_NOP;
			doSync(XamContLivePirs);
			isXamNxeInsPatched = TRUE;
		}
	}
}

void genericNxeInstallUnpatch(void)
{
	if(isXamNxeInsPatched)
	{
		*xamNxeInstAddr[0] = xamNxeInstSave[0];
		*xamNxeInstAddr[1] = xamNxeInstSave[1];
		isXamNxeInsPatched = FALSE;
	}
}

/* ************* patch kinect health videos out ************* */
PDWORD xamKHealthAddr = NULL;
DWORD xamKHealthSaveVal;
BOOL isXamKHealthPatched = FALSE;

void genericKHealthPatch(void)
{
	if(!isXamKHealthPatched)
	{
		if((xamKHealthAddr != NULL)&&(xamKHealthAddr != NULL))
		{
			xamKHealthSaveVal = *xamKHealthAddr;
			*xamKHealthAddr = ASM_NOP;
			isXamKHealthPatched = TRUE;
		}
	}
}

void genericKHealthUnpatch(void)
{
	if(isXamKHealthPatched)
	{
		*xamKHealthAddr = xamKHealthSaveVal;
		isXamKHealthPatched = FALSE;
	}
}

void genericKHealthSetAddr(PDWORD addr)
{
	xamKHealthAddr=addr;
}

/* ************* patch update process to not allow NEWER updates to install ************* */
PDWORD noNewUpdateAddr = NULL;
DWORD noNewUpdateSaveVal;
BOOL isNoNewUpdatePatched = FALSE;
void genericNoNewUpdateSetAddr(PDWORD addr)
{
	noNewUpdateAddr = addr;
}

void genericNoNewUpdatePatch(void)
{
	if((!isNoNewUpdatePatched)&&(noNewUpdateAddr != NULL))
	{
		noNewUpdateSaveVal = *noNewUpdateAddr;
		*noNewUpdateAddr = ASM_NOP;
		isNoNewUpdatePatched = TRUE;
	}
}

void genericNoNewUpdateUnpatch(void)
{
	if((isNoNewUpdatePatched)&&(noNewUpdateAddr != NULL))
	{
		*noNewUpdateAddr = noNewUpdateSaveVal;
		isNoNewUpdatePatched = FALSE;
	}
}


