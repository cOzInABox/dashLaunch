#include "optionsData.h"
#include "dashLaunch.h"
#include "stdio.h"
#include "Resource.h"
#include "dlDrives.h"
#include "Strings.h"
#include "XboxUtil.h"
#include "logging.h"

static WCHAR catLookup[OPT_CAT_MAX][24] = {
	L"opt_fil_all",		// min or all
	L"opt_fil_paths",	// OPT_CAT_PATHS
	L"opt_fil_behavior",// OPT_CAT_BEHAVIOR
	L"opt_fil_net",		// OPT_CAT_NETWORK
	L"opt_fil_timer",	// OPT_CAT_TIMERS
	L"opt_fil_plugins",	// OPT_CAT_PLUGINS
	L"opt_fil_extern",  // OPT_CAT_EXTERNAL
};

static WCHAR enableText[MAX_PATH];
static WCHAR disableText[MAX_PATH];
static WCHAR portText[MAX_PATH];

vector<OPTS_SHOWN> OptionsData::m_optsShow;
vector<OPTS_LIST> OptionsData::m_optsList;

OptionsData::OptionsData(void)
{
	PWCHAR infolist = Resource::GetInstance().GetFilePath(L"optionInfo.xus");
	if(XuiLoadStringTableFromFile(infolist, &hInfo) != S_OK)
		hInfo = (HXUISTRINGTABLE)INVALID_HANDLE_VALUE;

	InitShowOptsList();
	m_optsList.clear();
	LPCWSTR lp = Strings::GetInstance().Lookup(L"enable");
	wsprintfW(enableText, L"%s      ", lp);
// 	lDbgPrint("enable : '%S' to '%S'\n", lp, enableText);
	lp = Strings::GetInstance().Lookup(L"disable");
	wsprintfW(disableText, L"%s      ", lp);
// 	lDbgPrint("disable: '%S' to '%S'\n", lp, disableText);
	lp = Strings::GetInstance().Lookup(L"port");
	wcscpy(portText, lp);
// 	lDbgPrint("port: '%S'\n", portText);
	delete [] infolist;
}

// inserts the categories for the first time
VOID OptionsData::InitShowOptsList(VOID)
{
	DWORD i;
	OPTS_SHOWN opm;
	m_optsShow.clear();
	opm.listItem = 0xFFFFFFFF;
	opm.optType = 0xFFFFFFFF;
	for(i = 1; i < OPT_CAT_MAX; i++)
	{
		opm.optCategory = i|OPT_IS_CATEGORY;
		opm.wname = Strings::GetInstance().Lookup(catLookup[i]);
		LPCWSTR inf = XuiLookupStringTable(hInfo, catLookup[i]);
		if(inf == NULL)
			inf = XuiLookupStringTable(hInfo, L"oopsie");
		if(inf == NULL)
			opm.wdesc = L"No Info!";
		else
			opm.wdesc = inf;
		m_optsShow.push_back(opm);
		m_localList.InsertItems(i, 1);
	}
}

VOID OptionsData::CleanShowOptsList(DWORD category, int sitem)
{
	int i;
	DWORD itemCat;
	for(i = m_optsShow.size()-1; i >= sitem; i--)
	{
		itemCat = m_optsShow.at(i).optCategory;
		if((itemCat & OPT_IS_CATEGORY)&&((category == OPT_IS_CATEGORY) || (i == sitem)))
		{
			m_optsShow.at(i).optCategory = m_optsShow.at(i).optCategory&~OPT_IS_CAT_OPEN;
		}
		else if((itemCat == category) || (category == OPT_IS_CATEGORY))
		{
			m_optsShow.erase(m_optsShow.begin()+i);
			m_localList.DeleteItems(i, 1);
		}
	}
}

VOID OptionsData::AddShowOptsList(DWORD category, int sitem)
{
	if(m_optsList.size())
	{
		OPTS_SHOWN lopt;
		DWORD cat = OPT_NONCAT_INFO(category);
		DWORD i;
		int inst = 0;
		m_optsShow.at(sitem).optCategory = m_optsShow.at(sitem).optCategory|OPT_IS_CAT_OPEN;
		for(i = 0; i < m_optsList.size(); i++)
		{
			if(m_optsList.at(i).optCategory == cat)
			{
				int itm = sitem+inst+1;
				SetShownInfo(&lopt, i);
				m_optsShow.insert(m_optsShow.begin()+itm, lopt);
				m_localList.InsertItems(sitem+inst, 1);
				sitem++;
			}
		}
	}
}

VOID OptionsData::InitTitleText(VOID)
{
	m_curTitle = Strings::GetInstance().Lookup(L"opt_title");
	m_titleText.SetText(m_curTitle.c_str());
}

