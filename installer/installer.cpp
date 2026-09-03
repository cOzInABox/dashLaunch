#include "installer.h"
#include "xkelib.h"
#include "..\..\_common.h"
#include "dashLaunch.h"
#include "XboxUtil.h"
#include "Nand.h"
#include "Resource.h"
#include "optionsList.h"
#include "DataEntry.h"
#include "iniList.h"
#include "filerList.h"
#include "Strings.h"
#include "dlDrives.h"
#include "SlimFTPd.h"
#include "sysInfo.h"
#include "updSvr.h"
#include "logging.h"

InstallerMain* g_MainObject;

HRESULT InstallerMain::OnInit(XUIMessageInit* pInitData, BOOL& bHandled)
{
	g_MainObject = this;
	m_isKernelSupported = FALSE;
	m_hNotify = XamNotifyCreateListenerInternal(XNOTIFY_SYSTEM|XNOTIFY_LIVE, 1, 8 );
	if((m_hNotify != NULL)&&(m_hNotify != INVALID_HANDLE_VALUE))
	{
		SetTimer(TIMER_XNOT_UPDATE, 1000); // start a timer to check for notifications every second
	}
#ifdef LOG_EXTRA_OUT
	lDbgPrint("OnInit: starting\n");
#endif

	m_HelpManual = FALSE;
	m_filerIsLaunch = FALSE;
	m_isHelpShown = FALSE;
	m_isNetInit = FALSE;
	m_isSvrInit = FALSE;

	// Retrieve controls for later use.
	GetChildById(L"TitleText", &m_TitleText);
	GetChildById(L"BackgroundImage", &m_Background);
	DWORD bgSize;
	PBYTE bgData;
	bgData = (PBYTE)Resource::GetInstance().LoadExternalFile("Game:\\background.png", &bgSize);
	if(bgData != NULL)
	{
		WCHAR bgPath[128];
		wsprintfW(bgPath, L"memory://%08X,%X", bgData, bgSize);
		m_Background.SetImagePath(bgPath);
	}
	else
	{
		PWCHAR bg = Resource::GetInstance().GetFilePath(L"background.png");
		m_Background.SetImagePath(bg);
		delete [] bg;
	}
	
	// retrieve temp group
	GetChildById(L"Temps", &m_TempGroup);
	m_TempGroup.GetChildById(L"CpuTemp", &m_TempCPU);
	m_TempGroup.GetChildById(L"GpuTemp", &m_TempGPU);
	m_TempGroup.GetChildById(L"RamTemp", &m_TempEDRAM);
	m_TempGroup.GetChildById(L"MoboTemp", &m_TempMOBO);

	// retrieve hardware group
	GetChildById(L"Hardware", &m_HwGroup);
	m_HwGroup.GetChildById(L"Board", &m_HwBoard);
	m_HwGroup.GetChildById(L"Flash", &m_HwFlash);
	m_HwGroup.GetChildById(L"Type", &m_HwType);
	m_HwGroup.GetChildById(L"Kernel", &m_HwKernel);
	UpdateHardware();

	// stuff for the popup selection scene
	GetChildById(L"PopupScene", &m_PopupScene);
	m_PopupScene.GetChildById(L"TextBox", &m_TextBox);
	m_PopupScene.GetChildById(L"MiddleButton", &m_MiddleButton);
	m_PopupScene.GetChildById(L"LeftButton", &m_LeftButton);
	m_PopupScene.GetChildById(L"RightButton", &m_RightButton);
	m_PopupScene.GetChildById(L"BGHide", &m_BGHide);

	// stuff for the data entry scene
	GetChildById(L"DataEntry", &m_DataScene);
	m_DataScene.GetChildById(L"Cancel", &m_DataCancel);
	m_DataScene.GetChildById(L"OK", &m_DataAccept);
	DataEntry::GetInstance().Init(m_DataScene);

	// stuff for the filer scene
	GetChildById(L"PathBrowse", &m_FilerScene);
	m_FilerScene.GetChildById(L"PathBox", &m_FilerPath);
	m_FilerScene.GetChildById(L"PageTitle", &m_FilerTitle);
	m_FilerScene.GetChildById(L"FileList", &m_FilerList);
	m_FilerScene.GetChildById(L"InfoText", &m_FilerInfo);
	FilerData::GetInstance().SetListControl(m_FilerList);
	FilerData::GetInstance().SetPathBoxControl(m_FilerPath);
	FilerData::GetInstance().SetInfoBoxControl(m_FilerInfo);

	// stuff for the live options scene
	GetChildById(L"OptionsScene", &m_OptionScene);
	m_OptionScene.GetChildById(L"OptionList", &m_OptionList);
	m_OptionScene.GetChildById(L"OptionInfo", &m_OptionInfo);
	m_OptionInfo.SetText(L"");
	m_OptionScene.GetChildById(L"PageTitle", &m_OptionTitle);
	m_OptionList.SetCurSelVisible(0);
	OptionsData::GetInstance().SetListControl(m_OptionList);
	OptionsData::GetInstance().SetInfoBox(m_OptionInfo);
	OptionsData::GetInstance().SetTitleBox(m_OptionTitle);
	OptionsData::GetInstance().InitTitleText();

	// ini list scene
	GetChildById(L"IniScene", &m_IniScene);
	m_IniScene.GetChildById(L"IniList", &m_IniList);
	m_IniScene.GetChildById(L"PageTitle", &m_IniTitle);
	m_IniScene.GetChildById(L"PathBox", &m_IniInfo);
	m_IniScene.GetChildById(L"LastOperation", &m_OptionLast);
	m_IniScene.GetChildById(L"CurrentIni", &m_IniCurrentIni);
	m_IniList.SetCurSelVisible(0);
	IniData::GetInstance().SetListControl(m_IniList);
	IniData::GetInstance().SetPathBoxControl(m_IniInfo);
	IniData::GetInstance().SetInfoBoxControl(m_OptionLast);

	// misc scene
	GetChildById(L"MiscScene", &m_MiscScene);
	m_MiscScene.GetChildById(L"PageTitle", &m_MiscTitle);
	m_MiscScene.GetChildById(L"UnloadButton", &m_MiscUnload);
	m_MiscScene.GetChildById(L"QuitButton", &m_MiscQuit);
	m_MiscScene.GetChildById(L"CurrentVer", &m_MiscCurrentVer);
	m_MiscScene.GetChildById(L"Uninstall", &m_MiscUninstall);
	m_MiscScene.GetChildById(L"PatchesButton", &m_MiscPatches);
	m_MiscScene.GetChildById(L"InstallButton", &m_MiscInstallThis);
	m_MiscScene.GetChildById(L"LaunchButton", &m_MiscLaunch);
	m_MiscScene.GetChildById(L"DebugText", &m_MiscDebug);
	m_MiscScene.GetChildById(L"InfoButton", &m_MiscInfo);

	// info scene
	GetChildById(L"Info", &m_InfoScene);
	m_InfoScene.GetChildById(L"SaveButton", &m_InfoSaveSmc);
	m_InfoScene.GetChildById(L"FtpInfoText", &m_InfoFtpInfo);
	m_InfoScene.GetChildById(L"UpdsvrInfoText", &m_InfoUpdsvrInfo);
	SysInfo::GetInstance().Init(m_InfoScene, m_InfoSaveSmc);
	m_InfoScene.GetChildById(L"CpuFanCheck", &m_CpuFanCheck);
	m_InfoScene.GetChildById(L"GpuFanCheck", &m_GpuFanCheck);
	
	if((!XboxUtil::GetInstance().IsFileExist("Game:\\default.xex"))&&(!XboxUtil::GetInstance().IsFileExist("Game:\\installer.xex")))
		m_MiscInstallThis.SetEnable(FALSE);
	//m_MiscLaunch.SetFocus();

	// help scene (never gets focus!)
	GetChildById(L"Help", &m_HelpScene);
	m_HelpScene.GetChildById(L"AText", &m_HelpAText);
	m_HelpScene.GetChildById(L"BText", &m_HelpBText);
	m_HelpScene.GetChildById(L"XText", &m_HelpXText);
	m_HelpScene.GetChildById(L"YText", &m_HelpYText);
	m_HelpScene.GetChildById(L"RBText", &m_HelpRBText);
	m_HelpScene.GetChildById(L"LBText", &m_HelpLBText);
	//m_HelpScene.SetShow(FALSE);
#ifdef LOG_EXTRA_OUT
	lDbgPrint("OnInit: scenes are ready\n");
#endif

	MiscSetText();
	SetTitleText();
	UpdateTemps();
	SetTimer(TIMER_TEMP_UPDATE, 1000); // start a timer to update temps every 1s
	SetTimer(TIMER_SHOW_HELP, SHOW_HELP_TIME);
#ifdef LOG_EXTRA_OUT
	lDbgPrint("OnInit: timers are ready\n");
#endif

	m_currScene = SCENE_LIVEOPT;
	m_isKernelSupported = DashLaunch::GetInstance().kernelVersionCheck();
	UpdateInstallStatus();
#ifdef LOG_EXTRA_OUT
	lDbgPrint("OnInit: versions checked\n");
#endif

	IniData::GetInstance().FetchList();
#ifdef LOG_EXTRA_OUT
	lDbgPrint("OnInit: FetchList completed\n");
#endif

	HandleNetwork();
#ifdef LOG_EXTRA_OUT
	lDbgPrint("OnInit: network first call completed\n");
#endif
	if(m_isRunning && m_isUpToDate)
	{
		DWORD caopt;
		SetCurrentScene(SCENE_LIVEOPT);
		if(DashLaunch::GetInstance().canUseImports())
		{
#ifdef LOG_EXTRA_OUT
			lDbgPrint("OnInit: dash launch imports are available\n");
#endif
			if(DashLaunch::GetInstance().dlaunchGetOptValByName("calaunch", &caopt))
			{
				if(caopt)
				{
#ifdef LOG_EXTRA_OUT
					lDbgPrint("OnInit: starting in file launch mode\n");
#endif
					m_filerIsLaunch = TRUE;
					FilerData::GetInstance().Init(INVALID_ITEM, NULL, DL_OPT_TYPE_ALLEXEC, TRUE);
					SetCurrentScene(SCENE_PATHBROWSER);
				}
			}
		}
#ifdef LOG_EXTRA_OUT
		else
			lDbgPrint("OnInit: dash launch imports are not available!!\n");
#endif
	}
	else
	{
#ifdef LOG_EXTRA_OUT
		lDbgPrint("OnInit: setting update mode\n");
#endif
		SetCurrentScene(SCENE_NONE);
		Install(TRUE);
	}
	bHandled = TRUE;
#ifdef LOG_EXTRA_OUT
	lDbgPrint("OnInit: complete\n");
#endif

	return S_OK;
}

