#include "filerList.h"
#include "dashLaunch.h"
#include "stdio.h"
#include "Resource.h"
#include "dlDrives.h"
#include "XboxUtil.h"
#include "optionsData.h"
#include "SimpleIni.h"
#include "logging.h"

#define FILER_XEX	1
#define FILER_CON	2
#define FILER_ELF	3

#define XCONTENT_ISLAUNCHABLE(x) ( \
	(x==XCONTENTTYPE_INSTALLED_XBOX360TITLE)|| \
	(x==XCONTENTTYPE_XBOXTITLE)|| \
	(x==XCONTENTTYPE_XBOX360TITLE)|| \
	(x==XCONTENTTYPE_GAMEDEMO)|| \
	(x==XCONTENTTYPE_GAMETITLE)|| \
	(x==XCONTENTTYPE_ARCADE) \
	) 

vector<FILER_ITEM_INFO> FilerData::m_finfo;

FilerData::FilerData(void)
{
	m_curPath.clear();
	m_curMount.clear();
	m_finfo.clear();
}

VOID FilerData::CleanupList(VOID)
{
	DWORD i;
	int cnt = m_finfo.size();
	m_localList.DeleteItems(0, GetItemCount());
	for(i = 0; i < m_finfo.size(); i++)
	{
		if(m_finfo.at(i).iconData)
			delete [] m_finfo.at(i).iconData;
	}
	m_finfo.clear();
}

PBYTE FilerData::GetItemIcon(DWORD item, PDWORD size)
{
	*size = m_finfo.at(item).iconSize;
	if(*size != 0)
	{
		return m_finfo.at(item).iconData;
	}
	return NULL;
}

BOOL FilerData::IsConLaunchable(DWORD type, DWORD tid)
{
	BOOL ret = FALSE;
	if(type == XCONTENTTYPE_MARKETPLACE)
	{
		if(tid == 0x584E07D2)
		{
			ret = TRUE;
		}
	}
	else if(XCONTENT_ISLAUNCHABLE(type))
	{
		ret = TRUE;
	}
	return ret;
}

VOID FilerData::AddItem(char* item, DWORD device, DWORD lowBytes, DWORD hiBytes, BOOL isDir, BOOL isXex, BOOL isFakeDir, BOOL isRecurse, BOOL isElf)
{
	FILER_ITEM_INFO linfo;
	int iNum = m_finfo.size();
	linfo.itemPath = item;
	linfo.showName = XboxUtil::GetInstance().GetWstring(linfo.itemPath);
	linfo.witemPath = XboxUtil::GetInstance().GetWstring(linfo.itemPath);
	linfo.isDir = isDir;
	linfo.isXex = isXex;
	linfo.isElf = isElf;
	linfo.fEnabled = TRUE;
	linfo.device = device;
	linfo.iconData = NULL;
	linfo.iconSize = 0;
	if((isDir)&&(!isFakeDir))
	{
		if(isRecurse)
			linfo.sizeInfo = m_recurseFirstFound.c_str();
		else
			linfo.sizeInfo = L"";
	}
	else
		linfo.sizeInfo = XboxUtil::GetInstance().BytesToWstring(lowBytes, hiBytes);
	m_finfo.push_back(linfo);
	//lDbgPrint("inserting non-con %s at %d\n", item, iNum);
	m_localList.InsertItems(iNum, 1);
}