VOID OptionsData::CleanupList(VOID)
{
	DWORD it = m_optsShow.size();
	CleanShowOptsList(OPT_IS_CATEGORY);
	m_optsList.clear();
	m_localList.SetCurSel(0);
	//m_localList.DeleteItems(0, it);
	m_infoBox.SetText(L"");
}

#define MAX_REGION_TXT	48
static WCHAR knownRegions[][MAX_REGION_TXT] = {
	L"0x%04x - (CUSTOM/UNKNOWN)", // 0
	L"0x00FF - (NTSC/US)", // 1
	L"0x01FE - (NTSC/JAP)", // 2
	L"0x01FF - (NTSC/JAP)", // 3
	L"0x01FC - (NTSC/KOR)", // 4
	L"0x0101 - (NTSC/HK)", // 5
	L"0x02FE - (PAL/EU)", // 6
	L"0x0201 - (PAL/AUS)", // 7
	L"0x7FFF - (DEVKIT/ALL)", // 8
};
static WCHAR custRegion[MAX_REGION_TXT];

PWCHAR OptionsData::GetWRegionFromVal(DWORD region)
{
	PWCHAR ret = NULL;
	switch(region&0xFFFF)
	{
		case 0x00FF: // 0x00FF NTSC/US
			ret = knownRegions[1];
			break;
		case 0x01FE: // 0x01FE NTSC/JAP
			ret = knownRegions[2];
			break;
		case 0x01FF: // 0x01FF NTSC/JAP
			ret = knownRegions[3];
			break;
		case 0x01FC: // 0x01FC NTSC/KOR
			ret = knownRegions[4];
			break;
		case 0x0101: // 0x0101 NTSC/HK
			ret = knownRegions[5];
			break;
		case 0x02FE: // 0x02FE PAL/EU
			ret = knownRegions[6];
			break;
		case 0x0201: // 0x0201 PAL/AUS
			ret = knownRegions[7];
			break;
		case 0x7fff: // 0x7fff DEVKIT
			ret = knownRegions[8];
			break;
		default:
			wsprintfW(custRegion, knownRegions[0], region&0xFFFF);
			ret = custRegion;
			break;
	}
	return ret;
}

VOID OptionsData::SetShownInfo(POPTS_SHOWN pshow, DWORD idx)
{
	pkeydata pkey;
	WCHAR wTemp[MAX_PATH];
	LPCWSTR inf;
	DWORD val = m_optsList.at(idx).optionVal;
	pshow->listItem = idx;
	pshow->optType = m_optsList.at(idx).optType;
	pshow->optCategory = m_optsList.at(idx).optCategory;
	pshow->wname = XboxUtil::GetInstance().GetWstring(m_optsList.at(idx).optName);
	inf = XuiLookupStringTable(hInfo, pshow->wname.c_str());
	if(inf == NULL)
		inf = XuiLookupStringTable(hInfo, L"oopsie");
	if(inf == NULL)
		pshow->wdesc = L"No Info!";
	else
		pshow->wdesc = inf;
	ZeroMemory(wTemp, MAX_PATH*sizeof(WCHAR));
	switch(pshow->optType)
	{
		case DL_OPT_TYPE_BOOL:
			if(val)
				pshow->wval = enableText;
			else
				pshow->wval = disableText;
			break;
		case DL_OPT_TYPE_DWORD:
			wsprintfW(wTemp, L"0x%08x", val);
			pshow->wval = wTemp;
			break;
		case DL_OPT_TYPE_WORD:
			wsprintfW(wTemp, L"%d", val);
			pshow->wval = wTemp;
			break;
		case DL_OPT_TYPE_WORDREGION:
			pshow->wval = GetWRegionFromVal(val);
			break;
		case DL_OPT_TYPE_WORDPORT:
			wsprintfW(wTemp, L"%s: %d", portText, val);
			pshow->wval = wTemp;
			break;
		case DL_OPT_TYPE_DWORDTIME:
			wsprintfW(wTemp, L"%ds", val);
			pshow->wval = wTemp;
			break;
		case DL_OPT_TYPE_PATH:
		case DL_OPT_TYPE_PATHQLB:
		case DL_OPT_TYPE_PATHPLUGIN:
			pkey = (pkeydata)val;
			if(pkey != NULL)
			{
				if(pkey->launchpath[0] != 0)
				{
					PCHAR mnt = dlDrives::GetInstance().GetDriveMount(pkey->dev);
					if(pkey->flags == INVALID_ITEM)
						swprintf(wTemp, MAX_OPTVAL_LEN, L"(N/A) %S%S", mnt, pkey->launchpath);
					else
						swprintf(wTemp, MAX_OPTVAL_LEN, L"%S%S", mnt, pkey->launchpath);
				}
				else
					wsprintfW(wTemp, L"(%s)", Strings::GetInstance().Lookup(L"none"));
				pshow->wval = wTemp;
			}
			break;
		default:
			lDbgPrint("SetShownInfo: inserted '%s' type: %d val: %x NO TYPE INFO!!!\n", m_optsList.at(idx).optName, m_optsList.at(idx).optType, m_optsList.at(idx).optionVal);
			break;
	}
}