VOID InstallerMain::MiscSetText(VOID)
{
	m_IniTitle.SetText(Strings::GetInstance().Lookup(L"ini_title"));
	m_MiscTitle.SetText(Strings::GetInstance().Lookup(L"misc_title"));
	m_FilerTitle.SetText(Strings::GetInstance().Lookup(L"filer_title"));
	m_MiscUnload.SetText(Strings::GetInstance().Lookup(L"unload"));
	m_MiscUninstall.SetText(Strings::GetInstance().Lookup(L"uninstall"));
	m_MiscQuit.SetText(Strings::GetInstance().Lookup(L"quit"));
	m_MiscPatches.SetText(Strings::GetInstance().Lookup(L"misc_patchb"));
	m_MiscInstallThis.SetText(Strings::GetInstance().Lookup(L"misc_instbut"));
	m_MiscLaunch.SetText(Strings::GetInstance().Lookup(L"misc_launch"));
	m_MiscInfo.SetText(Strings::GetInstance().Lookup(L"misc_infobut"));

	Strings::GetInstance().LookupAndSetChild(m_MiscScene, L"InstallText", L"misc_insthis");
	Strings::GetInstance().LookupAndSetChild(m_MiscScene, L"LaunchText", L"misc_linfo");
	Strings::GetInstance().LookupAndSetChild(m_MiscScene, L"UnloadText", L"misc_load");
	Strings::GetInstance().LookupAndSetChild(m_MiscScene, L"UninstallText", L"misc_install");
	Strings::GetInstance().LookupAndSetChild(m_MiscScene, L"PatchesText", L"misc_patches");
	Strings::GetInstance().LookupAndSetChild(m_MiscScene, L"QuitText", L"misc_quit");
	Strings::GetInstance().LookupAndSetChild(m_MiscScene, L"CurrentVerText", L"misc_running");
	Strings::GetInstance().LookupAndSetChild(m_MiscScene, L"Language", L"language");
	Strings::GetInstance().LookupAndSetChild(m_MiscScene, L"InfoText", L"misc_info");
	Strings::GetInstance().LookupAndSetChild(m_IniScene, L"CurrentIniTxt", L"ini_curr");
}

