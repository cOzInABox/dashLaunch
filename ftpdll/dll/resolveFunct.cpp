#include <xtl.h>
#include "types.h"


#ifdef __cplusplus
extern "C" {
#endif
	UINT32 __stdcall XexGetModuleHandle(char* module, PVOID hand); //ie XexGetModuleHandle("xam.xex", &hand);// uint32 hand, returns 0 on success
	UINT32 __stdcall XexGetProcedureAddress(UINT32 hand ,UINT32, PVOID);// ie XexGetProcedureAddress(hand ,0x50, &addr); // uint32 addr, returns 0 on success
#ifdef __cplusplus
}
#endif

UINT32 resolveFunct(char* modname, UINT32 ord)
{
	UINT32 ptr32=0, ret=0, ptr2=0;
	ret = XexGetModuleHandle(modname, &ptr32); //xboxkrnl.exe xam.dll?
	//console.Format("%s - XexGetModuleHandle ret: %08x, ptr32: %08x\n", modname, ret, ptr32);
	if(ret == 0)
	{
		ret = XexGetProcedureAddress(ptr32, ord, &ptr2 );
		//console.Format("%s - XexGetProcedureAddress ret: %08x, ptr2: %08x\n", modname, ret, ptr2);
		if(ptr2 != 0)
			return ptr2;
	}
	return 0; // function not found
}