VOID OptionsData::AddShownItem(DWORD idx)
{
	int iNum = m_optsShow.size();
	OPTS_SHOWN opt;
	SetShownInfo(&opt, idx);
	m_optsShow.push_back(opt);
	m_localList.InsertItems(iNum, 1);
}

VOID OptionsData::UpdateShownItem(DWORD showidx)
{
	SetShownInfo(&m_optsShow.at(showidx), m_optsShow.at(showidx).listItem);
}

// gets the master list
VOID OptionsData::FetchList(VOID)
{
	if(m_optsList.size() != 0)
		CleanupList();
	if(DashLaunch::GetInstance().canUseImports())
	{
		OPTS_LIST opt;
		int i, nitems;
		DashLaunch::GetInstance().dlaunchGetNumOpts(&nitems);
		//lDbgPrint("creating %d items\n", nitems);
		for(i = 0; i < nitems; i++)
		{
			//typedef int (*DLAUNCHGETOPTINFO)(int opt, PDWORD optType, PCHAR outStr, PDWORD currVal, PDWORD defValue, PDWORD optMask);
			DashLaunch::GetInstance().dlaunchGetOptInfo(i, &opt.optType, opt.optName, &opt.optionVal, &opt.defaultValue, &opt.optCategory);
			m_optsList.push_back(opt);
		}
	}
}

// updates the master list
VOID OptionsData::RefreshShownList(VOID)
{
	if((DashLaunch::GetInstance().canUseImports())&&(m_optsList.size() != 0))
	{
		DWORD dwTemp, i, litem;
		for(i = 0; i < m_optsShow.size(); i++)
		{
			litem = m_optsShow.at(i).listItem;
			if(litem != 0xFFFFFFFF)
			{
				//typedef BOOL (*DLAUNCHGETOPTVAL)(int opt, PDWORD val);
				DashLaunch::GetInstance().dlaunchGetOptVal(litem, &dwTemp);
				if((m_optsList.at(litem).optionVal != dwTemp)||(m_optsList.at(litem).optType >= DL_OPT_TYPE_MAX_ACCESS))
				{
					m_optsList.at(litem).optionVal = dwTemp;
					UpdateShownItem(i);
				}
			}
		}
	}
	else
		lDbgPrint("refresh called when imports not available or pItems is null!!\n");
}

VOID OptionsData::HandleCategory(int idx)
{
	if(m_optsList.size() != 0) // nothing to put in the categories
	{
		if(m_optsShow.at(idx).optCategory&OPT_IS_CAT_OPEN) // hide the categories options
			CleanShowOptsList(OPT_NONCAT_INFO(m_optsShow.at(idx).optCategory), idx);
		else // show the categories options
			AddShowOptsList(m_optsShow.at(idx).optCategory, idx);
	}
}

VOID OptionsData::ToggleBoolOption(int idx)
{
	DWORD tidx = m_optsShow.at(idx).listItem;
	DWORD val = FALSE;
	if(m_optsList.at(tidx).optionVal == 0)
		val = TRUE;
	DashLaunch::GetInstance().dlaunchSetOptVal(tidx, &val);
	RefreshShownList();
}

VOID OptionsData::SetValOption(int idx, DWORD val)
{
	DWORD tidx = m_optsShow.at(idx).listItem;
	DashLaunch::GetInstance().dlaunchSetOptVal(tidx, &val);
	RefreshShownList();
}

BOOL OptionsData::ValOptionIsHex(int idx)
{
	BOOL ret = FALSE;
	switch(m_optsShow.at(idx).optType)
	{
		case DL_OPT_TYPE_WORDREGION:
		case DL_OPT_TYPE_DWORD:
			ret = TRUE;
			break;
	}
	return ret;
}

BOOL OptionsData::PathGetInfo(int idx, PCHAR* path, PDWORD flags, PDWORD dev)
{
	DWORD tidx = m_optsShow.at(idx).listItem;
	BOOL ret = FALSE;
	pkeydata pkey;
	switch(m_optsShow.at(idx).optType)
	{
		case DL_OPT_TYPE_PATH:
		case DL_OPT_TYPE_PATHQLB:
		case DL_OPT_TYPE_PATHPLUGIN:
			pkey = (pkeydata)m_optsList.at(tidx).optionVal;
			if(pkey != NULL)
			{
				if(pkey->flags != INVALID_ITEM)
				{
					if(path)
						*path  = pkey->launchpath;
					if(flags)
						*flags = pkey->flags;
					if(dev)
						*dev = pkey->dev;
					ret = TRUE;
				}
			}
			else
				lDbgPrint("OptionsData::PathGetInfo failed to retrieve pkey data, idx 0x%x tidx 0x%x\n", idx, tidx);
			break;
		default:
			break;
	}
	return ret;
}

