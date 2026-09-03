#include <string>
#include <cstring>
#include "IniList.h"
#include "dashLaunch.h"
#include "stdio.h"
#include "Resource.h"
#include "dlDrives.h"
#include "XboxUtil.h"
#include "optionsData.h"
#include "Nand.h"
#include "Strings.h"
#include "SimpleIni.h"
#include "logging.h"

using namespace std;

#define INI_GET_MOUNTPOINT(x)	(dlDrives::GetInstance().GetDrivePath(x))
#define INI_GET_FRIENDLY(x)		(dlDrives::GetInstance().GetDriveFriendly(x))
#define INIDRIVEMOUNT	"ini:"
#define INIDRIVE		"ini:\\"
#define INIPATH			"ini:\\launch.ini"
#define INIPATHTEST		"ini:\\launch_test.ini"

IniData::IniData(void)
{
	m_ListData.nItems = 0;
	g_ListData = NULL;
	m_ListData.pItems = g_ListData;
	m_OverrideIni = -1;
}

// returns true if it's cleaning up an existing list
BOOL IniData::CleanupList(VOID)
{
	BOOL ret = FALSE;
	if(m_ListData.nItems != 0)
	{
		m_localList.DeleteItems(0, m_ListData.nItems);
		m_ListData.nItems = 0;
		ret = TRUE;
	}
	if(g_ListData != NULL)
	{
		delete [] g_ListData;
		g_ListData = NULL;
	}
	m_ListData.pItems = g_ListData;
	m_pathBox.SetText(L"");
	m_infoBox.SetText(L"");
	return ret;
}

// VOID IniData::SetIniItem(int idx, DWORD sts, PCHAR driveName, PCHAR drivePath)
VOID IniData::SetIniItem(int idx)
{
	DWORD sts = dlDrives::GetInstance().CheckIniStatusNoUmt(idx);
	PCHAR driveName = INI_GET_FRIENDLY(idx);
	PCHAR drivePath = INI_GET_MOUNTPOINT(idx);
	if(driveName != NULL)
	{
		char dtemp[MAX_PATH];
		ZeroMemory(dtemp, MAX_PATH);
		strncpy(dtemp, driveName, strlen(driveName)-1);
		wsprintfW(m_ListData.pItems[idx].optValTxt, L"%S", dtemp);
	}
	if(drivePath != NULL)
		wsprintfW(m_ListData.pItems[idx].optPathTxt, L"%S", drivePath);
	m_ListData.pItems[idx].isCurrentIni = FALSE;
	switch(sts&DLDRIVE_VALID)//DLDRIVE_INI_CURR
	{
	case DLDRIVE_NODISK:
		wcscpy(m_ListData.pItems[idx].wsize, Strings::GetInstance().Look(L"ini_nodisk"));
		m_ListData.pItems[idx].hasIni = FALSE;
		m_ListData.pItems[idx].fEnabled = FALSE;
		break;
	case DLDRIVE_NOINI:
		dlDrives::GetInstance().GetDriveSizeWcharNoMnt(idx, m_ListData.pItems[idx].wsize);
		m_ListData.pItems[idx].hasIni = FALSE;
		m_ListData.pItems[idx].fEnabled = TRUE;
		break;
	case DLDRIVE_INI:
		dlDrives::GetInstance().GetDriveSizeWcharNoMnt(idx, m_ListData.pItems[idx].wsize);
		if(((sts&DLDRIVE_INI_CURR) == DLDRIVE_INI_CURR)||(idx == m_OverrideIni))
			m_ListData.pItems[idx].isCurrentIni = TRUE;
		m_ListData.pItems[idx].hasIni = TRUE;
		m_ListData.pItems[idx].fEnabled = TRUE;
		break;
	}
	m_localList.SetItemEnable(idx, m_ListData.pItems[idx].fEnabled);
	XboxUtil::GetInstance().DeleteLink(DLDRIVEMOUNT);
}

