#include "_hook_includes.h"

BOOL XamCheckExecPriv(DWORD priv)
{
	BOOL ret = XexCheckExecutablePrivilege(priv);
	if(priv == XEX_PRIVILEGE_INSECURE_SOCKETS)
	{
		if(getOpt(OPT_SOCKPATCH))
			ret = TRUE;
	}
	else if(priv == XEX_PRIVILEGE_AUTHENTICATION_EX_REQUIRED) // remove AP25 flags, we don't need no steenkin AP25
		ret = FALSE;
	//dbgPrintFake("priv: %d ret %d\n", priv, ret);
	//else if(priv == PRIV_MULTIDISK_INSECURE) // AP25 multidisk titles are required to have this as well as PRIV_MULTIDISK_SWAP
	//	ret = FALSE;
	//else if(priv == PRIV_CROSS_SYSTEMLINK)
	//{
	//	DbgPrint("*** priv cross system link checked, ret %d\n", ret);
	//	ret = TRUE;
	//}
	//else if(priv == PRIV_TITLE_INSTALL_INCOMPAT)
	//	ret = FALSE;
	return ret;
}

static IMPORT_HOOK_SAVE privsSave;
static BOOL isPrivsHooked = FALSE;
void privsHook(void)
{
	if(!isPrivsHooked)
	{
		hookImpStub(MODULE_XAM, MODULE_KERNEL, kernelExp_XexCheckExecutablePrivilege, (DWORD)XamCheckExecPriv, &privsSave);
		isPrivsHooked = TRUE;
	}
}

void privsUnhook(void)
{
	if(isPrivsHooked)
	{
		unhookImpStub(&privsSave);
		isPrivsHooked = FALSE;
	}
}