static CHAR backslash[] = "\\";
VOID OptionsData::PathSetInfoFromDir(int idx, PCHAR path, DWORD flags, DWORD dev)
{
	DWORD tidx = m_optsShow.at(idx).listItem;
	pkeydata pkey;
	//lDbgPrint("PathSetInfoFromDir dev: %d flag: %d path: %s\n", dev, flags, path);
	switch(m_optsShow.at(idx).optType)
	{
		case DL_OPT_TYPE_PATH: // filter for any, use text input
			pkey = (pkeydata)m_optsList.at(tidx).optionVal;
			if(pkey != NULL)
			{
				pkey->flags = flags;
				pkey->dev = dev;
				strcpy_s(pkey->launchpath, MAX_PATH, path);
				strcat_s(pkey->launchpath, MAX_PATH, "\\");
				strcat_s(pkey->launchpath, MAX_PATH, m_optsList.at(tidx).optName);
				strcat_s(pkey->launchpath, MAX_PATH, ".txt");
				//lDbgPrint("final out: %s\n", pkey->launchpath);
				RefreshShownList();
			}
			else
				lDbgPrint("OptionsData::PathSetInfoFromDir failed to retrieve pkey data, idx 0x%x tidx 0x%x\n", idx, tidx);
			break;
		//case DL_OPT_TYPE_PATHQLB: // filter for xex and container
		//case DL_OPT_TYPE_PATHPLUGIN: // filter for xex
		default:
			break;
	}
}

VOID OptionsData::PathSetInfo(int idx, PCHAR path, DWORD flags, DWORD dev)
{
	DWORD tidx = m_optsShow.at(idx).listItem;
	pkeydata pkey;
	PCHAR ppAth;
	if(strlen(path) == 0)
		ppAth = backslash;
	else
		ppAth = path;
	//lDbgPrint("PathSetInfo dev: %d flag: %d path: %s\n", dev, flags, path);
	switch(m_optsShow.at(idx).optType)
	{
		case DL_OPT_TYPE_PATH: // filter for any, use text input
		case DL_OPT_TYPE_PATHQLB: // filter for xex and container
		case DL_OPT_TYPE_PATHPLUGIN: // filter for xex
			pkey = (pkeydata)m_optsList.at(tidx).optionVal;
			if(pkey != NULL)
			{
				pkey->flags = flags;
				pkey->dev = dev;
				strcpy_s(pkey->launchpath, MAX_PATH, ppAth);
				RefreshShownList();
			}
			else
				lDbgPrint("OptionsData::PathSetInfo failed to retrieve pkey data, idx 0x%x tidx 0x%x\n", idx, tidx);
			break;
		default:
			break;
	}
}

VOID OptionsData::PathClear(int idx)
{
	DWORD tidx = m_optsShow.at(idx).listItem;
	pkeydata pkey;
	//lDbgPrint("dev: %d flag: %d path: %s\n", dev, flags, path);
	switch(m_optsShow.at(idx).optType)
	{
		case DL_OPT_TYPE_PATH: // filter for any, use text input
		case DL_OPT_TYPE_PATHQLB: // filter for xex and container
		case DL_OPT_TYPE_PATHPLUGIN: // filter for xex
			pkey = (pkeydata)m_optsList.at(tidx).optionVal;
			if(pkey != NULL)
			{
				pkey->flags = INVALID_ITEM;
				pkey->dev = INVALID_ITEM;
				ZeroMemory(pkey->launchpath, MAX_PATH);
				RefreshShownList();
			}
			else
				lDbgPrint("OptionsData::PathClear failed to retrieve pkey data, idx 0x%x tidx 0x%x\n", idx, tidx);
			break;
		default:
			break;
	}
}

VOID OptionsData::ValClear(int idx)
{
	DWORD tidx = m_optsShow.at(idx).listItem;
	DWORD val = FALSE;
	switch(m_optsShow.at(idx).optType)
	{
	case DL_OPT_TYPE_WORD: // filter for any, use text input
// 	case DL_OPT_TYPE_WORDREGION: // filter for xex and container
// 	case DL_OPT_TYPE_WORDPORT: // filter for xex
	case DL_OPT_TYPE_DWORD:
// 	case DL_OPT_TYPE_DWORDTIME:
		DashLaunch::GetInstance().dlaunchSetOptVal(tidx, &val);
		RefreshShownList();
		break;
	default:
		break;
	}
}
