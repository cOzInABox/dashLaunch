#pragma once
#include <xtl.h>
#include <xui.h>
#include <xuiapp.h>

#define TIMER_XNOT_UPDATE	1
#define TIMER_TEMP_UPDATE	2
#define TIMER_SHOW_HELP		3
#define SHOW_HELP_TIME		7500

enum _SCENES {
	SCENE_PREVIOUS,
	SCENE_NONE,
	SCENE_MIN,
	SCENE_LIVEOPT,
	SCENE_INICONF,
	SCENE_MISC,
	SCENE_MAX,
	SCENE_DATAENTRY,
	SCENE_PATHBROWSER,
	SCENE_INFO,
};

typedef VOID (*POPUPHANDLER)(PVOID obj, DWORD opt);

class InstallerMain : public CXuiSceneImpl
{
private:
	HANDLE m_hNotify; // notification monitor
	// install status
	BOOL m_isInstalled; // tracks whether both lhelper and launch are installed
	BOOL m_isLhelperInstalled; // tracks whether lhelper is installed
	BOOL m_isUpToDate; // tracks whether it's up to date
	BOOL m_requiresReboot; // tracks whether changes to the current install require a reboot
	BOOL m_isRunning; // tracks whether launch.xex is in memory
	BOOL m_isKernelSupported; // tracks whether kernel is supported by this build
	BOOL m_arePatchesAvail; // tracks whether patches remain to be installed

	// some vars to help handle entry list/data
	BOOL m_HelpManual;
	int m_pressedIndex;
	CXuiControl m_pressedButton;

	// 3 button popups and dispatchers
	POPUPHANDLER fp_popHandler;
	static VOID popupReboot(PVOID obj,DWORD opt);
	static VOID popupInstall(PVOID obj,DWORD opt);
	static VOID popupPatches(PVOID obj, DWORD opt);
	static VOID popupPatches2(PVOID obj, DWORD opt);

	DWORD m_currScene;
	DWORD m_prevScene;
	BOOL m_filerIsLaunch;
	BOOL m_isHelpShown;
	BOOL m_isNetInit;
	BOOL m_isSvrInit;
	WCHAR m_kernelVer[64];
	//WCHAR m_wKeyboardBuf[MAX_PATH];
	//CHAR m_KeyboardBuf[MAX_PATH];
	VOID SetCurrentScene(DWORD scene);

	VOID UpdateInstallStatus(VOID);
	VOID SetTitleText(VOID);
	VOID UpdateTemps(VOID);
	VOID UpdateHardware(VOID);
	VOID Uninstall(VOID);
	VOID Install(BOOL BootTime);
	VOID LoadUnload(VOID);
	VOID ShowHelp();
	VOID ShowPopup(const PWCHAR messageTxt, const PWCHAR leftTxt, const PWCHAR midTxt, const PWCHAR rightTxt, BOOL opaque, POPUPHANDLER fun);
	VOID HandleFtp(DWORD toggle, DWORD dwPort, char* iptxt);
	VOID HandleUpdSvr(DWORD toggle, char* iptxt);
	VOID HandleNetwork();
	VOID HandleXnotify(VOID);
	VOID MiscSetText(VOID);

	//static VOID KeyboardCompletion(DWORD errCode, DWORD numBytes, PXOVERLAPPED pOv);
	//BOOL ProcessKeyboard(DWORD user, PWCHAR default, PWCHAR title);
	HRESULT PlayAnimation(PWCHAR szStartFrame, PWCHAR szEndFrame, BOOL bRecurse );

protected:
	// all scenes
	CXuiImageElement m_Background;
	CXuiTextElement m_TitleText;

	// temps box
	CXuiElement m_TempGroup;
	CXuiTextElement m_TempCPU;
	CXuiTextElement m_TempGPU;
	CXuiTextElement m_TempEDRAM;
	CXuiTextElement m_TempMOBO;

	// hardware info box
	CXuiElement m_HwGroup;
	CXuiTextElement m_HwBoard;// flash type kernel
	CXuiTextElement m_HwFlash;
	CXuiTextElement m_HwType;
	CXuiTextElement m_HwKernel;

	// popup scene
	CXuiScene m_PopupScene;
	CXuiElement m_BGHide;
	CXuiTextElement m_TextBox;
	CXuiControl m_MiddleButton;
	CXuiControl m_LeftButton;
	CXuiControl m_RightButton;

	// data entry scene
	CXuiScene m_DataScene;
	CXuiControl m_DataCancel;
	CXuiControl m_DataAccept;
	
	// file select scene
	CXuiScene m_FilerScene;
	CXuiTextElement m_FilerPath;
	CXuiTextElement m_FilerTitle;
	CXuiTextElement m_FilerInfo;
	CXuiList m_FilerList;

	// option list scene
	CXuiScene m_OptionScene;
	CXuiTextElement m_OptionTitle;
	CXuiTextElement m_OptionLast;
	CXuiTextElement m_OptionInfo;
	CXuiList m_OptionList;

	// ini list scene
	CXuiScene m_IniScene;
	CXuiTextElement m_IniTitle;
	CXuiTextElement m_IniInfo;
	CXuiTextElement m_IniCurrentIni;
	CXuiList m_IniList;

	// misc scene
	CXuiScene m_MiscScene;
	CXuiTextElement m_MiscTitle;
	CXuiTextElement m_MiscCurrentVer;
	CXuiControl m_MiscUnload;
	CXuiControl m_MiscInfo;
	CXuiControl m_MiscUninstall;
	CXuiControl m_MiscPatches;
	CXuiControl m_MiscInstallThis;
	CXuiControl m_MiscLaunch;
	CXuiControl m_MiscQuit;
	CXuiTextElement m_MiscDebug;

	// info scene
	CXuiScene m_InfoScene;
	CXuiTextElement m_InfoFtpInfo;
	CXuiTextElement m_InfoUpdsvrInfo;
	CXuiControl m_InfoSaveSmc;
	CXuiCheckbox m_CpuFanCheck;
	CXuiCheckbox m_GpuFanCheck;

	// help scene
	CXuiScene m_HelpScene;
	CXuiTextElement m_HelpAText;
	CXuiTextElement m_HelpBText;
	CXuiTextElement m_HelpXText;
	CXuiTextElement m_HelpYText;
	CXuiTextElement m_HelpRBText;
	CXuiTextElement m_HelpLBText;

	// Message map.
	XUI_BEGIN_MSG_MAP()
		XUI_ON_XM_INIT(OnInit)
		XUI_ON_XM_NOTIFY_PRESS(OnNotifyPress)
		XUI_ON_XM_TIMER(OnTimer)
		XUI_ON_XM_KEYUP(OnKeyUp)
		XUI_ON_XM_NOTIFY_VALUE_CHANGED(OnNotifyValueChanged)
	XUI_END_MSG_MAP()

	HRESULT OnKeyUp(XUIMessageInput *pInputData, BOOL& bHandled);
	HRESULT OnNotifyPress( HXUIOBJ hObjPressed, BOOL& bHandled );
	HRESULT OnInit( XUIMessageInit* pInitData, BOOL& bHandled );
	HRESULT OnTimer( XUIMessageTimer *pTimer, BOOL& bHandled );
	HRESULT OnNotifyValueChanged(HXUIOBJ hObjSource, XUINotifyValueChanged *pNotifyValueChangedData, BOOL& bHandled);

public:
	VOID OnAnyKey(DWORD key);

	// Define the class.
	XUI_IMPLEMENT_CLASS(InstallerMain, L"InstallerMain", XUI_CLASS_SCENE)
};
