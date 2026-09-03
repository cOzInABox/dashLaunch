#pragma once
#include "stdafx.h"
#include <PPCIntrinsics.h>
#include "FSDIApp.h"
#include "FSDIUIApp.h"
#include "optionsList.h"
#include "iniList.h"
#include "filerList.h"
#include "XboxUtil.h"
#include "Nand.h"
#include "Resource.h"
#include "installer.h"
#include "updSvr.h"
#include "logging.h"

extern InstallerMain* g_MainObject;

HRESULT CFSDIUIApp::InitializeUI( void * pInitData )
{
	DWORD size;
	PBYTE data;
	PWCHAR filename;
	HRESULT hr = S_FALSE;
	InitShared( CFFSDIApp::GetInstance().m_pd3dDevice, &(CFFSDIApp::GetInstance().m_d3dpp), (PFN_XUITEXTURELOADER)XuiTextureLoader );
#ifdef LOG_EXTRA_OUT
	lDbgPrint("InitializeUI: InitNetwork\n");
#endif
	XboxUtil::GetInstance().InitNetwork();
#ifdef LOG_EXTRA_OUT
	lDbgPrint("InitializeUI: usvrLoad\n");
#endif
	Updsvr::GetInstance().usvrLoad();
	// mount sysroot 
#ifdef LOG_EXTRA_OUT
	lDbgPrint("InitializeUI: mount sysroot\n");
#endif
	XboxUtil::GetInstance().MountPath("media:", "\\SystemRoot");
	XboxUtil::GetInstance().MountPath("sysmedia:", "\\Device\\Flash\\");
	XboxUtil::GetInstance().MountPath("xsep:", "\\sep");
#ifdef LOG_EXTRA_OUT
	lDbgPrint("InitializeUI: NAND init\n");
#endif
	Nand::GetInstance().Init();

//#ifdef LOG_EXTRA_OUT
//	DWORD config =  __loadvolatilewordbytereverse(0, (volatile u32*)(0x7feac000) );
//	lDbgPrint("flash config: %08x\n", config);
//#endif
#ifdef LOG_EXTRA_OUT
	lDbgPrint("InitializeUI: load font\n");
#endif

	data = (PBYTE)Resource::GetInstance().LoadExternalFile("game:\\font.ttf", &size);
	if(data != NULL)
	{
		WCHAR path[80];
		wsprintfW(path, L"memory://%08X,%X", data, size);
		hr = RegisterDefaultTypeface(L"EXTERNAL", path);
#ifdef LOG_EXTRA_OUT
		lDbgPrint("InitializeUI: external font found, register returns 0x%x\n", hr);
#endif
	}
	if(hr != 0)
	{
		filename = Resource::GetInstance().GetFilePath(L"font.ttf");
		if(Resource::GetInstance().IsXzpResourceExist(filename))
		{
			hr = RegisterDefaultTypeface(L"Skin", filename);
#ifdef LOG_EXTRA_OUT
			lDbgPrint("InitializeUI: skin font found at %S, register returns 0x%x\n", filename, hr);
#endif
		}
		delete [] filename;
	}
	if(hr != 0)
	{
		hr = RegisterDefaultTypeface(L"Xbox TC", L"file://media:\\xenonjklatin.xtt");
#ifdef LOG_EXTRA_OUT
		lDbgPrint("InitializeUI: no founts found, using flash font register returns 0x%x\n", hr);
#endif
	}

#ifdef LOG_EXTRA_OUT
	lDbgPrint("InitializeUI: locate skin\n");
#endif
	filename = Resource::GetInstance().GetFilePath(L"skin.xur");
#ifdef LOG_EXTRA_OUT
	lDbgPrint("InitializeUI: LoadSkin %s\n", filename);
#endif
	LoadSkin(filename, NULL);
	delete [] filename;

#ifdef LOG_EXTRA_OUT
	lDbgPrint("InitializeUI: LoadFirstScene\n");
#endif
	// Load the scene.
	LoadFirstScene(Resource::GetInstance().GetBasePath(), L"main.xur", NULL);

#ifdef LOG_EXTRA_OUT
	lDbgPrint("InitializeUI: Resume UI Rendering\n");
#endif
	// Resume UI Rendering
	Resume();

#ifdef LOG_EXTRA_OUT
	lDbgPrint("InitializeUI: complete\n");
#endif

	// Return Successfully
	return S_OK;
}

