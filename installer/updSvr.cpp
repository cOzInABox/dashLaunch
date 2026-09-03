#include "updSvr.h"
#include "XboxUtil.h"
#include "Resource.h"
#include "logging.h"

static USVR_RET_BOOL usvrStartup;
static USVR_RET_VOID usvrShutdown;
static USVR_RET_DWORD usvrStatus;
static USVR_GETSET_PATCH usvrGetPatch;
static USVR_GETSET_PATCH usvrSetPatch;
static USVR_RET_DWORD usvrGetConType;

Updsvr::Updsvr()
{
	usvrStartup = (USVR_RET_BOOL)NULL;
	usvrShutdown = (USVR_RET_VOID)NULL;
	usvrStatus = (USVR_RET_DWORD)NULL;
	usvrGetConType = (USVR_RET_DWORD)NULL;
	usvrGetPatch = (USVR_GETSET_PATCH)NULL;
	usvrSetPatch = (USVR_GETSET_PATCH)NULL;
	canUse = FALSE;
}

BOOL Updsvr::usvrResFuncts(HMODULE hMod)
{
	usvrStartup = (USVR_RET_BOOL)GetProcAddress(hMod, (LPCSTR)USVR_START_ORD);
	usvrShutdown = (USVR_RET_VOID)GetProcAddress(hMod, (LPCSTR)USVR_STOP_ORD);
	usvrStatus = (USVR_RET_DWORD)GetProcAddress(hMod, (LPCSTR)USVR_STAT_ORD);
	usvrGetConType = (USVR_RET_DWORD)GetProcAddress(hMod, (LPCSTR)USVR_CONTYP_ORD);
	usvrGetPatch = (USVR_GETSET_PATCH)GetProcAddress(hMod, (LPCSTR)USVR_GETPATCH_ORD);
	usvrSetPatch = (USVR_GETSET_PATCH)GetProcAddress(hMod, (LPCSTR)USVR_SETPATCH_ORD);
#ifdef LOG_EXTRA_OUT
	lDbgPrint("usvrResFuncts: start  : %08x shutdown: %08x status  : %08x\n", usvrStartup, usvrShutdown, usvrStatus);
	lDbgPrint("usvrResFuncts: contype: %08x getpatch: %08x setpatch: %08x\n", usvrGetConType, usvrGetPatch, usvrSetPatch);
#endif
	if((usvrStartup == NULL) || (usvrShutdown == NULL) || (usvrStatus == NULL))
		return FALSE;
	if((usvrGetConType == NULL) || (usvrGetPatch == NULL) || (usvrSetPatch == NULL))
		return FALSE;
	canUse = TRUE;
	return TRUE;
}

BOOL Updsvr::usvrLoad(void)
{
	PVOID loadAddr;
	DWORD size;
	if(Resource::GetInstance().GetEmbeddedFile("usvr", &loadAddr, &size))
	{
		PBYTE dat = (BYTE*)VirtualAlloc(NULL, size, MEM_COMMIT|MEM_LARGE_PAGES, PAGE_READWRITE);
		if(dat)
		{
			NTSTATUS ret;
			HMODULE hTemp;
			XMemCpy(dat, loadAddr, size);
			//lDbgPrint("module at %08x size %08x\n", loadAddr, size);
			ret = XexLoadImageFromMemory(dat, size, "usvr.xex", XEX_MODULE_TYPE_TITLE_DLL, 0, (PHANDLE)&hTemp);
			VirtualFree(dat, 0, MEM_RELEASE);
			if(ret >= 0)
			{
				BOOL ret = usvrResFuncts(hTemp);
#ifdef LOG_EXTRA_OUT
				if(ret)
					lDbgPrint("usvrLoad: resolved functions OK\n");
				else
					lDbgPrint("usvrLoad: could not resolve functions\n");
#endif
				return ret;
			}
			else
				lDbgPrint("loading usvr.xex failed! ret: 0x%08x\n", ret);
		}
		else
			lDbgPrint("unable to allocate memory to load usvr.xex\n");
	}
#ifdef LOG_EXTRA_OUT
	else
		lDbgPrint("usvrLoad: could not locate embedded file\n");
#endif

	return FALSE;
}

BOOL Updsvr::startup(void)
{
	BOOL ret = FALSE;
	if((usvrStartup != NULL)&&(canUse))
	{
		ret = usvrStartup();
#ifdef LOG_EXTRA_OUT
		lDbgPrint("Updsvr::startup: returned %d\n", ret);
#endif
	}
#ifdef LOG_EXTRA_OUT
	else
		lDbgPrint("Updsvr::startup: could not make call!\n");
#endif
	return ret;
}

void Updsvr::shutdown(void)
{
	if((usvrShutdown != NULL)&&(canUse))
	{
		usvrShutdown();
	}
#ifdef LOG_EXTRA_OUT
	else
		lDbgPrint("Updsvr::shutdown: could not make call!\n");
#endif
}

DWORD Updsvr::status(void)
{
	DWORD ret = 0;
	if((usvrStatus != NULL)&&(canUse))
	{
		ret = usvrStatus();
#ifdef LOG_EXTRA_OUT
		lDbgPrint("Updsvr::status: returned %08x\n", ret);
#endif
	}
#ifdef LOG_EXTRA_OUT
	else
		lDbgPrint("Updsvr::status: could not make call!\n");
#endif
	return ret;
}

DWORD Updsvr::getConType(void)
{
	DWORD ret = 0;
	if((usvrGetConType != NULL)&&(canUse))
	{
		ret = usvrGetConType();
#ifdef LOG_EXTRA_OUT
		lDbgPrint("Updsvr::getConType: returned %08x\n", ret);
#endif
	}
#ifdef LOG_EXTRA_OUT
	else
		lDbgPrint("Updsvr::usvrGetConType: could not make call!\n");
#endif
	return ret;
}

BOOL Updsvr::getPatches(void* buf, int blen)
{
	BOOL ret = FALSE;
	if((usvrGetPatch != NULL)&&(canUse))
	{
		ret = usvrGetPatch(buf, blen);
#ifdef LOG_EXTRA_OUT
		lDbgPrint("Updsvr::getPatches: returned %d\n", ret);
#endif
	}
#ifdef LOG_EXTRA_OUT
	else
		lDbgPrint("Updsvr::getPatches: could not make call!\n");
#endif
	return ret;
}

BOOL Updsvr::setPatches(void* buf, int blen)
{
	BOOL ret = FALSE;
	if((usvrSetPatch != NULL)&&(canUse))
	{
		ret = usvrSetPatch(buf, blen);
#ifdef LOG_EXTRA_OUT
		lDbgPrint("Updsvr::setPatches: returned %d\n", ret);
#endif
	}
#ifdef LOG_EXTRA_OUT
	else
		lDbgPrint("Updsvr::setPatches: could not make call!\n");
#endif
	return ret;
}

