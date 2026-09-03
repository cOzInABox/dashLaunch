#pragma once

#include <stdafx.h>
#include <xtl.h>
#include <xui.h>
#include <xuiapp.h>
#include <AtgApp.h>
#include <AtgFont.h>
#include <AtgInput.h>
#include <AtgMesh.h>
#include <AtgResource.h>
#include <AtgUtil.h>
#include <AtgDebugDraw.h>

class CFSDIUIApp : public CXuiModule
{

private:
	IXuiDevice * m_pXuiDevice;

public:
	CFSDIUIApp() {}
	~CFSDIUIApp() {}
	CFSDIUIApp(const CFSDIUIApp&);			// Prevent copy-construction
	CFSDIUIApp& operator=(const CFSDIUIApp&); // Prevent assignment

	static CFSDIUIApp& GetInstance()
	{
		static CFSDIUIApp singleton;
		return singleton;
	}

	IXuiDevice * getXuiDevice() { return m_pXuiDevice; }
    
	virtual void RunFrame();
	HRESULT DestroyMainScene();
	void DispatchXuiInput( XINPUT_KEYSTROKE* pKeystroke );
	HRESULT PreRenderUI( void *pRenderData );
	HRESULT RenderUI(D3DDevice * pDevice, int nHOverscan, int nVOverscan, UINT uWidth, UINT uHeight);
	HRESULT InitializeUI( void * pInitData );
	HRESULT UpdateUI( void * pUpdateData );

	HRESULT CreateMainCanvas();
	HRESULT RegisterXuiClasses();
	HRESULT UnregisterXuiClasses();
};