HRESULT InstallerMain::PlayAnimation(PWCHAR szStartFrame, PWCHAR szEndFrame, BOOL bRecurse)
{
	int nStartFrame = -1; int nEndFrame = -1;

	// Find our named frame indices
	this->FindNamedFrame(szStartFrame, &nStartFrame);
	this->FindNamedFrame(szEndFrame, &nEndFrame);
//	lDbgPrint("start %d %S end %d %S rec %d\n", nStartFrame, szStartFrame, nEndFrame, szEndFrame, bRecurse);
	// Check if we found the frames, and if so, continue to play the animation, with recursion
	if( nStartFrame > -1 && nEndFrame > -1 ) {
		return this->PlayTimeline( nStartFrame, nStartFrame, nEndFrame, FALSE, bRecurse );
	}

	// If we didn't play the animation we return as failed
	return S_FALSE;
}

VOID InstallerMain::OnAnyKey(DWORD key)
{
	//if(key == VK_PAD_LTRIGGER)
	//{
	//	PlayAnimation(L"FadeIn", L"FadeInEnd", TRUE);
	//}
	//else if(key == VK_PAD_RTRIGGER)
	//	PlayAnimation(L"FadeOut", L"FadeOutEnd", TRUE);
	//else
	if(key != VK_PAD_BACK)
	{
		if(m_isHelpShown)
			ShowHelp();
		else if(!m_HelpManual)
			SetTimer(TIMER_SHOW_HELP, SHOW_HELP_TIME);
	}
	//if(key == VK_PAD_LTRIGGER)
	//	OptionsData::GetInstance().CleanupList();
	//if(key == VK_PAD_RTRIGGER)
	//	OptionsData::GetInstance().FetchList();

}

// for sliders
HRESULT InstallerMain::OnNotifyValueChanged(HXUIOBJ hObjSource, XUINotifyValueChanged *pNotifyValueChangedData, BOOL& bHandled)
{
	if(m_currScene == SCENE_INFO)
	{
		SysInfo::GetInstance().SliderChange(hObjSource, pNotifyValueChangedData->nValue);
	}
	return S_OK;
}
//VOID InstallerMain::KeyboardCompletion(DWORD errCode, DWORD numBytes, PXOVERLAPPED pOv)
//{
//	lDbgPrint("completion routine: errcode %x numbytes %x exterr: %x\n", errCode, numBytes, pOv->dwExtendedError);
//}
//
//BOOL InstallerMain::ProcessKeyboard(DWORD user, PWCHAR defTxt, PWCHAR ttlTxt)
//{
//	XOVERLAPPED xov;
//	ZeroMemory(&xov, sizeof(XOVERLAPPED));
//	ZeroMemory(m_wKeyboardBuf, MAX_PATH*sizeof(WCHAR));
//	xov.pCompletionRoutine = (PXOVERLAPPED_COMPLETION_ROUTINE)KeyboardCompletion;
//	DWORD res = XShowKeyboardUI(user, VKBD_LATIN_ALPHABET, defTxt, ttlTxt, NULL, m_wKeyboardBuf, MAX_PATH, &xov);
//	lDbgPrint("Show keyboard returns %x\n", res);
//	if(res == ERROR_IO_PENDING)
//	{
//		lDbgPrint("waiting for kb\n");
//		return TRUE;
//	}
//	return FALSE;
//}


