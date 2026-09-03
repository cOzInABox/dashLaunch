#include "_hook_includes.h"

typedef NTSTATUS (*XEXPVERIFYXEXHEADERSFUN)(PIMAGE_XEX_HEADER XexHeader, BOOL TransferHeader, PIMAGE_XEX_HEADER* TransferedHeader); // XexpVerifyXexHeaders
typedef NTSTATUS (*XEXPLOADIMAGEFUN)(LPCSTR xexName, DWORD typeInfo, DWORD ver, PHANDLE modHandle); // XexpLoadImage

static const BYTE xexRetail[] = {0x20, 0xB1, 0x85, 0xA5, 0x9D, 0x28, 0xFD, 0xC3, 0x40, 0x58, 0x3F, 0xBB, 0x08, 0x96, 0xBF, 0x91};
static const BYTE xexDev[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static BOOL switchKeys = FALSE;
extern BOOL g_SkipLaunchNotify;

#define XEXLOAD_SIGNIN	"signin.xex"
#define XEXLOAD_CREATE	"createprofile.xex"
#define XEXLOAD_HUD		"hud.xex"

// 54 6B 06 3F ?? ?? ?? ?? 3B BE 01 08 7F A3 EB 78 ?? ?? ?? ?? 54 6B 06 3F
//                                                 <li %r3, 1>
#define DASHNXE_POFF		4
static DWORD dashNxePatchInstall = LI_R3_1;
static DWORD dashPatchNxeInstall[] = {0x546B063F, 0xFFFFFFFF, 0x3BBE0108, 0x7FA3EB78, 0xFFFFFFFF, 0x546B063F};
#define DASHNXEINST_SZ		(sizeof(dashPatchNxeInstall)/4)


#define XEXVERIFY_MAX_SEARCH		12
#define XEXLOADIMAGE_MAX_SEARCH		9
#define XEVERIFYIMAGEHEADERS_ORD	420
#define XEXLOADIMAGE_ORD			409


// using XexVerifyImageHeaders to find XexpVerifyXexHeaders
void __declspec(naked) XexpVerifyXexHeadersSaveVar(void)
{
	__asm{
		li r3, XEXVERIFYHEAD_VAL
		nop
		nop
		nop
		nop
		nop
		nop
		blr
	}
}
XEXPVERIFYXEXHEADERSFUN XexpVerifyXexHeadersSave = (XEXPVERIFYXEXHEADERSFUN)XexpVerifyXexHeadersSaveVar;
NTSTATUS XexpVerifyXexHeadersHook(PIMAGE_XEX_HEADER XexHeader, BOOL TransferHeader, PIMAGE_XEX_HEADER *TransferedHeader)
{
	NTSTATUS ret;
	if(switchKeys)
	{
		XECRYPT_AES_STATE aesc;
		PXEX_SECURITY_INFO sec = (PXEX_SECURITY_INFO)((DWORD)XexHeader->SecurityInfo+(DWORD)XexHeader);
		//DWORD i;
		//PDWORD dat = (PDWORD)sec->ImageInfo.ImageKey;
		//for(i = 0; i < 4; i++)
		//	dbgPrintFake("%04x", dat[i]);

		//dbgPrintFake("\ntrying setting devkit key hdr: %x sec: %x\n", XexHeader, sec);
		// decrypt with xexRetail
		XeCryptAesKey(&aesc, (PBYTE)xexRetail);
		XeCryptAesEcb(&aesc, sec->ImageInfo.ImageKey, sec->ImageInfo.ImageKey, FALSE);
		// encrypt with xexDev
		XeCryptAesKey(&aesc, (PBYTE)xexDev);
		XeCryptAesEcb(&aesc, sec->ImageInfo.ImageKey, sec->ImageInfo.ImageKey, TRUE);
		//for(i = 0; i < 4; i++)
		//	dbgPrintFake("%04x", dat[i]);
		//dbgPrintFake("\n");
	}
	ret = XexpVerifyXexHeadersSave(XexHeader, TransferHeader, TransferedHeader);
	//dbgPrintFake("XexpVerifyXexHeadersHook (%d) ret 0x%08x\n", switchKeys, ret);
	if(ret >= 0)
	{
		if(getOpt(OPT_AUTOFAKE))
		{
			PXEX_EXECUTION_ID exId = (PXEX_EXECUTION_ID)RtlImageXexHeaderField(XexHeader, XEX_HEADER_EXECUTION_ID);
			if(exId != NULL)
				procTitleFakeLive(exId->TitleID);
		}
	}
	return ret;
}


void __declspec(naked) XexpLoadImageSaveVar(void)
{
	__asm{
		li r3, XEXLOADIMAGE_VAL
		nop
		nop
		nop
		nop
		nop
		nop
		blr
	}
}
// snprintf 0x100 "\\Device\\Harddisk0\\SystemExtPartition\\%08X\\dash.xex", XamUpdateGetCurrentSystemVersion()
// "\\SystemRoot\\dash.xex"
// "\\Device\\Flash\\dash.xex"
extern BOOL g_FirstRun;
XEXPLOADIMAGEFUN XexpLoadImageSave = (XEXPLOADIMAGEFUN)XexpLoadImageSaveVar;
NTSTATUS XexpLoadImageHook(LPCSTR xexName, DWORD typeInfo, DWORD ver, PHANDLE modHandle)
{
	NTSTATUS ret;
	char* dxex = (char*)xexName;
	BOOL isDash = isXexDash(dxex);
	// crashes console after lhelper!
	//if(xexName != NULL)
	//{
	//	if(xexName[0] != 0)
	//		dbgPrintFake("XexpLoadImageHook load %s\n", xexName);
	//}
	if(isDash && g_FirstRun) // this is a fallback for when loadprep.c fails to catch on bootup
	{
		dxex = firstRunTasks(xexName);
	}
	ret = XexpLoadImageSave(dxex, typeInfo, ver, modHandle);
//	dbgPrintFake("XexpLoadImageHook load %s ret 0x%08x\n", xexName, ret);
	if((ret == 0xC000007B)||(ret == 0xC0000102))
	{
		//dbgPrintFake("try again with retail key\n");
		switchKeys = TRUE;
		doSync(&switchKeys);
		ret = XexpLoadImageSave(dxex, typeInfo, ver, modHandle);
		switchKeys = FALSE;
		doSync(&switchKeys);
	}
	if(ret >= 0) // module load was good
	{
		//dbgPrintFake("XexpLoadImageHook loaded %s ret 0x%08x type %08x ver %08x phandle %08x\n", xexName, ret, typeInfo, ver, modHandle);
		if(modHandle != NULL)
		{
			//if((stricmp(xexName, XEXLOAD_DASH2) == 0)||(stricmp(xexName, XEXLOAD_DASH) == 0))
			if(isXexDash(dxex)) // starting dashboard, wherever you are
			{
				//dbgPrintFake("patching dash.xex onload\n");
				if(!patchModuleSearchkey(*modHandle, dashPatchNxeInstall, DASHNXEINST_SZ, DASHNXE_POFF, &dashNxePatchInstall, 1))
					dbgPrintFake("WARNING: could not patch dash!\n");
			}
			else if((stricmp(xexName, XEXLOAD_SIGNIN) == 0)||(stricmp(xexName, XEXLOAD_CREATE) == 0))
			{ // when signin.xex or createprofile.xex are invoked in metro, this is used to stop dash launch from subverting dash or avatar editor
				g_SkipLaunchNotify = TRUE;
				doSync(&g_SkipLaunchNotify);
			}
			else if(stricmp(xexName, XEXLOAD_HUD) == 0)
			{
				g_SkipLaunchNotify = FALSE;
				doSync(&g_SkipLaunchNotify);
			}
		}
	}
	return ret;
}

static BOOL isXexHooked = FALSE;
static DWORD xexVerifyOld[4];
static DWORD xexLoadOld[4];
static PDWORD xexVerifyHookAddr = NULL;
static PDWORD xexLoadHookAddr = NULL;
void XexHook(void)
{
	if(!isXexHooked)
	{
		xexVerifyHookAddr = (PDWORD)findInterpretBranchOrd(MODULE_KERNEL, XEVERIFYIMAGEHEADERS_ORD, XEXVERIFY_MAX_SEARCH);
		xexLoadHookAddr = (PDWORD)findInterpretBranchOrd(MODULE_KERNEL, XEXLOADIMAGE_ORD, XEXLOADIMAGE_MAX_SEARCH);
		if((xexVerifyHookAddr != NULL)&&(xexLoadHookAddr != NULL))
		{
			//dbgPrintFake("xexhook: hooking XexpVerifyXexHeaders at 0x%08x\n", xexVerifyHookAddr);
			//dbgPrintFake("xexhook: hooking XexpLoadImage at 0x%08x\n", xexLoadHookAddr);
			hookFunctionStart(xexVerifyHookAddr, (PDWORD)XexpVerifyXexHeadersSaveVar, xexVerifyOld, (DWORD)XexpVerifyXexHeadersHook);
			hookFunctionStart(xexLoadHookAddr, (PDWORD)XexpLoadImageSaveVar, xexLoadOld, (DWORD)XexpLoadImageHook);
			isXexHooked = TRUE;
		}
	}
}

void XexUnhook(void)
{
	if(isXexHooked)
	{
		unhookFunctionStart(xexVerifyHookAddr, xexVerifyOld);
		unhookFunctionStart(xexLoadHookAddr, xexLoadOld);
		isXexHooked = FALSE;
	}
}

