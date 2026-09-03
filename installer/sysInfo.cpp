#include <xtl.h>
#include "xkelib.h"
#include "sysInfo.h"
#include "XboxUtil.h"
#include "Resource.h"
#include "Strings.h"
#include "logging.h"

SysInfo::SysInfo()
{
	m_fXconfGood = FALSE;
}

VOID SysInfo::DisableControls(VOID)
{
	m_SaveSmc.SetEnable(FALSE);
	m_CpuSlide.SetEnable(FALSE);
	m_GpuSlide.SetEnable(FALSE);
	m_EdramSlide.SetEnable(FALSE);
	m_CpuFanSlide.SetEnable(FALSE);
	m_GpuFanSlide.SetEnable(FALSE);
	m_CpuFanCheck.SetEnable(FALSE);
	m_GpuFanCheck.SetEnable(FALSE);
}

VOID SysInfo::Init(CXuiScene sscene, CXuiControl ssave)
{
	int i;
	BYTE tbuf[0x10];
	CHAR ctempTxt[128];
	WCHAR tempTxt[128];
	DWORD dwTemp;
	WORD wTemp;
	CXuiTextElement ttx;
	LPCWSTR pwtmp;
	selfScene = sscene;
	m_SaveSmc = ssave;
	m_SaveSmc.SetShow(FALSE);
	selfScene.GetChildById(L"InfoTitle", &m_Title);
	selfScene.GetChildById(L"CpuSlider", &m_CpuSlide);
	selfScene.GetChildById(L"GpuSlider", &m_GpuSlide);
	selfScene.GetChildById(L"EdramSlider", &m_EdramSlide);
	selfScene.GetChildById(L"ConfigureTitle", &m_SmcTitle);
	selfScene.GetChildById(L"CpuFanSpeed", &m_CpuFanSlide);
	selfScene.GetChildById(L"GpuFanSpeed", &m_GpuFanSlide);
	selfScene.GetChildById(L"CpuFanCheck", &m_CpuFanCheck);
	selfScene.GetChildById(L"GpuFanCheck", &m_GpuFanCheck);

	m_SaveSmc.SetText(Strings::GetInstance().Lookup(L"info_save"));

	Strings::GetInstance().LookupAndSetChild(selfScene, L"SmcHelp", L"info_smhelp");
	
	m_CpuSlide.SetFocus();
	m_Title.SetText(Strings::GetInstance().Lookup(L"info_title"));
	m_SmcTitle.SetText(Strings::GetInstance().Lookup(L"info_titlesmc"));

	pwtmp = Strings::GetInstance().Lookup(L"info_target");
	wsprintfW(tempTxt, L"CPU %s", pwtmp);
	m_CpuSlide.SetText(tempTxt);

	wsprintfW(tempTxt, L"GPU %s", pwtmp);
	m_GpuSlide.SetText(tempTxt);

	wsprintfW(tempTxt, L"EDRAM %s", pwtmp);
	m_EdramSlide.SetText(tempTxt);

	pwtmp = Strings::GetInstance().Lookup(L"info_fan");
	wsprintfW(tempTxt, L"CPU %s", pwtmp);
	m_CpuFanSlide.SetText(tempTxt);

	wsprintfW(tempTxt, L"GPU %s", pwtmp);
	m_GpuFanSlide.SetText(tempTxt);

	// get CPU key
	ZeroMemory(tbuf, 0x10);
	selfScene.GetChildById(L"CpuKey", &ttx);
	if(XboxUtil::GetInstance().GetCpuKey(tbuf)) // 0F17D09D89EA12B1716E5D134F8266FF
	{
		ZeroMemory(ctempTxt, 0x60);
		for(i = 0; i < 0x10; i++)
		{
			sprintf_s(&ctempTxt[(i*2)], 0x40, "%02X", (tbuf[i]&0xFF));
		}
		wsprintfW(tempTxt, L"%S", ctempTxt);
		ttx.SetText(tempTxt);
	}
	else
	{
		lDbgPrint("not showing cpu key\n");
		ttx.SetShow(FALSE);
	}

	// get DVD key
	ZeroMemory(tbuf, 0x10);
	selfScene.GetChildById(L"DvdKey", &ttx);
	if(XboxUtil::GetInstance().GetDvdKey(tbuf)) // B75EA8B074D84220C5D7C93928FF6583
	{
		ZeroMemory(ctempTxt, 0x60);
		for(i = 0; i < 0x10; i++)
		{
			sprintf_s(&ctempTxt[(i*2)], 0x40, "%02X", (tbuf[i]&0xFF));
		}
		wsprintfW(tempTxt, L"%S", ctempTxt);
		ttx.SetText(tempTxt);
	}
	else
	{
		lDbgPrint("not showing dvd key\n");
		ttx.SetShow(FALSE);
	}

	// get X Value
	dwTemp = XboxUtil::GetInstance().GetXVal();
	wsprintfW(tempTxt, L"0x%08x", dwTemp);
	selfScene.GetChildById(L"XVal", &ttx);
	ttx.SetText(tempTxt);

	// get Console Id
	ZeroMemory(ctempTxt, 0x60);
	selfScene.GetChildById(L"ConsoleId", &ttx);
	if(XeKeysGetConsoleID(NULL, ctempTxt) >= 0)
	{
		wsprintfW(tempTxt, L"%S", ctempTxt);
		ttx.SetText(tempTxt);
	}
	else
	{
		lDbgPrint("not showing console ID\n");
		ttx.SetShow(FALSE);
	}

	// get Console Serial number
	ZeroMemory(tbuf, 0x10);
	selfScene.GetChildById(L"ConsoleSerial", &ttx);
	dwTemp = 0xC;
	if(XeKeysGetKey(XEKEY_CONSOLE_SERIAL_NUMBER, tbuf, &dwTemp) >= 0)
	{
		wsprintfW(tempTxt, L"%S", tbuf);
		ttx.SetText(tempTxt);
	}
	else
		ttx.SetShow(FALSE);

	ZeroMemory(tbuf, 0x10);
	selfScene.GetChildById(L"MacId", &ttx);
	if(ExGetXConfigSetting(XCONFIG_SECURED_CATEGORY, XCONFIG_SECURED_MAC_ADDRESS, &tbuf, 6, &wTemp) >= 0)
	{
		wsprintfW(tempTxt, L"%02X:%02X:%02X:%02X:%02X:%02X", tbuf[0],tbuf[1],tbuf[2],tbuf[3],tbuf[4],tbuf[5]);
		ttx.SetText(tempTxt);
	}
	else
		ttx.SetShow(FALSE);
	
	// get smc config from xconfig
	if(ExGetXConfigSetting(XCONFIG_STATIC_CATEGORY, XCONFIG_STATIC_DATA, &m_xcfs, sizeof(XCONFIG_STATIC_SETTINGS), &m_xcfs_sz) >= 0)
	{
		//lDbgPrint("get xconfig worked out\n");
		//CXuiSlider m_CpuSlide; // text_Label
		//CXuiSlider m_GpuSlide;
		//CXuiSlider m_EdramSlide;
		m_CpuSlide.SetRange(40, m_xcfs.SMCConfig.Temperature.Overload.Cpu);
		m_GpuSlide.SetRange(40, m_xcfs.SMCConfig.Temperature.Overload.Gpu);
		m_EdramSlide.SetRange(40, m_xcfs.SMCConfig.Temperature.Overload.Edram);
		m_CpuSlide.SetValue(m_xcfs.SMCConfig.Temperature.SetPoint.Cpu);
		m_GpuSlide.SetValue(m_xcfs.SMCConfig.Temperature.SetPoint.Gpu);
		m_EdramSlide.SetValue(m_xcfs.SMCConfig.Temperature.SetPoint.Edram);
		if(m_xcfs.SMCConfig.fanOrCpu.Enable)
		{
			m_CpuFanSlide.SetValue(m_xcfs.SMCConfig.fanOrCpu.Speed);
			m_CpuFanCheck.SetCheck(TRUE);
		}
		else
		{
			m_CpuFanSlide.SetEnable(FALSE);
		}
		if(m_xcfs.SMCConfig.fanOrGpu.Enable)
		{
			m_GpuFanSlide.SetValue(m_xcfs.SMCConfig.fanOrGpu.Speed);
			m_GpuFanCheck.SetCheck(TRUE);
		}
		else
		{
			m_GpuFanSlide.SetEnable(FALSE);
		}
		//lDbgPrint("console from flags: %08x\n", CONSOLE_TYPE_FROM_FLAGS);
		if(CONSOLE_TYPE_FROM_FLAGS != CONSOLE_TYPE_XENON)
		{
			if(IS_CONSOLE_TYPE_SLIM)
			{
				m_GpuFanCheck.SetShow(FALSE);
				m_GpuFanSlide.SetShow(FALSE);
			}
			else
			{
				m_CpuFanCheck.SetShow(FALSE);
				m_CpuFanSlide.SetShow(FALSE);
			}
		}
		m_fXconfGood = TRUE;
	}
	else
	{
// 		lDbgPrint("get xconfig didn't work out\n");
		DisableControls();
	}
}