HRESULT InstallerMain::OnKeyUp(XUIMessageInput *pInputData, BOOL& bHandled)
{
	if(!m_PopupScene.IsShown())
	{
		if(pInputData->dwKeyCode == VK_PAD_BACK)
		{
			if(m_isHelpShown)
			{
				m_HelpManual = TRUE;
				KillTimer(TIMER_SHOW_HELP);
			}
			ShowHelp();
			bHandled = TRUE;
		}
		else
		{
			int nIndex;
			switch(m_currScene)
			{
				case SCENE_PATHBROWSER:
					switch(pInputData->dwKeyCode)
					{
						case VK_PAD_Y:
							// path up
							FilerData::GetInstance().UpDir();
							bHandled = TRUE;
							break;
						case VK_PAD_B:
							m_filerIsLaunch = FALSE;
							SetCurrentScene(SCENE_PREVIOUS);
							bHandled = TRUE;
							break;
						case VK_PAD_X:
							if((!FilerData::GetInstance().IsRoot())&&(FilerData::GetInstance().IsCreateFile()||FilerData::GetInstance().IsSearchPath()))
							{
								SetCurrentScene(SCENE_PREVIOUS);
								if(FilerData::GetInstance().IsCreateFile())
								{
									// show keyboard dialog here, create a file at the path
									if(m_currScene == SCENE_LIVEOPT) // only option currently using this is dumpfile
									{
										PCHAR fpath = FilerData::GetInstance().GetCurPath();
										DWORD fdev = FilerData::GetInstance().GetCurDev();
										OptionsData::GetInstance().PathSetInfoFromDir(m_pressedIndex, fpath, ITEM_FOUND, fdev);
									}
								}
								else
								{
									// dispatch whatever wanted the path
									if(m_currScene == SCENE_MISC) // currently only install this is here
									{
										PCHAR fpath = FilerData::GetInstance().GetCurPath();
										DWORD fdev = FilerData::GetInstance().GetCurDev();
										PCHAR drive = dlDrives::GetInstance().GetDriveFriendly(fdev);
										XboxUtil::GetInstance().InstallThis(drive, fpath);
									}
								}
							}
						default:
							break;
					}
					break;
				case SCENE_DATAENTRY:
					switch(pInputData->dwKeyCode)
					{
						case VK_PAD_X:
								DataEntry::GetInstance().ProcessBackspace();
								bHandled = TRUE;
								break;
						case VK_PAD_B:
								SetCurrentScene(SCENE_PREVIOUS); // same as cancel really
								bHandled = TRUE;
								break;
						default:
							break;
					}
					break; // this scene doesn't allow quit or tabbing, so break
				case SCENE_INICONF:
					nIndex = m_IniList.GetCurSel();
					switch(pInputData->dwKeyCode) // a load, x save, y delete
					{
						case VK_PAD_A:
							if(m_IniList.IsItemEnabled(nIndex))
							{
								IniData::GetInstance().ReadFromFile(nIndex);
								UpdateInstallStatus(); // update the curr ini text in misc screen
								OptionsData::GetInstance().FetchList();
								bHandled = TRUE;
							}
							break;
						case VK_PAD_X:
							if(m_IniList.IsItemEnabled(nIndex))
							{
								IniData::GetInstance().WriteToFile(nIndex);
								bHandled = TRUE;
							}
							break;
						case VK_PAD_Y:
							if(m_IniList.IsItemEnabled(nIndex))
							{
								IniData::GetInstance().RemoveFile(nIndex);
								bHandled = TRUE;
							}
							break;
						case VK_PAD_B:
							XboxUtil::GetInstance().QuitToDefault();
							bHandled = TRUE;
							break;
						case VK_PAD_RSHOULDER:
							SetCurrentScene(m_currScene+1);
							bHandled = TRUE;
							break;
						case VK_PAD_LSHOULDER:
							SetCurrentScene(m_currScene-1);
							bHandled = TRUE;
							break;
						default:
							break;
					}
					break;
				case SCENE_LIVEOPT:
					switch(pInputData->dwKeyCode)
					{
						case VK_PAD_Y:
							nIndex = m_OptionList.GetCurSel();
							if(OptionsData::GetInstance().GetShownItemType(nIndex) >= DL_OPT_TYPE_PATH)
							{
								OptionsData::GetInstance().PathClear(nIndex);
								bHandled = TRUE;
							}
							else if(OptionsData::GetInstance().GetShownItemType(nIndex) >= DL_OPT_TYPE_WORD)
							{
								OptionsData::GetInstance().ValClear(nIndex);
								bHandled = TRUE;
							}
							break;
						case VK_PAD_B:
							XboxUtil::GetInstance().QuitToDefault();
							bHandled = TRUE;
							break;
						case VK_PAD_RSHOULDER:
							SetCurrentScene(m_currScene+1);
							bHandled = TRUE;
							break;
						case VK_PAD_LSHOULDER:
							SetCurrentScene(m_currScene-1);
							bHandled = TRUE;
							break;
						default:
							break;
					}
					break;
				case SCENE_MISC:
					switch(pInputData->dwKeyCode)
					{
						case VK_PAD_B:
							XboxUtil::GetInstance().QuitToDefault();
							bHandled = TRUE;
							break;
						case VK_PAD_RSHOULDER:
							SetCurrentScene(m_currScene+1);
							bHandled = TRUE;
							break;
						case VK_PAD_LSHOULDER:
							SetCurrentScene(m_currScene-1);
							bHandled = TRUE;
							break;
						default:
							break;
					}
					break;
				case SCENE_INFO:
					switch(pInputData->dwKeyCode)
					{
						case VK_PAD_B:
							SetCurrentScene(SCENE_PREVIOUS);
							bHandled = TRUE;
							break;
					}
					break;
				default:
					break;
			}
		}
	}
	// not setting handled so it will forward to other things too
	return S_OK;
}

