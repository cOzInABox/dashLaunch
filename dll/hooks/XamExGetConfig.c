#include "_hook_includes.h"

#define XAM_EXGETXCONFIG_ORD	16
#define XAM_OOBE_BIT			(1<<6)

BOOL XamExGetXConfigSettingHook(WORD dwCategory, WORD dwSetting, PVOID pBuffer, WORD cbBuffer, PWORD szSetting)
{
	NTSTATUS ret = ExGetXConfigSetting(dwCategory, dwSetting, pBuffer, cbBuffer, szSetting);
	if(ret >= 0)
	{
		if((dwCategory == XCONFIG_USER_CATEGORY)&&(dwSetting == XCONFIG_USER_RETAIL_FLAGS))
		{
			PDWORD pbuf = (PDWORD)pBuffer;
			pbuf[0] = pbuf[0]|XAM_OOBE_BIT;
		}
	}
	return ret;
}

static IMPORT_HOOK_SAVE XamExGetXConfigSettingSave;
static BOOL isXamExGetXConfigSettingHooked = FALSE;
void XamOobeHook(void)
{
	if(!isXamExGetXConfigSettingHooked)
	{
		if(hookImpStub(MODULE_XAM, MODULE_KERNEL, XAM_EXGETXCONFIG_ORD, (DWORD)XamExGetXConfigSettingHook, &XamExGetXConfigSettingSave))
		{
			isXamExGetXConfigSettingHooked = TRUE;
		}
	}
}

void XamOobeUnhook(void)
{
	if(isXamExGetXConfigSettingHooked)
	{
		unhookImpStub(&XamExGetXConfigSettingSave);
		isXamExGetXConfigSettingHooked = FALSE;
	}
}