HRESULT SysInfo::SliderChange(HXUIOBJ hObjSource, int newValue)
{
	if(hObjSource == m_CpuFanSlide)
	{
		m_SaveSmc.SetShow(TRUE);
		XboxUtil::GetInstance().SetFanSpeeds(SMC_FAN_CPU, newValue);
	}
	else if (hObjSource == m_GpuFanSlide)
	{
		m_SaveSmc.SetShow(TRUE);
		XboxUtil::GetInstance().SetFanSpeeds(SMC_FAN_GPU, newValue);
	}
	else if((hObjSource == m_CpuSlide)||(hObjSource == m_GpuSlide)||(hObjSource == m_EdramSlide))
	{
		m_SaveSmc.SetShow(TRUE);
// 		lDbgPrint("other slider changed\n");
	}
	return S_OK;
}

HRESULT SysInfo::CheckChange(HXUIOBJ hObjSource)
{
	int val;
	//CXuiCheckbox m_CpuFanCheck;
	//CXuiCheckbox m_GpuFanCheck;
	if(hObjSource == m_CpuFanCheck)
	{
		m_SaveSmc.SetShow(TRUE);
		if(m_CpuFanCheck.IsChecked())
		{
// 			lDbgPrint("I see checked\n");
			m_CpuFanSlide.SetEnable(TRUE);
			m_CpuFanSlide.GetValue(&val);
			XboxUtil::GetInstance().SetFanSpeeds(SMC_FAN_CPU, val);
		}
		else
		{
// 			lDbgPrint("I see unchecked\n");
			m_CpuFanSlide.SetEnable(FALSE);
			XboxUtil::GetInstance().SetFanSpeeds(SMC_FAN_CPU, 0x7F, TRUE);
		}
	}
	else if (hObjSource == m_GpuFanCheck)
	{
		m_SaveSmc.SetShow(TRUE);
		if(m_GpuFanCheck.IsChecked())
		{
// 			lDbgPrint("I see checked\n");
			m_GpuFanSlide.SetEnable(TRUE);
			m_GpuFanSlide.GetValue(&val);
			XboxUtil::GetInstance().SetFanSpeeds(SMC_FAN_GPU, val);
		}
		else
		{
// 			lDbgPrint("I see unchecked\n");
			m_GpuFanSlide.SetEnable(FALSE);
			XboxUtil::GetInstance().SetFanSpeeds(SMC_FAN_GPU, 0x7F, TRUE);
		}
	}
	return S_OK;
}