/*
LPCWSTR szId = NULL;
HRESULT hr = XuiElementGetId(hObjPressed, &szId);
if (szId != NULL && !wcscmp(szId, L"ButtonId"))
{
// The button named ButtonId was pressed.
}
*/
//----------------------------------------------------------------------------------
// Handler for the button press message.
//----------------------------------------------------------------------------------
HRESULT InstallerMain::OnNotifyPress(HXUIOBJ hObjPressed, BOOL& bHandled)
{
	if(m_PopupScene.IsShown())
	{
		if(hObjPressed == m_LeftButton)
		{
			fp_popHandler(this, 0);
		}
		else if(hObjPressed == m_MiddleButton)
		{
			fp_popHandler(this, 1);
		}
		else if(hObjPressed == m_RightButton)
		{
			fp_popHandler(this, 2);
		}
	}
	else
	{
		switch(m_currScene)
		{
			case SCENE_LIVEOPT:
				if(hObjPressed == m_OptionList)
				{
					m_pressedIndex = m_OptionList.GetCurSel(&m_pressedButton);
					if(OptionsData::GetInstance().GetShownItemIsDir(m_pressedIndex))
						OptionsData::GetInstance().HandleCategory(m_pressedIndex);
					else
					{
						DWORD optType = OptionsData::GetInstance().GetShownItemType(m_pressedIndex);
						if(optType == DL_OPT_TYPE_BOOL)
						{
							OptionsData::GetInstance().ToggleBoolOption(m_pressedIndex);
							if(stricmp("ftpserv", OptionsData::GetInstance().GetShownItemItemName(m_pressedIndex)) == 0)
								HandleNetwork();
							else if(stricmp("updserv", OptionsData::GetInstance().GetShownItemItemName(m_pressedIndex)) == 0)
								HandleNetwork();
						}
						else if(optType >= DL_OPT_TYPE_MAX_ACCESS) // all paths are >= to this
						{
							DWORD dev;
							PCHAR path;
							if(OptionsData::GetInstance().PathGetInfo(m_pressedIndex, &path, NULL, &dev))
								FilerData::GetInstance().Init(dev, path, optType);
							else
								FilerData::GetInstance().Init(INVALID_ITEM, NULL, optType);

							SetCurrentScene(SCENE_PATHBROWSER);
						}
						else
						{
							SetCurrentScene(SCENE_DATAENTRY);
							DataEntry::GetInstance().SetCurrVal(OptionsData::GetInstance().GetShownItemItemVal(m_pressedIndex), OptionsData::GetInstance().ValOptionIsHex(m_pressedIndex));
						}
					}
				}
				break;
			case SCENE_PATHBROWSER:
				if(hObjPressed == m_FilerList)
				{
					int flitem = m_FilerList.GetCurSel();
					if(FilerData::GetInstance().ItemSelect(flitem)) // returns true on a non dir item
					{
						DWORD fflag = CON_PATH;
						PCHAR fpath = FilerData::GetInstance().GetCurItem();
						DWORD fdev = FilerData::GetInstance().GetCurDev();
						if(FilerData::GetInstance().IsCurItemXex())
							fflag = XEX_PATH;
						else if(FilerData::GetInstance().IsCurItemElf())
							fflag = ELF_PATH;
						if(m_filerIsLaunch)
						{
							DashLaunch::GetInstance().LaunchItem(fdev, fflag, fpath);
							m_filerIsLaunch = FALSE;
						}
						else
						{
							OptionsData::GetInstance().PathSetInfo(m_pressedIndex, fpath, fflag, fdev);
						}
						SetCurrentScene(SCENE_PREVIOUS);
					}
				}
			case SCENE_INICONF:
				if(hObjPressed == m_IniList)
				{
					//int nIndex = m_IniList.GetCurSel();
					//lDbgPrint("notify press\n");
				}
				break;
			case SCENE_MISC:
				if(hObjPressed == m_MiscLaunch)
				{
					m_filerIsLaunch = TRUE;
					FilerData::GetInstance().Init(INVALID_ITEM, NULL, DL_OPT_TYPE_ALLEXEC, TRUE);
					SetCurrentScene(SCENE_PATHBROWSER);
				}
				else if(hObjPressed == m_MiscUnload)
				{
					LoadUnload();
				}
				else if(hObjPressed == m_MiscUninstall)
				{
					if(m_isInstalled)
						Uninstall();
					else
						Install(FALSE);
				}
				else if(hObjPressed == m_MiscPatches)
				{
					ShowPopup(Strings::GetInstance().Look(L"patchupdate"), Strings::GetInstance().Look(L"ok"), NULL, Strings::GetInstance().Look(L"no"), TRUE, popupPatches2);
// 					Nand::GetInstance().UpdatePatches();
// 					UpdateInstallStatus();
				}
				else if(hObjPressed == m_MiscInstallThis)
				{
					FilerData::GetInstance().Init(INVALID_ITEM, NULL, FILER_SEARCH_PATH, FALSE);
					SetCurrentScene(SCENE_PATHBROWSER);
				}
				else if(hObjPressed == m_MiscQuit)
				{
					XboxUtil::GetInstance().QuitToDefault();
				}
				else if(hObjPressed == m_MiscInfo)
				{
					SetCurrentScene(SCENE_INFO);
				}
				break;
			case SCENE_DATAENTRY:
				if(hObjPressed == m_DataCancel)
				{
					SetCurrentScene(SCENE_PREVIOUS);
				}
				else if (hObjPressed == m_DataAccept)
				{
					OptionsData::GetInstance().SetValOption(m_pressedIndex, DataEntry::GetInstance().GetValue());
					SetCurrentScene(SCENE_PREVIOUS);
				}
				else
				{
					DataEntry::GetInstance().ProcessButton(hObjPressed);
				}
				break;
			case SCENE_INFO:
				if(hObjPressed == m_InfoSaveSmc)
				{
					SysInfo::GetInstance().SaveSmcChanges();
					//SetCurrentScene(SCENE_PREVIOUS);
				}
				else if ((hObjPressed == m_CpuFanCheck)||(hObjPressed == m_GpuFanCheck))
					SysInfo::GetInstance().CheckChange(hObjPressed);
				break;
			default:
				lDbgPrint("m_currScene unhandled button press %08x\n", hObjPressed);
				break;
		}
	}

	bHandled = TRUE;
	return S_OK;
}

VOID InstallerMain::HandleFtp(DWORD toggle, DWORD dwPort, char* ciptxt)
{
	if((toggle == 0)&&(m_isNetInit == TRUE))
	{
		StopFtpd();
		m_isNetInit = FALSE;
		m_InfoFtpInfo.SetText(Strings::GetInstance().Lookup(L"misc_norun"));
	}
	else if((toggle == 1)&&(m_isNetInit == FALSE))
	{
		USHORT port;
		port = dwPort&0xFFFF;
		if(InitFtpd(port))
		{
			WCHAR ipTxt[64];
			wsprintfW(ipTxt, L"%S:%d", ciptxt, port);
			m_InfoFtpInfo.SetText(ipTxt);
			m_isNetInit = TRUE;
		}
		else
		{
			lDbgPrint("error starting FTP\n");
			m_InfoFtpInfo.SetText(Strings::GetInstance().Lookup(L"misc_norun"));
		}
	}
}

VOID InstallerMain::HandleUpdSvr(DWORD toggle, char* ciptxt)
{
	if((toggle == 0)&&(m_isSvrInit == TRUE))
	{
		Updsvr::GetInstance().shutdown();
		m_isSvrInit = FALSE;
		m_InfoUpdsvrInfo.SetText(Strings::GetInstance().Lookup(L"misc_norun"));
	}
	else if((toggle == 1)&&(m_isSvrInit == FALSE))
	{
		if(Updsvr::GetInstance().startup())
		{
			WCHAR ipTxt[64];
			wsprintfW(ipTxt, L"%S", ciptxt);
			m_InfoUpdsvrInfo.SetText(ipTxt);
			m_isSvrInit = TRUE;
		}
		else
		{
			lDbgPrint("error starting updsvr\n");
			m_InfoUpdsvrInfo.SetText(Strings::GetInstance().Lookup(L"misc_norun"));
		}
	}
}

VOID InstallerMain::HandleNetwork(void)
{
	if(XNetGetEthernetLinkStatus())
	{
		DWORD ftpopt = 1, updopt = 1, dwPort = 21; // default servers to operational
		char ip[20];
		memset(ip, 0, 20);
		if(XboxUtil::GetInstance().GetIpAddr(ip, 20))
		{
			if(DashLaunch::GetInstance().canUseImports())
			{
				if(DashLaunch::GetInstance().dlaunchGetOptValByName("ftpserv", &ftpopt) == FALSE)
					ftpopt = 1;
				if(DashLaunch::GetInstance().dlaunchGetOptValByName("updserv", &updopt) == FALSE)
					updopt = 1;
				if(DashLaunch::GetInstance().dlaunchGetOptValByName("ftpport", &dwPort) == FALSE)
					dwPort = 21;
			}
			lDbgPrint("updopt:%d ftpopt:%d dwport:%d ip:%s\n", updopt, ftpopt, dwPort, ip);
			HandleFtp(ftpopt, dwPort, ip);
			HandleUpdSvr(updopt, ip);
			return;
		}
		else
			lDbgPrint("network connection present, unable to get IP!\n");
	}
	HandleFtp(0, 21, NULL);
	HandleUpdSvr(0, NULL);
}