VOID FilerData::AddItemCon(char* item, const char* path, DWORD device, DWORD lowBytes, DWORD hiBytes)
{
	FILER_ITEM_INFO linfo;
	int iNum = m_finfo.size();
	HANDLE fHand = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	linfo.showName.clear();
	linfo.iconData = NULL;
	linfo.iconSize = 0;
	if(fHand != INVALID_HANDLE_VALUE)
	{
		XCONTENT_HEADER xhdr;
		XCONTENT_METADATA xmeta;
		DWORD bRead;
		ReadFile(fHand, &xhdr, sizeof(XCONTENT_HEADER), &bRead, NULL);
		if(bRead == sizeof(XCONTENT_HEADER))
		{
			ReadFile(fHand, &xmeta, sizeof(XCONTENT_METADATA), &bRead, NULL);
			if(bRead == sizeof(XCONTENT_METADATA))
			{
				if(IsConLaunchable(xmeta.ContentType, xmeta.ExecutionId.TitleID))
				{
					int i;
					BOOL nameFound = FALSE; // TODO: should this default to the console's region for first check??
					for(i = 0; i < XCONTENT_METADATA_LANG_MAX; i++)
					{
						if(xmeta.DisplayName[i][0] != 0)
						{
							linfo.showName = xmeta.DisplayName[i];
							nameFound = TRUE;
							i = XCONTENT_METADATA_LANG_MAX;
						}
					}
					if(nameFound == FALSE)
					{
						for(i = 0; i < XCONTENT_METADATA_LANG_EX_MAX; i++)
						{
							if(xmeta.DisplayNameEx[i][0] != 0)
							{
								linfo.showName = xmeta.DisplayNameEx[i];
								nameFound = TRUE;
								i = XCONTENT_METADATA_LANG_EX_MAX;
							}
						}
						if(nameFound == FALSE)
						{
							if(xmeta.TitleName[0] != 0)
								linfo.showName = xmeta.TitleName;
							else
								linfo.showName = L"Unable To Find Name!!";
						}
					}
					if(xmeta.ThumbnailSize != 0)
					{
						linfo.iconSize = xmeta.ThumbnailSize;
						linfo.iconData = new BYTE[linfo.iconSize];
						//lDbgPrint("adding icon sz %x\n", linfo.iconSize);
						memcpy(linfo.iconData, xmeta.Thumbnail, linfo.iconSize);
						linfo.itemPath = item;
						linfo.witemPath = XboxUtil::GetInstance().GetWstring(linfo.itemPath);
						linfo.isDir = FALSE;
						linfo.fEnabled = TRUE;
						linfo.isXex = FALSE;
						linfo.isElf = FALSE;
						linfo.device = device;
						linfo.sizeInfo = XboxUtil::GetInstance().BytesToWstring(lowBytes, hiBytes);
						m_finfo.push_back(linfo);
						//lDbgPrint("inserting container %s at %d\n", item, iNum);
						m_localList.InsertItems(iNum, 1);					
					}
				}
			}
		}
		CloseHandle(fHand);
	}
}

bool nameDirSortCompare(const FILER_ITEM_INFO &left, const FILER_ITEM_INFO &right)
{
	wstring::const_iterator lit, lite, rit, rite;
	int lsz, rsz;
	if(left.sizeInfo.size()){
		lit = left.sizeInfo.begin();
		lite = left.sizeInfo.end();
		lsz = left.sizeInfo.size();
	}
	else{
		lit = left.witemPath.begin();
		lite = left.witemPath.end();
		lsz = left.witemPath.size();
	}
	if(right.sizeInfo.size()){
		rit = right.sizeInfo.begin();
		rite = right.sizeInfo.end();
		rsz = right.sizeInfo.size();
	}
	else{
		rit = right.witemPath.begin();
		rite = right.witemPath.end();
		rsz = right.witemPath.size();
	}
	for(; (lit != lite) && (rit != rite); ++lit, ++rit)
	{
		if(towlower(*lit) < towlower(*rit))
			return true;
		else if(towlower(*lit) > towlower(*rit))
			return false;
	}
	if(lsz < rsz)
		return true;
	return false;
}

bool showNameSortCompare(const FILER_ITEM_INFO &left, const FILER_ITEM_INFO &right)
{
	for(wstring::const_iterator lit = left.showName.begin(), rit = right.showName.begin(); (lit != left.showName.end()) && (rit != right.showName.end()); ++lit, ++rit)
	{
		if(towlower(*lit) < towlower(*rit))
			return true;
		else if(towlower(*lit) > towlower(*rit))
			return false;
	}
	if(left.showName.size() < right.showName.size())
		return true;
	return false;
}