VOID SysInfo::SaveSmcChanges(VOID)
{
	//CXuiSlider m_CpuFanSlide;
	//CXuiSlider m_GpuFanSlide;
	//CXuiCheckbox m_CpuFanCheck;
	//CXuiCheckbox m_GpuFanCheck;
	if(m_fXconfGood)
	{
		int val;
		m_CpuSlide.GetValue(&val);
		m_xcfs.SMCConfig.Temperature.SetPoint.Cpu = (BYTE)val&0xFF;
		m_GpuSlide.GetValue(&val);
		m_xcfs.SMCConfig.Temperature.SetPoint.Gpu = (BYTE)val&0xFF;
		m_EdramSlide.GetValue(&val);
		m_xcfs.SMCConfig.Temperature.SetPoint.Edram = (BYTE)val&0xFF;
		if(m_CpuFanCheck.IsShown())
		{
			if(m_CpuFanCheck.IsChecked())
			{
				m_CpuFanSlide.GetValue(&val);
				m_xcfs.SMCConfig.fanOrCpu.Speed = (BYTE)val&0xFF;
				m_xcfs.SMCConfig.fanOrCpu.Enable = 1;
			}
			else
			{
				m_xcfs.SMCConfig.fanOrCpu.Speed = 0x7F;
				m_xcfs.SMCConfig.fanOrCpu.Enable = 0;
			}
		}
		if(m_GpuFanCheck.IsShown())
		{
			if(m_GpuFanCheck.IsChecked())
			{
				m_GpuFanSlide.GetValue(&val);
				m_xcfs.SMCConfig.fanOrGpu.Speed = (BYTE)val&0xFF;
				m_xcfs.SMCConfig.fanOrGpu.Enable = 1;
			}
			else
			{
				m_xcfs.SMCConfig.fanOrGpu.Speed = 0x7F;
				m_xcfs.SMCConfig.fanOrGpu.Enable = 0;
			}
		}
		// recalc hash
		m_xcfs.CheckSum = XboxUtil::GetInstance().CalcSmcConfigHash((PBYTE)&m_xcfs);
		// write to xconfig
		NTSTATUS sta = ExSetXConfigSetting(XCONFIG_STATIC_CATEGORY, XCONFIG_STATIC_DATA, &m_xcfs, m_xcfs_sz);
		lDbgPrint("saving static size 0x%x (sizeof 0x%x) returns %x\n", m_xcfs_sz, sizeof(XCONFIG_STATIC_SETTINGS), sta);
		if(sta >= 0)
		{
			m_SaveSmc.SetShow(FALSE);
			m_CpuSlide.SetFocus();
		}
	}
}
