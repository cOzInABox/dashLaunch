#pragma once
#include <xtl.h>
#include <xui.h>
#include <xuiapp.h>

class SysInfo
{
public:	
	static SysInfo& GetInstance(){static SysInfo singleton; return singleton;}
	VOID Init(CXuiScene sscene, CXuiControl ssave);
	HRESULT SliderChange(HXUIOBJ hObjSource, int newValue);
	HRESULT CheckChange(HXUIOBJ hObjSource);
	VOID SaveSmcChanges(VOID);

private:
	CXuiScene selfScene;
	CXuiControl m_SaveSmc;
	CXuiTextElement m_Title;
	CXuiTextElement m_SmcTitle;
	CXuiSlider m_CpuSlide; // text_Label
	CXuiSlider m_GpuSlide;
	CXuiSlider m_EdramSlide;
	CXuiSlider m_CpuFanSlide;
	CXuiSlider m_GpuFanSlide;
	CXuiCheckbox m_CpuFanCheck;
	CXuiCheckbox m_GpuFanCheck;
	WORD m_xcfs_sz;
	XCONFIG_STATIC_SETTINGS m_xcfs;

	BOOL m_fXconfGood;

	VOID DisableControls(VOID);
	VOID DisableTargets(VOID);
	VOID EnableTargets(VOID);

	SysInfo();
	~SysInfo() {}
	SysInfo(const SysInfo&);                 // Prevent copy-construction
	SysInfo& operator=(const SysInfo&);      // Prevent assignment
};