bool dirSortCompare(const FILER_ITEM_INFO &left, const FILER_ITEM_INFO &right)
{
	return left.isDir > right.isDir;
}

// finds dll xex
DWORD FilerData::IsXex(const char* filePath)
{
	//lDbgPrint("IsXex %s\n", filePath);
	HANDLE fHand = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fHand != INVALID_HANDLE_VALUE)
	{
		DWORD head[2];
		DWORD bRead;
		ReadFile(fHand, head, 8, &bRead, NULL);
		CloseHandle(fHand);
		if(head[0] == 'XEX2')
		{
			if((head[1]&XEX_MODULE_FLAG_TITLE_PROCESS) == 0)
			{
				if(head[1]&XEX_MODULE_FLAG_DLL)
				{
					return FILER_XEX;
				}
			}
		}
	}
	return 0;
}

// finds launchable xex and con
DWORD FilerData::IsXexConElf(const char* filePath, BOOL isRecurse)
{
	//lDbgPrint("IsXexConElf %s\n", filePath);
	DWORD ret = 0;
	HANDLE fHand = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fHand != INVALID_HANDLE_VALUE)
	{
		DWORD headBytes[2];
		DWORD bRead = 0;
		ReadFile(fHand, headBytes, 8, &bRead, NULL);
		//lDbgPrint("head: %08x\n", headBytes[0]);
		if(headBytes[0] == 'XEX2')
		{
			if(headBytes[1]&XEX_MODULE_FLAG_TITLE_PROCESS) // restrict to title xex
			{
				if((headBytes[1]&XEX_MODULE_FLAG_DLL) == 0)
				{
					ret = FILER_XEX;
				}
			}
		}
		else if ((headBytes[0] == 'LIVE')||(headBytes[0] == 'CON ')||(headBytes[0] == 'PIRS')) // content type 4 bytes at 0x344
		{
			BOOL checkIndie = FALSE;
			if(m_AllowIndie)
				checkIndie = IsIndiePath(filePath);
			if(SetFilePointer(fHand, 0x344, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER)
			{
				ReadFile(fHand, headBytes, 4, &bRead, NULL);
				if(checkIndie)
				{
					if(SetFilePointer(fHand, 0x360, NULL, FILE_BEGIN) != 0)
					{
						ReadFile(fHand, &headBytes[1], 4, &bRead, NULL);
						if(IsConLaunchable(headBytes[0], headBytes[1]))
						{
							ret = FILER_CON;
						}
					}
				}
				else if(IsConLaunchable(headBytes[0], 0))
				{
					ret = FILER_CON;
				}
				if(isRecurse&&(ret == FILER_CON))
				{
					if(SetFilePointer(fHand, 0x411, NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER)
					{
						WCHAR twbuf[0x80];
						ReadFile(fHand, twbuf, 0x100, &bRead, NULL);
						m_recurseFirstFound = twbuf;
					}
				}
			}
		}
		else
		{
			if(m_allowElf)
			{
				if(headBytes[0] == ELF_HEADER)
				{
					ret = FILER_ELF;
				}
			}
		}
		CloseHandle(fHand);
	}
	//else
	//	lDbgPrint("could not open file %s\n", filePath);
	return ret;
}

BOOL FilerData::IsFilteredContentPath(const char* scanpath)
{
	string filterPath = m_curMount+"\\Content\\";
	//lDbgPrint("checking %s against %s ", scanpath, filterPath.c_str());
	if(strncmp(scanpath, filterPath.c_str(), filterPath.size()) == 0)
	{
		filterPath += "0000000000000000";
		if(strncmp(scanpath, filterPath.c_str(), filterPath.size()) != 0)
		{
			//lDbgPrint("TRUE\n");
			return TRUE;
		}
	}
	//lDbgPrint("FALSE\n");
	return FALSE;
	//string contRoot = contPath + "\\*";
}

BOOL FilerData::IsContentPath(const char* scanpath)
{
	string conPath = m_curMount+"\\Content\\0000000000000000\\";
	if(strncmp(scanpath, conPath.c_str(), conPath.size()) == 0)
		return TRUE;
	return FALSE;
}

BOOL FilerData::IsIndiePath(const char* scanpath)
{ // Usb0 Usb1 Usb2 no indie on mass devices
	if(strncmp(m_curMount.c_str(), "Usb", 3) != 0)
	{
		string conPath = m_curMount+"\\Content\\0000000000000000\\584E07D2\\00000002";
		//lDbgPrint("indie check %s to %s\n", scanpath, conPath.c_str());
		if(strncmp(scanpath, conPath.c_str(), conPath.size()) == 0)
			return TRUE;
	}
	return FALSE;
}

BOOL FilerData::RecurseSearch(const char* basePath, DWORD curDepth)
{
	if(!IsFilteredContentPath(basePath))
	{
		WIN32_FIND_DATA findData;
		BOOL hasDirs = FALSE;
		DWORD depth = curDepth+1;
		memset(&findData, 0, sizeof(WIN32_FIND_DATA));
		string fpath = basePath;
		fpath += "\\*";
		HANDLE hFind = FindFirstFile(fpath.c_str(), &findData);
		//lDbgPrint("recurse lv %d '%s'\n", depth, fpath.c_str());
		if (hFind == INVALID_HANDLE_VALUE)
			return FALSE;
		do {
			BOOL isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			if(!isDir)
			{
				if((findData.nFileSizeLow > 0x100)||(findData.nFileSizeHigh != 0)) // valid xex or con will always have a size larger than this
				{
					string tpath = basePath;
					tpath += "\\";
					tpath += findData.cFileName;
					if(IsSearchXex())
					{
						m_recurseFirstFound.clear();
						if(IsXex(tpath.c_str()) == FILER_XEX)
						{
							FindClose(hFind);
							return TRUE;
						}
					}
					else
					{
						if(IsXexConElf(tpath.c_str(), TRUE)) // ends the find on item discovery
						{
							FindClose(hFind);
							return TRUE;
						}
					}
				}
			}
			else
				hasDirs = TRUE;
		} while (FindNextFile(hFind, &findData));
		FindClose(hFind);
		// if we got here nothing valid was found... check dirs if there were any
		if((hasDirs))
		{
			if(depth < MAX_RECURSE_DEPTH)
			{
				memset(&findData, 0, sizeof(WIN32_FIND_DATA));
				HANDLE hFind = FindFirstFile(fpath.c_str(), &findData);
				if (hFind == INVALID_HANDLE_VALUE)
					return FALSE;
				do {
					if(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						string checkpath = basePath;
						checkpath += "\\";
						checkpath += findData.cFileName;
						if(RecurseSearch(checkpath.c_str(), depth))
						{
							FindClose(hFind);
							return TRUE;
						}
					}
				} while (FindNextFile(hFind, &findData));
				FindClose(hFind);
			}
			else
				return TRUE;
		}
	}
	return FALSE;
}

// m_curPath = "" on drive root!!!
VOID FilerData::UpdateList(VOID)
{
	WIN32_FIND_DATA findData;
	DWORD dirCount = 1;
	memset(&findData, 0, sizeof(WIN32_FIND_DATA));
	string fpath = m_curMount + m_curPath + "\\*";
	//lDbgPrint("updatelist fpath %s\n", fpath.c_str());
	AddItem("..", m_currentDev, 0, 0, TRUE);
	HANDLE hFind = FindFirstFile(fpath.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE)
		return;
	do {
		BOOL isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		if(!isDir)
		{
			if(!IsSearchPath())
			{
				if((findData.nFileSizeLow > 0x100)||(findData.nFileSizeHigh != 0))
				{
					string tpath;
	// 				lDbgPrint("m_curMount: %s\n", m_curMount.c_str());
	// 				lDbgPrint("m_curPath : %s\n", m_curPath.c_str());
	// 				lDbgPrint("file      : %s\n", findData.cFileName);
					if(m_curPath.size() == 0)
						tpath = m_curMount+"\\"+m_curPath+findData.cFileName;
					else
						tpath = m_curMount+m_curPath+"\\"+findData.cFileName;
					if(IsSearchXexCon())
					{
						DWORD ttyp = IsXexConElf(tpath.c_str());
// 						lDbgPrint("%s is type %x\n", tpath.c_str(), ttyp);
						if(ttyp)
						{
							if(ttyp == FILER_XEX)
							{
								AddItem(findData.cFileName, m_currentDev, findData.nFileSizeLow, findData.nFileSizeHigh, FALSE, TRUE, FALSE, FALSE, FALSE);
							}
							else if(ttyp == FILER_ELF)
							{
								AddItem(findData.cFileName, m_currentDev, findData.nFileSizeLow, findData.nFileSizeHigh, FALSE, FALSE, FALSE, FALSE, TRUE);
							}
							else
							{
								AddItemCon(findData.cFileName, tpath.c_str(), m_currentDev, findData.nFileSizeLow, findData.nFileSizeHigh);
							}
						}
					}
					else if(IsSearchXex())
					{
						if(IsXex(tpath.c_str()))
							AddItem(findData.cFileName, m_currentDev, findData.nFileSizeLow, findData.nFileSizeHigh, FALSE, TRUE);
					}
					else
					{
						if(IsXexConElf(tpath.c_str()) == 0) // everything but launchables
							AddItem(findData.cFileName, m_currentDev, findData.nFileSizeLow, findData.nFileSizeHigh, FALSE, FALSE, FALSE, FALSE, FALSE);
					}
				}
			}
		}
		else
		{
			if(IsSearchXexCon()||IsSearchXex()) // if it's filtered only show dirs that have a xex or con in them
			{
				string checkpath = m_curMount + m_curPath + "\\" + findData.cFileName;
				m_recurseFirstFound.clear();
				if((IsContentPath(checkpath.c_str()))&&(strcmp(findData.cFileName, "584E07D2") == 0))
				{
					if(m_AllowIndie)
					{
						if(RecurseSearch(checkpath.c_str(), 0))
						{
							m_recurseFirstFound = L"      XBLIG";
							AddItem(findData.cFileName, m_currentDev, findData.nFileSizeLow, findData.nFileSizeHigh, isDir, FALSE, FALSE, TRUE);
							dirCount++;
						}
					}
				}
				else if(RecurseSearch(checkpath.c_str(), 0))
				{
					AddItem(findData.cFileName, m_currentDev, findData.nFileSizeLow, findData.nFileSizeHigh, isDir, FALSE, FALSE, TRUE);
					dirCount++;
				}
			}
			else
			{
				AddItem(findData.cFileName, m_currentDev, findData.nFileSizeLow, findData.nFileSizeHigh, isDir);
				dirCount++;
			}
		}
	} while (FindNextFile(hFind, &findData));
	FindClose(hFind);
	if(dirCount > 1)
	{
		sort(m_finfo.begin()+1, m_finfo.end(), dirSortCompare);
		sort(m_finfo.begin()+1, m_finfo.begin()+dirCount, nameDirSortCompare);
	}
	sort(m_finfo.begin()+dirCount, m_finfo.end(), showNameSortCompare);
}

VOID FilerData::FakeRoot(VOID)
{
	DWORD i;
	PCHAR name;
	//lDbgPrint("fakeroot checking %d devices\n", dlDrives::GetInstance().GetNumDrives());
	for(i = 0; i < dlDrives::GetInstance().GetNumDrives(); i++)
	{
		name = NULL;
		if(m_allowFlash == FALSE)
		{
			if(!dlDrives::GetInstance().IsDriveFlash(i))
				name = dlDrives::GetInstance().MountDrive(i);
		}
		else
		{
			name = dlDrives::GetInstance().MountDrive(i);
		}
		if(name)
		{
			DWORD low, hi;
			dlDrives::GetInstance().GetDriveSize(i, &low, &hi);
			//lDbgPrint("adding %s\n", name);
			AddItem(name, i, low, hi, TRUE, FALSE, TRUE);
		}
	}
}

VOID FilerData::UpdateCurPath(VOID)
{
	m_IsRoot = ((m_curMount.size() == 0)&&(m_curPath.size() == 0));
	if(!m_IsRoot)
	{
		wstring drive = XboxUtil::GetInstance().GetWstring(m_curMount);
		wstring path = XboxUtil::GetInstance().GetWstring(m_curPath);
		wstring cur = drive+path;
		//lDbgPrint("drive '%S' path '%S' cur '%S'\n", drive.c_str(), path.c_str(), cur.c_str());
		m_pathBox.SetText(cur.c_str());
	}
	else
		m_pathBox.SetText(L"");

	CleanupList();
	if(m_IsRoot)
		FakeRoot();
	else
		UpdateList();
}

VOID FilerData::SetFilters(DWORD type, BOOL allowFlash)
{
	m_allowFlash = allowFlash;
	m_AllowIndie = FALSE;
	m_allowElf = FALSE;
	switch(type)
	{
		case FILER_SEARCH_PATH: // folder
			m_searchMode = FILER_SEARCH_PATH;
			break;
		case DL_OPT_TYPE_PATH: // path to a folder -> new file dialog
			m_searchMode = FILER_SEARCH_FILEPATH;
			break;
		case DL_OPT_TYPE_PATHQLB: // xex/con
			m_searchMode = FILER_SEARCH_TITLE;
			break;
		case DL_OPT_TYPE_PATHPLUGIN: // dll xex only
			m_searchMode = FILER_SEARCH_XEX;
			break;
		case DL_OPT_TYPE_ALLEXEC:
			m_searchMode = FILER_SEARCH_TITLE;
			m_AllowIndie = TRUE;
			m_allowElf = TRUE;
			break;
	}
}

VOID FilerData::Init(int devNum, char* curPath, DWORD type, BOOL allowFlash)
{
	CleanupList();
	m_curPath.clear();
	m_curMount.clear();
	if(devNum != INVALID_ITEM)
	{
		//lDbgPrint("Filer init dev %d path %s\n", devNum, curPath);
		PCHAR name = dlDrives::GetInstance().MountDrive(devNum);
		if(name != NULL)
		{
			m_curMount = name;
			if(curPath != NULL)
			{
				m_curPath = curPath;
				if(m_curPath.size() != 0)
				{
					int pos = m_curPath.find_last_of('\\');
					if(pos != -1)
						m_curPath.erase(pos);
				}
				m_currentDev = devNum;
			}
		}
	}
	SetFilters(type, allowFlash);
	UpdateCurPath();
}

VOID FilerData::UpDir(VOID)
{
	if(!m_IsRoot)
	{
		if(m_curPath.size() != 0)
		{
			int pos = m_curPath.find_last_of('\\');
			//lDbgPrint("updir curpath: %s\n", m_curPath.c_str());
			m_curPath.erase(pos);
		}
		else
			m_curMount.clear();
		UpdateCurPath();
		m_localList.SetCurSel(0);
	}
}

BOOL FilerData::ItemSelect(DWORD item)
{
	if(m_IsRoot) // device select
	{
		m_curMount = m_finfo.at(item).itemPath;
		m_currentDev = m_finfo.at(item).device;
		UpdateCurPath();
		m_localList.SetCurSel(0);
		return FALSE;
	}
	else
	{
		if(m_finfo.at(item).isDir)
		{
			if(strcmp(m_finfo.at(item).itemPath.c_str(), "..") == 0)
			{
				UpDir();
			}
			else
			{
				m_curPath += "\\";
				m_curPath += m_finfo.at(item).itemPath;
				UpdateCurPath();
				m_localList.SetCurSel(0);
			}
			return FALSE;
		}
	}
	m_curItem = m_curPath + "\\";
	m_curItem += m_finfo.at(item).itemPath;
	m_curItemIsXex = m_finfo.at(item).isXex;
	m_curItemIsElf = m_finfo.at(item).isElf;
	return TRUE;
}

VOID FilerData::RefreshIfRoot(VOID)
{
	if(m_IsRoot)
		UpdateCurPath();
}

