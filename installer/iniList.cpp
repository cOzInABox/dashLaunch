#include "iniList.h"
#include "dashLaunch.h"
#include "stdio.h"
#include "logging.h"

static WCHAR noDriveImg[] = L"ini_nodisk.png"; // no disk drive at the mount point
static WCHAR hasIniImg[] = L"ini_hasini.png"; // has disk, has launch.ini
static WCHAR noIniImg[] = L"ini_noini.png"; // has disk, has no launch.ini
static WCHAR currIniImg[] = L"ini_curini.png"; // has disk, has launch.ini, launch.ini is currently loaded

HRESULT IniList::OnInit(XUIMessageInit *pInitData, BOOL& bHandled)
{
	g_pListData = IniData::GetInstance().GetListInfo();
// 	IniData::GetInstance().FetchList();
	//lDbgPrint("IniList g_pListData %08x\n", g_pListData);
	return S_OK;
}

// Gets called every frame
HRESULT IniList::OnGetSourceDataText(XUIMessageGetSourceText *pGetSourceTextData, BOOL& bHandled)
{
	if(pGetSourceTextData->bItemData)
	{
		if(pGetSourceTextData->iData == 0)
		{
			pGetSourceTextData->szText = g_pListData->pItems[pGetSourceTextData->iItem].optValTxt;
			bHandled = TRUE;
		}
		else if(pGetSourceTextData->iData == 1)
		{
			pGetSourceTextData->szText = g_pListData->pItems[pGetSourceTextData->iItem].wsize;
			bHandled = TRUE;
		}
	}
	return S_OK;
}

HRESULT IniList::OnGetItemCountAll(XUIMessageGetItemCount *pGetItemCountData, BOOL& bHandled)
{
	pGetItemCountData->cItems = g_pListData->nItems;
	//lDbgPrint("IniList GetItemCount: %d\n", pGetItemCountData->cItems);
	bHandled = TRUE;
	return S_OK;
}

HRESULT IniList::OnGetItemEnable(XUIMessageGetItemEnable *pGetItemEnableData, BOOL& bHandled)
{
	if(g_pListData->nItems == 0)
		pGetItemEnableData->bEnabled = FALSE;
	else
		pGetItemEnableData->bEnabled = g_pListData->pItems[pGetItemEnableData->iItem].fEnabled;
	bHandled = TRUE;
	return S_OK;
}

HRESULT IniList::OnDeleteItems(XUIMessageDeleteItems *pDeleteItemsData, BOOL& bHandled)
{
	//lDbgPrint("IniList deleting %d items at %d\n", pDeleteItemsData->cItems, pDeleteItemsData->iAtItem);
	if(pDeleteItemsData->iAtItem == 0)
	{
		g_pListData->nItems = g_pListData->nItems-pDeleteItemsData->cItems;
	}
	return S_OK;
}

HRESULT IniList::OnInsertItems(XUIMessageInsertItems *pInsertItemsData, BOOL& bHandled)
{
	//lDbgPrint("IniList inserting %d items at %d\n", pInsertItemsData->cItems, pInsertItemsData->iAtItem);
	if(pInsertItemsData->iAtItem == 0)
	{
		g_pListData->nItems = pInsertItemsData->cItems;
	}
	return S_OK;
}

HRESULT IniList::OnNotifySelChanged(HXUIOBJ hObjSource, XUINotifySelChanged * pNotifySel, BOOL& bHandled)
{
	IniData::GetInstance().GetInfoBox().SetText(L"");
	if(g_pListData->nItems != 0)
		IniData::GetInstance().GetPathBox().SetText(g_pListData->pItems[pNotifySel->iItem].optPathTxt);
	return S_OK;
}

HRESULT IniList::OnGetSourceDataImage(XUIMessageGetSourceImage *pGetSourceImageData, BOOL& bHandled)
{
	if((pGetSourceImageData->iData == 0) && (pGetSourceImageData->bItemData))
	{
		if(g_pListData->pItems[pGetSourceImageData->iItem].isCurrentIni)
			pGetSourceImageData->szPath = currIniImg;
		else if(g_pListData->pItems[pGetSourceImageData->iItem].hasIni)
			pGetSourceImageData->szPath = hasIniImg;
		else if(g_pListData->pItems[pGetSourceImageData->iItem].fEnabled)
			pGetSourceImageData->szPath = noIniImg;
		else
			pGetSourceImageData->szPath = noDriveImg;
		bHandled = TRUE;
	}
	return S_OK;
}

