#include "optionsList.h"
#include "logging.h"

static WCHAR check[] = L"check.png";
static WCHAR ex[] = L"ex.png";
static WCHAR gplus[] = L"plus.png";
static WCHAR gminus[] = L"minus.png";

HRESULT OptionList::OnInit(XUIMessageInit *pInitData, BOOL& bHandled)
{
	OptionsData::GetInstance().FetchList();
	//lDbgPrint("g_pListData %08x\n", g_pListData);
	return S_OK;
}

// Gets called every frame
HRESULT OptionList::OnGetSourceDataText(XUIMessageGetSourceText *pGetSourceTextData, BOOL& bHandled)
{
	if(pGetSourceTextData->bItemData)
	{
		if(OptionsData::GetInstance().GetShownNumItems() != 0)
		{
			if(pGetSourceTextData->iData == 0)
			{
				pGetSourceTextData->szText = OptionsData::GetInstance().GetShownItemName(pGetSourceTextData->iItem);
				bHandled = TRUE;
			}
			else if(pGetSourceTextData->iData == 1)
			{
				pGetSourceTextData->szText = OptionsData::GetInstance().GetShownItemVal(pGetSourceTextData->iItem);
				bHandled = TRUE;
			}
		}
	}
	return S_OK;
}

HRESULT OptionList::OnGetItemCountAll(XUIMessageGetItemCount *pGetItemCountData, BOOL& bHandled)
{
	pGetItemCountData->cItems = OptionsData::GetInstance().GetShownNumItems();
	//lDbgPrint("GetItemCount: %d\n", pGetItemCountData->cItems);
	bHandled = TRUE;
	return S_OK;
}

HRESULT OptionList::OnGetItemEnable(XUIMessageGetItemEnable *pGetItemEnableData, BOOL& bHandled)
{
	if(OptionsData::GetInstance().GetShownNumItems() == 0)
		pGetItemEnableData->bEnabled = FALSE;
	else
		pGetItemEnableData->bEnabled = TRUE;
	bHandled = TRUE;
	return S_OK;
}

HRESULT OptionList::OLDeleteItems(XUIMessageDeleteItems *pDeleteItemsData, BOOL& bHandled)
{
	//lDbgPrint("deleting %d items at %d\n", pDeleteItemsData->cItems, pDeleteItemsData->iAtItem);
	return S_OK;
}

HRESULT OptionList::OnInsertItems(XUIMessageInsertItems *pInsertItemsData, BOOL& bHandled)
{
//	lDbgPrint("inserting %d items at %d\n", pInsertItemsData->cItems, pInsertItemsData->iAtItem);
	return S_OK;
}

HRESULT OptionList::OnNotifySelChanged(HXUIOBJ hObjSource, XUINotifySelChanged * pNotifySel, BOOL& bHandled)
{
	if(OptionsData::GetInstance().GetShownNumItems() == 0)
		OptionsData::GetInstance().GetInfoBox().SetText(L"");
	else
	{
		LPCWSTR info = OptionsData::GetInstance().GetShownItemDesc(pNotifySel->iItem);
		OptionsData::GetInstance().GetInfoBox().SetText(info);
	}
	return S_OK;
}

HRESULT OptionList::OnGetSourceDataImage(XUIMessageGetSourceImage *pGetSourceImageData, BOOL& bHandled)
{
	if((pGetSourceImageData->bItemData))
	{
		if(OptionsData::GetInstance().GetShownNumItems() != 0)
		{
			if(pGetSourceImageData->iData == 0) // opt folder +/-
			{
				if(OptionsData::GetInstance().GetShownItemIsDir(pGetSourceImageData->iItem))
				{
					if(OptionsData::GetInstance().GetShownItemIsDirOpen(pGetSourceImageData->iItem))
						pGetSourceImageData->szPath = gminus;
					else
						pGetSourceImageData->szPath = gplus;
					bHandled = TRUE;
				}
			}
			else if(pGetSourceImageData->iData == 1)
			{
				if(OptionsData::GetInstance().GetShownItemType(pGetSourceImageData->iItem) == DL_OPT_TYPE_BOOL)
				{
					if(OptionsData::GetInstance().GetShownItemItemVal(pGetSourceImageData->iItem))
						pGetSourceImageData->szPath = check;
					else
						pGetSourceImageData->szPath = ex;
					bHandled = TRUE;
				}
			}
		}
	}
	return S_OK;
}