VOID IniData::SetIniItem(int idx, DWORD sts, PCHAR driveName, PCHAR drivePath)
{
	if(driveName != NULL)
	{
		char dtemp[MAX_PATH];
		ZeroMemory(dtemp, MAX_PATH);
		strncpy(dtemp, driveName, strlen(driveName)-1);
		wsprintfW(m_ListData.pItems[idx].optValTxt, L"%S", dtemp);
	}
	if(drivePath != NULL)
		wsprintfW(m_ListData.pItems[idx].optPathTxt, L"%S", drivePath);
	m_ListData.pItems[idx].isCurrentIni = FALSE;
	switch(sts&DLDRIVE_VALID)//DLDRIVE_INI_CURR
	{
		case DLDRIVE_NODISK:
			wcscpy(m_ListData.pItems[idx].wsize, Strings::GetInstance().Look(L"ini_nodisk"));
			m_ListData.pItems[idx].hasIni = FALSE;
			m_ListData.pItems[idx].fEnabled = FALSE;
			break;
		case DLDRIVE_NOINI:
			dlDrives::GetInstance().GetDriveSizeWchar(idx, m_ListData.pItems[idx].wsize);
			m_ListData.pItems[idx].hasIni = FALSE;
			m_ListData.pItems[idx].fEnabled = TRUE;
			break;
		case DLDRIVE_INI:
			dlDrives::GetInstance().GetDriveSizeWchar(idx, m_ListData.pItems[idx].wsize);
			if(((sts&DLDRIVE_INI_CURR) == DLDRIVE_INI_CURR)||(idx == m_OverrideIni))
				m_ListData.pItems[idx].isCurrentIni = TRUE;
			m_ListData.pItems[idx].hasIni = TRUE;
			m_ListData.pItems[idx].fEnabled = TRUE;
			break;
	}
	m_localList.SetItemEnable(idx, m_ListData.pItems[idx].fEnabled);
}

VOID IniData::FetchList(VOID)
{
	if(DashLaunch::GetInstance().canUseImports())
	{
		int i;
#ifdef LOG_EXTRA_OUT
		lDbgPrint("IniData::FetchList\n");
#endif
		m_ListData.nItems = dlDrives::GetInstance().GetNumIniDrives();
		if(g_ListData != NULL)
			delete [] g_ListData;
		g_ListData = new INI_ITEM_INFO[m_ListData.nItems];
		m_ListData.pItems = g_ListData;
		for(i = 0; i < m_ListData.nItems; i++)
		{
// 			SetIniItem(i, dlDrives::GetInstance().CheckIniStatus(i), INI_GET_FRIENDLY(i), INI_GET_MOUNTPOINT(i));
			SetIniItem(i);
		}
		m_localList.InsertItems(0, m_ListData.nItems);
	}
	else
		CleanupList();
}

// expects existing items to be present
VOID IniData::RefreshList(VOID)
{
	if(DashLaunch::GetInstance().canUseImports())
	{
		int i;
#ifdef LOG_EXTRA_OUT
		lDbgPrint("IniData::RefreshList\n");
#endif
		for(i = 0; i < m_ListData.nItems; i++)
		{
			SetIniItem(i, dlDrives::GetInstance().CheckIniStatus(i), INI_GET_FRIENDLY(i), INI_GET_MOUNTPOINT(i));
		}
	}
}

