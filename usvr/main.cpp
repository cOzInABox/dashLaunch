//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
#include <xtl.h>
#include <xbox.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include "xkelib.h"
#include "util.h"
#include "svr.h"
#include "logging.h"

extern "C" const TCHAR szModuleName[] = "usvr.xex";

extern "C" BOOL usvrStartup(void)
{
	return Svr::GetInstance().startup();
	return FALSE;
}

extern "C" void usvrShutdown(void)
{
	Svr::GetInstance().shutdown();
}

extern "C" DWORD usvrStatus(void)
{
	return Svr::GetInstance().getStatus();
}

extern "C" DWORD usvrGetConType(void)
{
	return Nand::Inst().getConType();
}

extern "C" BOOL usvrGetPatch(void* bout, int blen)
{
	return Nand::Inst().getPatchData(bout, blen);
}

extern "C" BOOL usvrSetPatch(void* bin, int blen)
{
	return Nand::Inst().setPatchData(bin, blen);
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  dwReason, LPVOID lpReserved )
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
#ifdef _DEBUG
			_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
			DbgLog::GetInstance().log("updsvr: dll has attached\n");
			break;
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
			break;
		case DLL_PROCESS_DETACH:
			DbgLog::GetInstance().log("updsvr: dll detaching\n");
			usvrShutdown();
			break;
	}
	return TRUE;
}