VOID InstallerMain::HandleXnotify(VOID)
{
	DWORD nId;
	ULONG_PTR pParam;
	if(XNotifyGetNext(m_hNotify, 0, &nId, &pParam)) // XN_SYS_DVDSTATECHANGED
	{
		switch(nId)
		{
			case XN_LIVE_LINK_STATE_CHANGED:
				HandleNetwork(); // pParam will be 1 if connecting, 0 if disconnecting
				break;
			case XN_SYS_STORAGEDEVICESCHANGED:
				IniData::GetInstance().RefreshList();
				FtpdSetDevices();
				if(m_FilerScene.IsShown())
					FilerData::GetInstance().RefreshIfRoot();
				break;
		}
	}
}

HRESULT InstallerMain::OnTimer(XUIMessageTimer *pTimer, BOOL& bHandled)
{
	// which timer is it?
	switch(pTimer->nId)
	{
		case TIMER_TEMP_UPDATE:
			UpdateTemps();
			bHandled = TRUE;
			break;
		case TIMER_XNOT_UPDATE:
			HandleXnotify();
			bHandled = TRUE;
			break;
		case TIMER_SHOW_HELP:
			ShowHelp();
			bHandled = TRUE;
			break;
	}
	return S_OK;
}

VOID InstallerMain::SetTitleText(VOID)
{
	WCHAR titlTemp[128];
#ifdef RELEASE_IS_BETA
	wsprintfW(titlTemp, L"DashLaunch V%d.%02d (%d) BETA", VER_MAJ, VER_MIN, VER_SVN);
#else
	wsprintfW(titlTemp, L"DashLaunch V%d.%02d (%d)", VER_MAJ, VER_MIN, VER_SVN);
#endif
	m_TitleText.SetText(titlTemp);
}

VOID InstallerMain::UpdateTemps(VOID)
{
	UCHAR msg[16];
	UCHAR ret[16];
	ZeroMemory(msg, 16);
	ZeroMemory(ret, 16);

	msg[0] = 0x07;
	HalSendSMCMessage(msg, ret);

	if(ret[0] == 0x07)
	{
		WCHAR outTemp[64];
		double cpu = (ret[1] | (ret[2] << 8)) / 256.0;
		double gpu = (ret[3] | (ret[4] << 8)) / 256.0;
		double edram = (ret[5] | (ret[6] << 8)) / 256.0;
		double mb = (ret[7] | (ret[8] << 8)) / 256.0;
		if(DashLaunch::GetInstance().IsUseFarenheit())
		{
			cpu = (cpu*1.8)+32;
			gpu = (gpu*1.8)+32;
			edram = (edram*1.8)+32;
			mb = (mb*1.8)+32;
			wsprintfW(outTemp, L"CPU: %3.1f°F", cpu);
			m_TempCPU.SetText(outTemp);
			wsprintfW(outTemp, L"GPU: %3.1f°F", gpu);
			m_TempGPU.SetText(outTemp);
			wsprintfW(outTemp, L"MOBO: %3.1f°F", mb);
			m_TempMOBO.SetText(outTemp);
			wsprintfW(outTemp, L"EDRAM: %3.1f°F", edram);
			m_TempEDRAM.SetText(outTemp);
		}
		else
		{
			wsprintfW(outTemp, L"CPU: %3.1f°C", cpu);
			m_TempCPU.SetText(outTemp);
			wsprintfW(outTemp, L"GPU: %3.1f°C", gpu);
			m_TempGPU.SetText(outTemp);
			wsprintfW(outTemp, L"MOBO: %3.1f°C", mb);
			m_TempMOBO.SetText(outTemp);
			wsprintfW(outTemp, L"EDRAM: %3.1f°C", edram);
			m_TempEDRAM.SetText(outTemp);
		}
	}
}

VOID InstallerMain::UpdateHardware(VOID)
{
	WCHAR outTemp[64];
	wsprintfW(m_kernelVer, L"%d.%d.%d.%d", XboxKrnlVersion->Major, XboxKrnlVersion->Minor, XboxKrnlVersion->Build, XboxKrnlVersion->Qfe);
	wsprintfW(outTemp, L"Kernel: %s", m_kernelVer);
	m_HwKernel.SetText(outTemp);
	wsprintfW(outTemp, L"Board: %s", Nand::GetInstance().GetHardwareModelWchar());
	m_HwBoard.SetText(outTemp);
	wsprintfW(outTemp, L"Flash: %s", Nand::GetInstance().GetFlashModelWchar());
	m_HwFlash.SetText(outTemp);
	wsprintfW(outTemp, L"Type: %s", Nand::GetInstance().GetFlashTypeWchar());
	m_HwType.SetText(outTemp);
}

VOID InstallerMain::Uninstall(VOID)
{
	if(Nand::GetInstance().Uninstall())
	{
		m_MiscUninstall.SetText(Strings::GetInstance().Lookup(L"install"));
		m_isInstalled = FALSE;
		m_isLhelperInstalled = FALSE;
	}
	else
	{
		m_MiscUninstall.SetText(L"ERROR!");
		m_MiscUninstall.SetEnable(FALSE);
	}
	if(m_isRunning && !m_requiresReboot)
		LoadUnload();
	UpdateInstallStatus();
	if(m_requiresReboot)
		ShowPopup(Strings::GetInstance().Look(L"shutdown"), NULL, Strings::GetInstance().Look(L"ok"), NULL, TRUE, popupReboot);
}