VOID IniData::WriteToFile(DWORD idx)
{
	// load the template ini
	DWORD sz;
	PVOID addr;
	if(Resource::GetInstance().GetEmbeddedFile("ini", &addr, &sz))
	{
		int i, numItems;
		CSimpleIniA ini(false, false, false);
		WCHAR infoText[128];
		if(!dlDrives::GetInstance().IsDriveFlash(idx))
		{
			SI_Error serr = ini.LoadData((const char*)addr, sz);
			//lDbgPrint("load embedded ini returns 0x%x\n", serr);
		}
		//lDbgPrint("writing to file\n");
		XboxUtil::GetInstance().MountPath(INIDRIVEMOUNT, INI_GET_MOUNTPOINT(idx));
		// apply current settings
		numItems = OptionsData::GetInstance().GetFullNumItems();

		for(i = 0; i < numItems; i++)
		{
			DWORD dwVal = OptionsData::GetInstance().GetFullItemVal(i);
			DWORD dwDval = OptionsData::GetInstance().GetFullItemDefVal(i);
			PCHAR name = OptionsData::GetInstance().GetFullItemName(i);
			DWORD type = OptionsData::GetInstance().GetFullItemType(i);
			BOOL ext = OptionsData::GetInstance().GetIsFullItemExternal(i);
			switch(type)
			{
				case DL_OPT_TYPE_BOOL:
					bVal = (dwVal&1);
					bDval = (dwDval&1);
					if(ext)
						ini.SetBoolValue("Externals", name, bVal);
					else
						ini.SetBoolValue("Settings", name, bVal);
					break;
				case DL_OPT_TYPE_WORDREGION:
					if(ext)
						ini.SetLongValue("Externals", name, dwVal, 0, TRUE);
					else
						ini.SetLongValue("Settings", name, dwVal, 0, TRUE);
					break;
				case DL_OPT_TYPE_WORD:
				case DL_OPT_TYPE_WORDPORT:
				case DL_OPT_TYPE_DWORDTIME:
					if(ext)
						ini.SetLongValue("Externals", name, dwVal);
					else
						ini.SetLongValue("Settings", name, dwVal);
					break;
				case DL_OPT_TYPE_DWORD:
					sprintf(pathTemp, "0x%08x", dwVal);
					if(ext)
						ini.SetValue("Externals", name, pathTemp);
					else
						ini.SetValue("Settings", name, pathTemp);
					break;
				case DL_OPT_TYPE_PATH:
				case DL_OPT_TYPE_PATHQLB:
				case DL_OPT_TYPE_PATHPLUGIN:
					pkey = (pkeydata)dwVal;
					if(pkey != NULL)
					{
						if(pkey->launchpath[0] != 0)
						{
							mnt = dlDrives::GetInstance().GetDriveMount(pkey->dev);
							sprintf(pathTemp, "%s%s", mnt, pkey->launchpath);
							if(ext)
								ini.SetLongValue("Externals", name, dwVal);
							else
							{
								if(type == DL_OPT_TYPE_PATHPLUGIN)
									ini.SetValue("Plugins", name, pathTemp);
								else
									ini.SetValue("Paths", name, pathTemp);
							}
						}
					}
					break;
				default:
					lDbgPrint("saving type %d to ini struct failed!\n", type);
					break;
			}
		}
		if(XboxUtil::GetInstance().IsFileExist(INIPATH))
			DeleteFileA(INIPATH);
		// write to the desired disk
		if(dlDrives::GetInstance().IsDriveFlash(idx))
		{
			string cst;
			ini.Save(cst);
			//lDbgPrint("string len: %d cstr len %d\n", cst.size(), strlen(cst.c_str()));
			Nand::GetInstance().WriteFileToFlash((PBYTE)cst.c_str(), "launch.ini", cst.size());
		}
		else
			ini.SaveFile(INIPATH);
		XboxUtil::GetInstance().DeleteLink(INIDRIVEMOUNT);
		ini.Reset();
		wsprintfW(infoText, L"%s %S\\launch.ini", Strings::GetInstance().Lookup(L"ini_savefile"), INI_GET_FRIENDLY(idx));
		m_infoBox.SetText(infoText);
		//lDbgPrint("write file done\n");
		RefreshList();
	}
}

VOID IniData::ReadFromFile(DWORD idx)
{
	if(DashLaunch::GetInstance().canUseImports())
	{
		XboxUtil::GetInstance().MountPath(INIDRIVEMOUNT, INI_GET_MOUNTPOINT(idx));
		if(XboxUtil::GetInstance().IsFileExist(INIPATH))
		{
			WCHAR infoText[80];
			//lDbgPrint("loading file\n");
			DashLaunch::GetInstance().dlaunchForceIniLoad(INI_GET_MOUNTPOINT(idx));
			m_OverrideIni = idx;
			RefreshList();
			wsprintfW(infoText, L"%s %S\\launch.ini", Strings::GetInstance().Lookup(L"ini_loadfile"), INI_GET_FRIENDLY(idx));
			m_infoBox.SetText(infoText);
		}
		XboxUtil::GetInstance().DeleteLink(INIDRIVEMOUNT);
	}
}

VOID IniData::RemoveFile(DWORD idx)
{
	XboxUtil::GetInstance().MountPath(INIDRIVEMOUNT, INI_GET_MOUNTPOINT(idx));
	if(XboxUtil::GetInstance().IsFileExist(INIPATH))
	{
		WCHAR infoText[80];
		if(DeleteFileA(INIPATH))
			wsprintfW(infoText, L"%s %S\\launch.ini", Strings::GetInstance().Lookup(L"ini_delfile"), INI_GET_FRIENDLY(idx));
		else
			wsprintfW(infoText, L"%s %S\\launch.ini (0x%08x)", Strings::GetInstance().Lookup(L"ini_delerr"), INI_GET_FRIENDLY(idx), GetLastError());
		m_infoBox.SetText(infoText);
		RefreshList();
	}
	XboxUtil::GetInstance().DeleteLink(INIDRIVEMOUNT);
}
