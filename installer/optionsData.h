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

// Information for one list item.
#define MAX_OPTTXT_LEN 20
#define MAX_OPTVAL_LEN MAX_PATH

#define OPT_IS_CATEGORY		0x80000000
#define OPT_IS_CAT_OPEN		0x40000000
#define OPT_NONCAT_INFO(x)	(x&~(OPT_IS_CATEGORY|OPT_IS_CAT_OPEN))

typedef struct _OPTS_SHOWN {
	DWORD listItem;
	DWORD optType;
	DWORD optCategory;
	wstring wname;
	wstring wval;
	wstring wdesc;
} OPTS_SHOWN, *POPTS_SHOWN;

typedef struct _OPTS_LIST {
	CHAR optName[MAX_OPTTXT_LEN];
	DWORD optionVal;
	DWORD defaultValue;
	DWORD optType;
	DWORD optCategory;
} OPTS_LIST, *POPTS_LIST;

class OptionsData
{
public:	
	static OptionsData& GetInstance(){static OptionsData singleton; return singleton;}
	static vector<OPTS_SHOWN> m_optsShow;
	static vector<OPTS_LIST> m_optsList;

	VOID FetchList(VOID);
	VOID RefreshShownList(VOID);
	VOID HandleCategory(int idx);
	VOID ToggleBoolOption(int idx);
	VOID SetValOption(int idx, DWORD val);
	BOOL ValOptionIsHex(int idx);
	BOOL PathGetInfo(int idx, PCHAR* path, PDWORD flags, PDWORD dev);
	VOID PathSetInfoFromDir(int idx, PCHAR path, DWORD flags, DWORD dev);
	VOID PathSetInfo(int idx, PCHAR path, DWORD flags, DWORD dev);
	VOID PathClear(int idx);
	VOID ValClear(int idx);

	// refs to full list
	DWORD GetFullNumItems() {return m_optsList.size();}
	DWORD GetFullItemType(int idx) {return m_optsList.at(idx).optType;}
	PCHAR GetFullItemName(int idx) {return m_optsList.at(idx).optName;}
	DWORD GetFullItemVal(int idx) {return m_optsList.at(idx).optionVal;}
	DWORD GetFullItemDefVal(int idx) {return m_optsList.at(idx).defaultValue;}
	BOOL GetIsFullItemExternal(int idx) {return m_optsList.at(idx).optCategory == OPT_CAT_EXTERNAL;}

	//refs to shown/filtered list
	DWORD GetShownNumItems() {return m_optsShow.size();}
	DWORD GetShownItemItem(int idx) {return m_optsShow.at(idx).listItem;}
	DWORD GetShownItemType(int idx) {return m_optsShow.at(idx).optType;}
	BOOL GetShownItemIsDir(int idx) {return (m_optsShow.at(idx).optCategory&OPT_IS_CATEGORY)!=0;}
	BOOL GetShownItemIsDirOpen(int idx) {return (m_optsShow.at(idx).optCategory&OPT_IS_CAT_OPEN)!=0;}
	const wchar_t* GetShownItemName(int idx) {return m_optsShow.at(idx).wname.c_str();}
	const wchar_t* GetShownItemVal(int idx) {return m_optsShow.at(idx).wval.c_str();}
	const wchar_t* GetShownItemDesc(int idx) {return m_optsShow.at(idx).wdesc.c_str();}
	// refs back from shown list to full list
	DWORD GetShownItemItemVal(int idx) {return m_optsList.at(m_optsShow.at(idx).listItem).optionVal;}
	PCHAR GetShownItemItemName(int idx) {return m_optsList.at(m_optsShow.at(idx).listItem).optName;}

	VOID SetListControl(CXuiList lst){m_localList = lst;}
	VOID SetInfoBox(CXuiTextElement lst){m_infoBox = lst;}
	VOID SetTitleBox(CXuiTextElement ttl){m_titleText = ttl;}
	VOID InitTitleText(VOID);
	CXuiTextElement GetInfoBox() {return m_infoBox;}
private:
	VOID CleanupList(VOID);
	VOID InitShowOptsList(VOID);
	VOID CleanShowOptsList(DWORD category, int sitem = 0);
	VOID AddShowOptsList(DWORD category, int sitem);
	PWCHAR GetWRegionFromVal(DWORD region);
	VOID SetShownInfo(POPTS_SHOWN pshow, DWORD idx);
	VOID UpdateShownItem(DWORD showidx);
	VOID AddShownItem(DWORD idx);
	VOID WprintRegion(DWORD region, PWCHAR regTxt);
	wstring m_curTitle;
	CXuiList m_localList;
	CXuiTextElement m_infoBox;
	CXuiTextElement m_titleText;
	HXUISTRINGTABLE hInfo;

	OptionsData();
	~OptionsData() {}
	OptionsData(const OptionsData&);                 // Prevent copy-construction
	OptionsData& operator=(const OptionsData&);      // Prevent assignment
};