VOID InstallerMain::UpdateInstallStatus(VOID)
{
	//BOOL m_isInstalled; // tracks whether both lhelper and launch are installed
	//BOOL m_isLhelperInstalled; // tracks whether lhelper is installed
	//BOOL m_isUpToDate; // tracks whether it's up to date
	//BOOL m_requiresReboot; // tracks whether changes to the current install require a reboot
	//BOOL m_isRunning; // tracks whether launch.xex is in memory
	//BOOL m_isSupported; // tracks whether this can even be installed

	m_isLhelperInstalled = XboxUtil::GetInstance().IsFileExist("media:\\lhelper.xex");
	m_isInstalled = (m_isLhelperInstalled&&XboxUtil::GetInstance().IsFileExist("media:\\launch.xex"));
	m_isUpToDate = DashLaunch::GetInstance().isUpToDate();
	m_requiresReboot = DashLaunch::GetInstance().requiresReboot();
	m_isRunning = DashLaunch::GetInstance().isLoaded();
	m_arePatchesAvail = Nand::GetInstance().IsPatchUpdateAvail();

	if(m_isKernelSupported)
	{
		if((!m_isRunning)&&(!m_isInstalled))
			m_requiresReboot = FALSE;
		if(m_isInstalled)
			m_MiscUninstall.SetText(Strings::GetInstance().Look(L"uninstall"));
		else
		{
			if(m_isRunning)
				m_MiscUninstall.SetText(Strings::GetInstance().Look(L"update"));
			else
				m_MiscUninstall.SetText(Strings::GetInstance().Look(L"install"));
		}

		if(m_isRunning)
		{
			m_MiscUnload.SetText(Strings::GetInstance().Look(L"unload"));
			if(m_requiresReboot)
				m_MiscUnload.SetEnable(FALSE);
		}
		else
			m_MiscUnload.SetText(Strings::GetInstance().Look(L"load"));

		if(m_arePatchesAvail)
			m_MiscPatches.SetEnable(TRUE);
		else
			m_MiscPatches.SetEnable(FALSE);
	}
	else
	{
		m_MiscUnload.SetText(Strings::GetInstance().Look(L"unload"));
		m_MiscUnload.SetEnable(FALSE);
		m_arePatchesAvail = FALSE;
		m_MiscUninstall.SetText(Strings::GetInstance().Look(L"install"));
		m_MiscUninstall.SetEnable(FALSE);
		m_MiscPatches.SetEnable(FALSE);
		m_requiresReboot = FALSE;
	}

	m_MiscCurrentVer.SetText(DashLaunch::GetInstance().getCurrDlVer());
	m_IniCurrentIni.SetText(DashLaunch::GetInstance().getCurrIni());
}

VOID InstallerMain::popupReboot(PVOID obj, DWORD opt)
{
	XboxUtil::GetInstance().Reboot();
}

VOID InstallerMain::popupPatches2(PVOID obj, DWORD opt) // 0 = install, 2 = cancel
{
	InstallerMain* pOb = (InstallerMain*)obj;
	if(opt == 0)
	{
		Nand::GetInstance().UpdatePatches();
		pOb->UpdateInstallStatus();
		pOb->m_requiresReboot = TRUE;
	}
	if(pOb->m_requiresReboot)
		pOb->ShowPopup(Strings::GetInstance().Look(L"shutdown"), NULL, Strings::GetInstance().Look(L"ok"), NULL, TRUE, pOb->popupReboot);
	else
		pOb->SetCurrentScene(pOb->m_currScene);
}

VOID InstallerMain::popupPatches(PVOID obj, DWORD opt) // 0 = install, 2 = cancel
{
	InstallerMain* pOb = (InstallerMain*)obj;
	if(opt == 0)
	{
		Nand::GetInstance().UpdatePatches();
		pOb->UpdateInstallStatus();
		pOb->m_requiresReboot = TRUE;
	}
	if(pOb->m_requiresReboot)
		pOb->ShowPopup(Strings::GetInstance().Look(L"shutdown"), NULL, Strings::GetInstance().Look(L"ok"), NULL, TRUE, pOb->popupReboot);
	else
		pOb->SetCurrentScene(pOb->m_currScene);
}

VOID InstallerMain::popupInstall(PVOID obj, DWORD opt) //0 = install/update, 1 = cancel, 2 = quit
{
	InstallerMain* pOb = (InstallerMain*)obj;
	if (opt == 2)// quit
		XboxUtil::GetInstance().QuitToDefault();
	else if(opt == 0) // install/update
	{
		if((pOb->m_isRunning)&&(!pOb->m_requiresReboot))
		{
			pOb->LoadUnload();
		}
		Nand::GetInstance().UpdateLaunchXex();
		pOb->UpdateInstallStatus();
		//WCHAR txt[40];
		//wsprintfW(txt, L"run: %d reboot: %d patches: %d", pOb->m_isRunning, pOb->m_requiresReboot, pOb->m_arePatchesAvail);
		//pOb->m_MiscDebug.SetText(txt);
		if(!pOb->m_requiresReboot)
		{
			pOb->LoadUnload();
			pOb->UpdateInstallStatus();
		}
		if(pOb->m_arePatchesAvail)
		{			
			pOb->ShowPopup(Strings::GetInstance().Look(L"patchupdate"), Strings::GetInstance().Look(L"ok"), NULL, Strings::GetInstance().Look(L"no"), TRUE, pOb->popupPatches);
		}
		else if(pOb->m_requiresReboot)
		{
			pOb->ShowPopup(Strings::GetInstance().Look(L"shutdown"), NULL, Strings::GetInstance().Look(L"ok"), NULL, TRUE, pOb->popupReboot);
		}
		else
			pOb->SetCurrentScene(pOb->m_currScene); // cancel
	}
	else
		pOb->SetCurrentScene(pOb->m_currScene); // cancel
}

VOID InstallerMain::Install(BOOL BootTime)
{
	WCHAR utxt[256];
	PWCHAR leftbut = Strings::GetInstance().Look(L"install");
	PWCHAR midbut = Strings::GetInstance().Look(L"cancel");
	PWCHAR rightbut = Strings::GetInstance().Look(L"quit");
	UpdateInstallStatus();
	if(BootTime)
		m_currScene = SCENE_LIVEOPT;
	if(!m_isKernelSupported)
	{
		wsprintfW(utxt, Strings::GetInstance().Lookup(L"unsupported"), m_kernelVer);
		ShowPopup(utxt, NULL, midbut, rightbut, !BootTime, popupInstall);
	}
	else
	{
		if(m_isRunning)
		{
			if(m_isInstalled)
			{
				wsprintfW(utxt, Strings::GetInstance().Lookup(L"versionmiss"), DashLaunch::GetInstance().getCurrDlVer());
				leftbut = Strings::GetInstance().Look(L"update");
			}
			else
				wsprintfW(utxt, Strings::GetInstance().Lookup(L"runinstall"), DashLaunch::GetInstance().getCurrDlVer());
		}
		else
			wsprintfW(utxt, Strings::GetInstance().Lookup(L"noruninstall"));
		ShowPopup(utxt, leftbut, midbut, rightbut, !BootTime, popupInstall);
	}
}

