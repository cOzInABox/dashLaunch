#pragma once
#include <xtl.h>
#include <string>
#include <cstring>
#include <vector>
#include <xui.h>
#include <xuiapp.h>
#include "xkelib.h"
#include "..\_common.h"

using namespace std;

#define MAX_RECURSE_DEPTH		3
#define FILER_SEARCH_PATH		0 // folder browse
#define FILER_SEARCH_XEX		1 // xex browse
#define FILER_SEARCH_TITLE		2 // launchable xex/con
#define FILER_SEARCH_FILEPATH	4 // new file create dialog in a path

#define ELF_HEADER				0x7F454C46 // ' ELF'

// Information for one list item.
typedef struct _FILER_ITEM_INFO {
	string itemPath;
	wstring witemPath;
	wstring showName;
	wstring sizeInfo;
	PBYTE iconData;
	DWORD iconSize;
	DWORD device;
	BOOL isXex; // if it's not a dir and not a xex, it's a con or something else entirely
	BOOL isElf;
	BOOL isDir;
	BOOL fEnabled;
} FILER_ITEM_INFO, *PFILER_ITEM_INFO;

class FilerData
{
public:	
	static FilerData& GetInstance(){static FilerData singleton; return singleton;}
	static vector<FILER_ITEM_INFO> m_finfo;

	DWORD GetItemCount(VOID) {return m_finfo.size();}

	VOID Init(int devNum, char* curPath, DWORD type, BOOL allowFlash = TRUE);
	PWCHAR GetItemWShowName(DWORD item) {return (PWCHAR)m_finfo.at(item).showName.c_str();}
	PWCHAR GetItemWSize(DWORD item) {return (PWCHAR)m_finfo.at(item).sizeInfo.c_str();}
	BOOL GetItemEnable(DWORD item) {return m_finfo.at(item).fEnabled;}
	BOOL GetItemIsdir(DWORD item) {return m_finfo.at(item).isDir;}
	BOOL GetItemIsXex(DWORD item) {return m_finfo.at(item).isXex;}
	BOOL GetItemIsElf(DWORD item) {return m_finfo.at(item).isElf;}
	PBYTE GetItemIcon(DWORD item, PDWORD size);

	BOOL ItemSelect(DWORD item);
	VOID UpDir(VOID);
	DWORD GetCurDev(VOID) {return m_currentDev;}
	PCHAR GetCurItem(VOID){return (PCHAR)m_curItem.c_str();}
	PCHAR GetCurPath(VOID){return (PCHAR)m_curPath.c_str();}
	BOOL IsRoot(VOID){return m_IsRoot;}
	BOOL IsCurItemXex(VOID){return m_curItemIsXex;}
	BOOL IsCurItemElf(VOID){return m_curItemIsElf;}
	VOID RefreshIfRoot(VOID);
	VOID SetSearchMode(DWORD mode) {m_searchMode = mode;}
	BOOL IsSearchXex(VOID) {return (m_searchMode == FILER_SEARCH_XEX);}
	BOOL IsSearchXexCon(VOID) {return (m_searchMode == FILER_SEARCH_TITLE);}
	BOOL IsSearchPath(VOID) {return (m_searchMode == FILER_SEARCH_PATH);}
	BOOL IsCreateFile(VOID) {return (m_searchMode == FILER_SEARCH_FILEPATH);}

	// scene controls
	VOID SetListControl(CXuiList lst){m_localList = lst;}
	VOID SetPathBoxControl(CXuiTextElement box){m_pathBox = box;}
	VOID SetInfoBoxControl(CXuiTextElement box){m_infoBox = box;}
	CXuiTextElement GetPathBox(VOID) {return m_pathBox;}

private:
	BOOL IsConLaunchable(DWORD type, DWORD tid);
	VOID AddItem(char* item, DWORD device, DWORD lowBytes, DWORD hiBytes, BOOL isDir, BOOL isXex = FALSE, BOOL isFakeDir = FALSE, BOOL isRecurse = FALSE, BOOL isElf = FALSE);
	VOID AddItemCon(char* item, const char* path, DWORD device, DWORD lowBytes, DWORD hiBytes);
	VOID UpdateList(VOID);
	VOID CleanupList(VOID);
	VOID UpdateCurPath(VOID);
	VOID FakeRoot(VOID);
	DWORD IsXex(const char* filePath);
	DWORD IsXexConElf(const char* filePath, BOOL isRecurse = FALSE);
	BOOL IsFilteredContentPath(const char* basePath);
	BOOL IsContentPath(const char* scanpath);
	BOOL IsIndiePath(const char* scanpath);
	BOOL RecurseSearch(const char* basePath, DWORD curDepth);
	VOID SetFilters(DWORD type, BOOL allowFlash);

	CXuiList m_localList;
	CXuiTextElement m_pathBox;
	CXuiTextElement m_infoBox;
	DWORD m_searchMode;
	BOOL m_AllowIndie;
	string m_curPath;
	wstring m_recurseFirstFound;
	string m_curMount;
	string m_curItem;
	BOOL m_curItemIsXex;
	BOOL m_curItemIsElf;
	BOOL m_allowFlash;
	BOOL m_allowElf;

	DWORD m_currentDev;
	BOOL m_IsRoot;

	FilerData();
	~FilerData() {}
	FilerData(const FilerData&);                 // Prevent copy-construction
	FilerData& operator=(const FilerData&);      // Prevent assignment
};