HRESULT CFSDIUIApp::UpdateUI( void * pUpdateData )
{
	// Run the next animation frame
	RunFrame();
	return S_OK;
}

HRESULT CFSDIUIApp::PreRenderUI( void *pRenderData )
{
	// Run any XUI Related pre render objects here
	PreRender();
	return S_OK;
}

HRESULT CFSDIUIApp::RenderUI(D3DDevice * pDevice, int nHOverscan, int nVOverscan, UINT uWidth, UINT uHeight)
{
	XuiTimersRun();
	XuiRenderBegin( GetDC(), D3DCOLOR_ARGB( 255, 0, 0, 0 ) );

	D3DXMATRIX matOrigView;
	D3DXMATRIX matOrigTrans;
	XuiRenderGetViewTransform( GetDC(), &matOrigView );

	// scale depending on the width of the render target
	D3DXMATRIX matView;
	int NewWidth = uWidth - (nHOverscan * 2);
	int NewHeight = uHeight - (nVOverscan * 2);
	D3DXVECTOR2 vScaling = D3DXVECTOR2( NewWidth / 1280.0f, NewHeight / 720.0f );
	D3DXVECTOR2 vTranslation = D3DXVECTOR2( (float)nHOverscan, (float)nVOverscan );
	D3DXMatrixTransformation2D( &matView, NULL, 0.0f, &vScaling, NULL, 0.0f, &vTranslation );
	XuiRenderSetViewTransform( GetDC(), &matView );

	XUIMessage msg;
	XUIMessageRender msgRender;
	XuiMessageRender( &msg, &msgRender, GetDC(), 0xffffffff, XUI_BLEND_NORMAL );
	XuiSendMessage( GetRootObj(), &msg );

	XuiRenderSetViewTransform( GetDC(), &matOrigView );

	XuiRenderEnd( GetDC() );

	return S_OK;
}

void CFSDIUIApp::DispatchXuiInput( XINPUT_KEYSTROKE *pKeystroke )
{
	// Send the keystroke to the xui message system for processing
	//XuiBroadcastMessage(GetRootObj(), &msg);
	if(pKeystroke->Flags & XINPUT_KEYSTROKE_KEYDOWN)
		g_MainObject->OnAnyKey(pKeystroke->VirtualKey);

	//	m_iob->OnAnyKey(pKeystroke->VirtualKey);
	XuiProcessInput(pKeystroke);
}

void CFSDIUIApp::RunFrame()
{
	CXuiModule::RunFrame();
}

HRESULT CFSDIUIApp::RegisterXuiClasses() 
{
	// Register Xui Sound and Video Systems
	XuiSoundXACTRegister();
	XuiSoundXAudioRegister();

	IniList::Register();
	OptionList::Register();
	FilerList::Register();
	InstallerMain::Register();

	// Return Successfully
	return S_OK;
}

HRESULT CFSDIUIApp::UnregisterXuiClasses()
{
	// Unregister Sound and video Systems
	XuiSoundXACTUnregister();
	XuiSoundXAudioUnregister();
	
	InstallerMain::Unregister();
	FilerList::Unregister();
	OptionList::Unregister();
	IniList::Unregister();
	
	// Return Successfully
	return S_OK;
}

HRESULT CFSDIUIApp::CreateMainCanvas()
{
	ASSERT( m_bXuiInited );
	if( !m_bXuiInited ) {
		return E_UNEXPECTED;
	}

	ASSERT( m_hObjRoot == NULL );
	if( m_hObjRoot ) {
		return E_UNEXPECTED;
	}

	HRESULT retVal = XuiCreateObject( L"XuiCanvas", &m_hObjRoot );
	if( FAILED( retVal ) )
		return retVal;

	return retVal;
}
