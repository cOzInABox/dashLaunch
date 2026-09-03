#include "FilerList.h"
#include "dashLaunch.h"
#include "stdio.h"
#include "logging.h"

static WCHAR folderIcon[] = L"filer_folder.png"; // is a directory
static WCHAR xexIcon[] = L"xex.png"; // is a xex
static WCHAR elfIcon[] = L"elf.png"; // is a elf
static WCHAR NoImage[] = L""; // nothing
static WCHAR IconPath[64];

HRESULT FilerList::OnInit(XUIMessageInit *pInitData, BOOL& bHandled)
{
	return S_OK;
}

// Gets called every frame
HRESULT FilerList::OnGetSourceDataText(XUIMessageGetSourceText *pGetSourceTextData, BOOL& bHandled)
{
	if(pGetSourceTextData->bItemData)
	{
		if(pGetSourceTextData->iData == 0)
		{
			pGetSourceTextData->szText = FilerData::GetInstance().GetItemWShowName(pGetSourceTextData->iItem);
			bHandled = TRUE;
		}
		if(pGetSourceTextData->iData == 1)
		{
			pGetSourceTextData->szText = FilerData::GetInstance().GetItemWSize(pGetSourceTextData->iItem);
			bHandled = TRUE;
		}
	}
	return S_OK;
}

HRESULT FilerList::OnGetItemCountAll(XUIMessageGetItemCount *pGetItemCountData, BOOL& bHandled)
{
	pGetItemCountData->cItems = FilerData::GetInstance().GetItemCount();
	//lDbgPrint("FilerList GetItemCount: %d\n", pGetItemCountData->cItems);
	bHandled = TRUE;
	return S_OK;
}

HRESULT FilerList::OnGetItemEnable(XUIMessageGetItemEnable *pGetItemEnableData, BOOL& bHandled)
{
	if(FilerData::GetInstance().GetItemCount() == 0)
		pGetItemEnableData->bEnabled = FALSE;
	else
		pGetItemEnableData->bEnabled = FilerData::GetInstance().GetItemEnable(pGetItemEnableData->iItem);
	bHandled = TRUE;
	return S_OK;
}

HRESULT FilerList::OnDeleteItems(XUIMessageDeleteItems *pDeleteItemsData, BOOL& bHandled)
{
	//lDbgPrint("FilerList deleting %d items at %d\n", pDeleteItemsData->cItems, pDeleteItemsData->iAtItem);
	//if(pDeleteItemsData->iAtItem == 0)
	//{
	//	g_pListData->nItems = g_pListData->nItems-pDeleteItemsData->cItems;
	//}
	return S_OK;
}

HRESULT FilerList::OnInsertItems(XUIMessageInsertItems *pInsertItemsData, BOOL& bHandled)
{
	//lDbgPrint("FilerList inserting %d items at %d\n", pInsertItemsData->cItems, pInsertItemsData->iAtItem);
	//if(pInsertItemsData->iAtItem == 0)
	//{
	//	g_pListData->nItems = pInsertItemsData->cItems;
	//}
	return S_OK;
}

HRESULT FilerList::OnNotifySelChanged(HXUIOBJ hObjSource, XUINotifySelChanged * pNotifySel, BOOL& bHandled)
{
	//if(g_pListData->nItems == 0)
	//	FilerData::GetInstance().GetPathBox().SetText(L"");
	//else
	//{
	//	FilerData::GetInstance().GetPathBox().SetText(g_pListData->pItems[pNotifySel->iItem].optPathTxt);
	//}
	return S_OK;
}

HRESULT FilerList::OnGetSourceDataImage(XUIMessageGetSourceImage *pGetSourceImageData, BOOL& bHandled)
{
	if((pGetSourceImageData->iData == 0) && (pGetSourceImageData->bItemData))
	{
//Syntax: "memory://" + Address + "," + Size
//L"memory://0123ABCD,21A3"

		if(FilerData::GetInstance().GetItemIsdir(pGetSourceImageData->iItem))
			pGetSourceImageData->szPath = folderIcon;
		else
		{
			DWORD isz;
			PBYTE icon = FilerData::GetInstance().GetItemIcon(pGetSourceImageData->iItem, &isz);
			if(icon)
			{
				wsprintfW(IconPath, L"memory://%08x,%x", icon, isz); // 0x3D00
				//lDbgPrint("showing icon: '%S'\n", IconPath);
				pGetSourceImageData->szPath = IconPath;
			}
			else
			{
				if(FilerData::GetInstance().GetItemIsXex(pGetSourceImageData->iItem))
					pGetSourceImageData->szPath = xexIcon;
				else if(FilerData::GetInstance().GetItemIsElf(pGetSourceImageData->iItem))
					pGetSourceImageData->szPath = elfIcon;
				else
					pGetSourceImageData->szPath = NoImage;
			}
		}
		bHandled = TRUE;
	}
	return S_OK;
}

