#pragma once
#include "stdafx.h"
#include "FSDIApp.h"
#include "FSDIUIApp.h"


HRESULT XuiTextureLoader(IXuiDevice *pDevice, LPCWSTR szFileName, XUIImageInfo *pImageInfo, IDirect3DTexture9 **ppTex)
{
	int Len = wcslen(szFileName);
	if (szFileName[Len-3] == L'P' || szFileName[Len-3] == L'p')
		return XuiPNGTextureLoader(pDevice,szFileName,pImageInfo,ppTex);
	return XuiD3DXTextureLoader(pDevice,szFileName,pImageInfo,ppTex);
}

CFFSDIApp::CFFSDIApp( void )
{
}

HRESULT CFFSDIApp::CreateRenderTargets( void )
{
	HRESULT retVal;
	
	// Initialize D3DSurface Parameters
	D3DSURFACE_PARAMETERS ddsParams = { 0 };

	// Create RenderTarget for use with Predicated Tiling
	retVal = m_pd3dDevice->CreateRenderTarget( g_dwTileWidth, g_dwTileHeight, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_4_SAMPLES, 0, 0, &m_pBackBuffer, &ddsParams );
	if(retVal != D3D_OK) {
		return S_FALSE;
	}
			
	// Set up the surface size parameters
	ddsParams.Base = m_pBackBuffer->Size / GPU_EDRAM_TILE_SIZE;
	ddsParams.HierarchicalZBase = 0;

	// Create Depth Stencil Surface
	retVal = m_pd3dDevice->CreateDepthStencilSurface( g_dwTileWidth, g_dwTileHeight, D3DFMT_D24S8, D3DMULTISAMPLE_4_SAMPLES, 0, 0, &m_pDepthBuffer, &ddsParams );
	if(retVal != D3D_OK) {
		return S_FALSE;
	}

	// Create First Frame Buffer
	retVal = m_pd3dDevice->CreateTexture( g_dwFrameWidth, g_dwFrameHeight, 1, 0, D3DFMT_LE_X8R8G8B8, 0, &m_pFrontBuffer[0], NULL );
	if(retVal != D3D_OK) {
		return S_FALSE;
	}

	// Create Second Frame Buffer
	retVal = m_pd3dDevice->CreateTexture( g_dwFrameWidth, g_dwFrameHeight, 1, 0, D3DFMT_LE_X8R8G8B8, 0, &m_pFrontBuffer[1], NULL );
	if(retVal != D3D_OK) {
		return S_FALSE;
	}

	return S_OK;
}

HRESULT CFFSDIApp::Initialize()
{
	// Create Render Targets for use with Predicated Tiling
	CreateRenderTargets();

	// Initialize the XUI Class
	CFSDIUIApp::GetInstance().InitializeUI(NULL);
#ifdef SHOW_FRAME_INFO
	m_dwFrameCounter = 0;
	if(m_OverlayFont.Create("game:\\Arial_12.xpr" ) == S_OK )
		m_OverlayFont.SetWindow(ATG::GetTitleSafeArea());
#endif

	return S_OK;
}

HRESULT CFFSDIApp::Update()
{
	// Retrieve and dispatch input to the UI
	XINPUT_KEYSTROKE keyStroke;
	XInputGetKeystroke( XUSER_INDEX_ANY, XINPUT_FLAG_ANYDEVICE, &keyStroke );

	if(keyStroke.Flags&(XINPUT_KEYSTROKE_KEYDOWN|XINPUT_KEYSTROKE_KEYUP|XINPUT_KEYSTROKE_REPEAT))
		CFSDIUIApp::GetInstance().DispatchXuiInput(&keyStroke);
	//if(keyStroke.VirtualKey == VK_PAD_LTHUMB_PRESS) {
	//	
	//	XINPUT_CAPABILITIES caps;
	//	XInputGetCapabilities(keyStroke.UserIndex, keyStroke.Flags, &caps);
	//} else {
	//	CFSDIUIApp::GetInstance().DispatchXuiInput( &keyStroke );
	//}

	// Update the next Xui Frame
	CFSDIUIApp::GetInstance().UpdateUI(NULL);

	return S_OK;
}

HRESULT CFFSDIApp::Render()
{
	// Pre Render any XUI effects or shadow effects prior to rendering the scene
	CFSDIUIApp::GetInstance().PreRenderUI(NULL);

	// Render the entire screen to the backbuffer
	RenderScene();

	// Synchronize the buffer to the presentation interval to avoid screen tearings
	m_pd3dDevice->SynchronizeToPresentationInterval();
	
	// Present the back buffer and swap frame buffers to continue rendering
	m_pd3dDevice->Swap( m_pFrontBuffer[ m_dwCurFrontBuffer ], NULL );
	m_dwCurFrontBuffer = ( m_dwCurFrontBuffer + 1 ) % 2;

	return S_OK;
}

HRESULT CFFSDIApp::RenderScene()
{
	// Reset the CPU Processing Timer
	m_CPUTimer.Reset();

	m_pd3dDevice->SetRenderTarget( 0, m_pBackBuffer );
	m_pd3dDevice->SetDepthStencilSurface( m_pDepthBuffer );

	// Begin Rendering our Scene and all of it's components
	m_pd3dDevice->BeginTiling( 0, ARRAYSIZE(g_dwTiles), g_dwTiles, &g_vClearColor, 1, 0 );
	{
		// Render XUI to the device
		CFSDIUIApp::GetInstance().RenderUI( m_pd3dDevice, 0, 0, g_dwFrameWidth, g_dwFrameHeight );
#ifdef SHOW_FRAME_INFO
		RenderOverlays();
#endif
	}
	m_pd3dDevice->EndTiling( 0, NULL, m_pFrontBuffer[m_dwCurFrontBuffer], NULL, 1, 0, NULL );

	// Return Successfully
	return S_OK;
}

#ifdef SHOW_FRAME_INFO
VOID CFFSDIApp::RenderOverlays()
{
	WCHAR szText[100];

	// Mark current statistics for calculations
	m_fRenderTime = (FLOAT) m_CPUTimer.GetElapsedTime();
	m_FrameRateTimer.MarkFrame();
	m_dwFrameCounter++;

	// Store available ram for display
	if(m_dwFrameCounter > 240)
	{
		MEMORYSTATUS memStat;
		GlobalMemoryStatus(&memStat);
		m_dwAvailableRam = memStat.dwAvailPhys;

		// Reset Frame Counter
		m_dwFrameCounter = 0;
	}

	// Begin Rendering Font
	m_OverlayFont.Begin();

	// Display Frames Per Second
	m_OverlayFont.SetScaleFactors( 1.0f, 1.0f );
	m_OverlayFont.DrawText( 0, 0, 0xFFFFFF00, m_FrameRateTimer.GetFrameRate(), ATGFONT_RIGHT );

	// Display Free Memory
	m_OverlayFont.SetScaleFactors( 1.0f, 1.0f );
	swprintf_s(szText, L"Free Mem:  %d bytes", m_dwAvailableRam );
	m_OverlayFont.DrawText( 0, 20, 0xFFFFFF00, szText, ATGFONT_RIGHT );

	m_OverlayFont.End();
}
#endif

