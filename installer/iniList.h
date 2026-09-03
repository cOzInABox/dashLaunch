#pragma once
#include <xtl.h>
#include <xui.h>
#include <xuiapp.h>
#include "xkelib.h"
#include "..\_common.h"
#include "iniData.h"

class IniList : CXuiListImpl
{
private:
	PINI_DATA g_pListData;
public:

	HRESULT OnInit(XUIMessageInit *pInitData, BOOL& bHandled);
	HRESULT OnGetSourceDataText(XUIMessageGetSourceText *pGetSourceTextData, BOOL& bHandled);
	HRESULT OnGetItemCountAll(XUIMessageGetItemCount *pGetItemCountData, BOOL& bHandled);
	HRESULT OnGetItemEnable(XUIMessageGetItemEnable *pGetItemEnableData, BOOL& bHandled);
	HRESULT OnDeleteItems(XUIMessageDeleteItems *pDeleteItemsData, BOOL& bHandled);
	HRESULT OnInsertItems(XUIMessageInsertItems *pInsertItemsData, BOOL& bHandled);
	HRESULT OnNotifySelChanged(HXUIOBJ hObjSource, XUINotifySelChanged *pNotifySelChangedData, BOOL& bHandled);
	HRESULT OnGetSourceDataImage(XUIMessageGetSourceImage *pGetSourceImageData, BOOL& bHandled);

	XUI_BEGIN_MSG_MAP()
		XUI_ON_XM_INIT(OnInit)
		XUI_ON_XM_NOTIFY_SELCHANGED(OnNotifySelChanged)
		XUI_ON_XM_DELETE_ITEMS(OnDeleteItems)
		XUI_ON_XM_INSERT_ITEMS(OnInsertItems)
		XUI_ON_XM_GET_SOURCE_IMAGE(OnGetSourceDataImage)
		XUI_ON_XM_GET_SOURCE_TEXT(OnGetSourceDataText)
		XUI_ON_XM_GET_ITEMCOUNT_ALL(OnGetItemCountAll)
		XUI_ON_XM_GET_ITEMENABLE(OnGetItemEnable)
	XUI_END_MSG_MAP()

	XUI_IMPLEMENT_CLASS(IniList, L"IList", XUI_CLASS_LIST);
};


