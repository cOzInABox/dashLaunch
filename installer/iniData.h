#pragma once
#include <xtl.h>
#include <xui.h>
#include <xuiapp.h>
#include "xkelib.h"
#include "..\_common.h"

// Information for one list item.
typedef struct _INI_ITEM_INFO {
	WCHAR optStatus[64];
	WCHAR optValTxt[64];
	WCHAR wsize[64];
	WCHAR optPathTxt[MAX_PATH];
	BOOL isCurrentIni;
	BOOL hasIni;
	BOOL fEnabled; // if the disk isn't present it will be disabled in the list
} INI_ITEM_INFO, *PINI_ITEM_INFO;

// List data.
typedef struct _INI_DATA {
	int nItems;
	PINI_ITEM_INFO pItems;
} INI_DATA, *PINI_DATA;

class IniData
{
public:	
	static IniData& GetInstance(){static IniData singleton; return singleton;}

	INI_DATA m_ListData;
	VOID FetchList(VOID);
	VOID RefreshList(VOID);
	int GetNumItems(){return m_ListData.nItems;}
	PINI_DATA GetListInfo(){return &m_ListData;}
	VOID WriteToFile(DWORD idx);
	VOID ReadFromFile(DWORD idx);
	VOID RemoveFile(DWORD idx);

	// scene controls
	VOID SetListControl(CXuiList lst){m_localList = lst;}
	VOID SetPathBoxControl(CXuiTextElement box){m_pathBox = box;}
	VOID SetInfoBoxControl(CXuiTextElement box){m_infoBox = box;}
	CXuiTextElement GetInfoBox() {return m_infoBox;}
	CXuiTextElement GetPathBox() {return m_pathBox;}

private:
	VOID SetIniItem(int idx);
	VOID SetIniItem(int idx, DWORD sts, PCHAR driveName, PCHAR drivePath);
	BOOL CleanupList(VOID);
	PINI_ITEM_INFO g_ListData;
	CXuiList m_localList;
	CXuiTextElement m_pathBox;
	CXuiTextElement m_infoBox;
	int m_OverrideIni;

	// for ini updating
	bool bVal, bDval;
	pkeydata pkey;
	char pathTemp[MAX_PATH];
	PCHAR mnt;

	IniData();
	~IniData() {}
	IniData(const IniData&);                 // Prevent copy-construction
	IniData& operator=(const IniData&);      // Prevent assignment
};