VOID InstallerMain::LoadUnload(VOID)
{
	if(m_isRunning)
	{
		DashLaunch::GetInstance().Unload();
		if(DashLaunch::GetInstance().CheckLoad())
			m_MiscUnload.SetEnable(FALSE);
	}
	else
	{
		if(!m_isLhelperInstalled)
		{
			 m_isLhelperInstalled = Nand::GetInstance().WriteLhelperToFlash();
		}
		if(m_isLhelperInstalled)
		{
			DashLaunch::GetInstance().Load();
			if(!DashLaunch::GetInstance().CheckLoad())
				m_MiscUnload.SetEnable(FALSE);
		}
		else
			m_MiscUnload.SetEnable(FALSE);
	}
	m_MiscCurrentVer.SetText(DashLaunch::GetInstance().getCurrDlVer());
	OptionsData::GetInstance().FetchList();
	IniData::GetInstance().FetchList();
	UpdateInstallStatus();
}

VOID InstallerMain::SetCurrentScene(DWORD scene)
{
	if(scene == SCENE_PREVIOUS)
	{
		m_currScene = m_prevScene;
	}
	else
	{
		m_prevScene = m_currScene;
		if(scene == SCENE_MAX)
			m_currScene = SCENE_MIN+1;
		else if(scene == SCENE_MIN)
			m_currScene = SCENE_MAX-1;
		else
			m_currScene = scene;
	}
	// hide all scenes unless its data entry
	if(scene != SCENE_DATAENTRY)
	{
		m_OptionLast.SetText(L"");
		m_FilerScene.SetShow(FALSE);
		m_PopupScene.SetShow(FALSE);
		m_DataScene.SetShow(FALSE);
		m_OptionScene.SetShow(FALSE);
		m_MiscScene.SetShow(FALSE);
		m_IniScene.SetShow(FALSE);
		m_InfoScene.SetShow(FALSE);
	}
	// show the one we want
	switch(m_currScene)
	{
		case SCENE_LIVEOPT:
			m_OptionScene.SetShow(TRUE);
			m_OptionList.SetFocus();
			break;
		case SCENE_INICONF:
			m_IniScene.SetShow(TRUE);
			m_IniScene.SetFocus();
			break;
		case SCENE_MISC:
			m_MiscScene.SetShow(TRUE);
			m_MiscScene.SetFocus();
			break;
		case SCENE_DATAENTRY:
			DataEntry::GetInstance().Reset();
			m_DataScene.SetShow(TRUE);
			m_DataScene.SetFocus();
			break;
		case SCENE_PATHBROWSER:
			m_FilerScene.SetShow(TRUE);
			m_FilerList.SetFocus();
			break;
		case SCENE_INFO:
			m_InfoScene.SetShow(TRUE);
			m_InfoScene.SetFocus();
			break;
		default:
			break;
	}
}

VOID InstallerMain::ShowHelp(VOID)
{
	if(m_isHelpShown)
	{
		PlayAnimation(L"FadeOut", L"FadeOutEnd", TRUE);
		m_isHelpShown = FALSE;
// 		m_HelpScene.SetShow(FALSE);
		if(!m_HelpManual)
			SetTimer(TIMER_SHOW_HELP, SHOW_HELP_TIME);
	}
	else
	{
		if(!m_PopupScene.IsShown())
		{
			WCHAR name[128] = L"\0";
			switch(m_currScene) // ini_helpA
			{
				case SCENE_INICONF:
					wsprintfW(name, L"ini_help");
					break;
				case SCENE_LIVEOPT:
					wsprintfW(name, L"opts_help");
					break;
				case SCENE_MISC:
					wsprintfW(name, L"misc_help");
					break;
				case SCENE_DATAENTRY:
					wsprintfW(name, L"data_help");
					break;
				case SCENE_PATHBROWSER:
					wsprintfW(name, L"path_help");
					break;
				case SCENE_INFO:
					wsprintfW(name, L"info_help");
					break;
				default:
					return;
			}
			if(name[0] != 0)
			{
				m_HelpAText.SetText(Strings::GetInstance().Lookup(name, L"A"));
				m_HelpBText.SetText(Strings::GetInstance().Lookup(name, L"B"));
				m_HelpXText.SetText(Strings::GetInstance().Lookup(name, L"X"));
				m_HelpYText.SetText(Strings::GetInstance().Lookup(name, L"Y"));
				m_HelpRBText.SetText(Strings::GetInstance().Lookup(name, L"RB"));
				m_HelpLBText.SetText(Strings::GetInstance().Lookup(name, L"LB"));
				PlayAnimation(L"FadeIn", L"FadeInEnd", TRUE);
				m_isHelpShown = TRUE;
// 				m_HelpScene.SetShow(TRUE);
				KillTimer(TIMER_SHOW_HELP);
			}
		}
	}
}

VOID InstallerMain::ShowPopup(const PWCHAR messageTxt, const PWCHAR leftTxt, const PWCHAR midTxt, const PWCHAR rightTxt, BOOL opaque, POPUPHANDLER fun)
{
	fp_popHandler = fun;
	if(messageTxt)
		m_TextBox.SetText(messageTxt);

	if(rightTxt)
	{
		m_RightButton.SetEnable(TRUE);
		m_RightButton.SetText(rightTxt);
		m_RightButton.SetShow(TRUE);
		m_RightButton.SetFocus(TRUE);
	}
	else
		m_RightButton.SetShow(FALSE);

	if(midTxt)
	{
		m_MiddleButton.SetEnable(TRUE);
		m_MiddleButton.SetText(midTxt);
		m_MiddleButton.SetShow(TRUE);
		m_MiddleButton.SetFocus(TRUE);
	}
	else
		m_MiddleButton.SetShow(FALSE);

	if(leftTxt)
	{
		m_LeftButton.SetEnable(TRUE);
		m_LeftButton.SetText(leftTxt);
		m_LeftButton.SetShow(TRUE);
		m_LeftButton.SetFocus(TRUE);
	}
	else
		m_LeftButton.SetShow(FALSE);

	if(opaque)
		m_BGHide.SetShow(TRUE);
	else
		m_BGHide.SetShow(FALSE);
	m_PopupScene.SetShow(TRUE);
	m_PopupScene.SetFocus();
}
