#define XAMLIVEHIVE_VAL			10
#define XAMLIVEHIVEA_VAL		11
		hookFunctionStart((PDWORD)0x81695E10, (PDWORD)XampGetLiveHiveValueASaveVar, (DWORD)XampGetLiveHiveValueAHook);
		hookFunctionStart((PDWORD)0x81696878, (PDWORD)XampGetLiveHiveValueSaveVar, (DWORD)XampGetLiveHiveValueHook);

// 81695E10 # HRESULT __stdcall XampGetLiveHiveValueA(const char *szName, char *szValue, DWORD cchValue, DWORD unk, PXOVERLAPPED xov, BOOL xmsgQuery, DWORD dwFlags)
VOID __declspec(naked) XampGetLiveHiveValueASaveVar(VOID)
{
	__asm{
		li r3, XAMLIVEHIVEA_VAL
		nop
		nop
		nop
		nop
		nop
		nop
		blr
	}
}
typedef HRESULT (*XAMLIVEHIVEAFUN)(const char *szName, char *szValue, DWORD cchValue, DWORD unk, PXOVERLAPPED xov, BOOL xmsgQuery, DWORD dwFlags);
XAMLIVEHIVEAFUN XampGetLiveHiveValueASave = (XAMLIVEHIVEAFUN)XampGetLiveHiveValueASaveVar;
HRESULT XampGetLiveHiveValueAHook(const char *szName, char *szValue, DWORD cchValue, DWORD unk, PXOVERLAPPED xov, BOOL xmsgQuery, DWORD dwFlags)
{
	HRESULT ret = E_FAIL;
	if(stricmp(szName, "IsXlinkEnabled") == 0)
	{
		strcpy(szValue, "0");
		ret = ERROR_SUCCESS;
	}
	else if(stricmp(szName, "XlfsBackgroundModeEnabled") == 0)
	{
		strcpy(szValue, "FALSE");
		ret = ERROR_SUCCESS;
	}
	else if(stricmp(szName, "ProfileSyncIntervalInSeconds") == 0)
	{
		strcpy(szValue, "6000");
		ret = ERROR_SUCCESS;
	}
	else if(stricmp(szName, "XlfsTestFdPort") == 0)
	{
		strcpy(szValue, "444");
		ret = ERROR_SUCCESS;
	}
	else if(stricmp(szName, "XlfsTestSSL") == 0)
	{
		strcpy(szValue, "FALSE");
		ret = ERROR_SUCCESS;
	}
	else if(stricmp(szName, "XlfsTestIgnoreSSLCert") == 0)
	{
		strcpy(szValue, "TRUE");
		ret = ERROR_SUCCESS;
	}
	else if(stricmp(szName, "XlfsTestProxyIP") == 0)
	{
		szValue[0] = 0x0;
		ret = ERROR_SUCCESS;
	}
	else if(stricmp(szName, "XlfsAsyncUploadInterval") == 0)
	{
		strcpy(szValue, "6000");
		ret = ERROR_SUCCESS;
	}
	else if(stricmp(szName, "VoiceCollectionOverrideEnabled") == 0)
	{
		strcpy(szValue, "0");
		ret = ERROR_SUCCESS;
	}
	else
		ret = XampGetLiveHiveValueASave(szName, szValue, cchValue, unk, xov, xmsgQuery, dwFlags);
#ifdef DEBUG_SIGNINSTATE_OUT
	if(ret >= 0)
		DbgPrint("ValueA:%x '%s' res '%s' cch: %x unk:%x xmsg: %x flag: %x\n", ret, szName, szValue, cchValue, unk, xmsgQuery, dwFlags);
	else
		DbgPrint("ValueA:%x '%s' res '' cch: %x unk:%x xmsg: %x flag: %x\n", ret, szName, cchValue, unk, xmsgQuery, dwFlags);
#endif

	if(ret == 0x80151802)
		ret = E_FAIL;
	return ret;
}

// 81696878 # HRESULT __stdcall XampGetLiveHiveValue(const char *szName, char *szValue, DWORD cchValue, HANDLE hiveHandle, DWORD dwFlags)
VOID __declspec(naked) XampGetLiveHiveValueSaveVar(VOID)
{
	__asm{
		li r3, XAMLIVEHIVE_VAL
		nop
		nop
		nop
		nop
		nop
		nop
		blr
	}
}
typedef HRESULT (*XAMLIVEHIVEFUN)(const char *szName, char *szValue, DWORD cchValue, HANDLE hiveHandle, DWORD dwFlags);
XAMLIVEHIVEFUN XampGetLiveHiveValueSave = (XAMLIVEHIVEFUN)XampGetLiveHiveValueSaveVar;
HRESULT XampGetLiveHiveValueHook(const char *szName, char *szValue, DWORD cchValue, HANDLE hiveHandle, DWORD dwFlags)
{
	HRESULT ret = XampGetLiveHiveValueSave(szName, szValue, cchValue, hiveHandle, dwFlags);
#ifdef DEBUG_SIGNINSTATE_OUT
	DbgPrint("Value:%x '%s' res '%s' cch: %x hh: %x flag: %x\n", ret, szName, szValue, cchValue, hiveHandle, dwFlags);
#endif
	return ret;
}
// # HRESULT __stdcall XamGetLiveHiveValueW(const wchar_t *szName, wchar_t *wcsValue, DWORD cwchValue, DWORD unk, PXOVERLAPPED xov)

/* **************************************************************************************************************** */


