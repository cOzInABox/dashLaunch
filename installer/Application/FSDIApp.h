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
#include "FSDIUIApp.h"

//#define SHOW_FRAME_INFO 1

HRESULT XuiTextureLoader(IXuiDevice *pDevice, LPCWSTR szFileName, XUIImageInfo *pImageInfo, IDirect3DTexture9 **ppTex);

static const D3DVECTOR4 g_vClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
static const DWORD g_dwTileWidth   = 1280;
static const DWORD g_dwTileHeight  = 256;
static const DWORD g_dwFrameWidth  = 1280;
static const DWORD g_dwFrameHeight = 720;
static const D3DRECT g_dwTiles[3] = 
{
    {             0,              0,  g_dwTileWidth,  g_dwTileHeight },
    {             0, g_dwTileHeight,  g_dwTileWidth, g_dwTileHeight * 2 },
    {             0, g_dwTileHeight * 2,  g_dwTileWidth, g_dwFrameHeight },
};

class CFFSDIApp : public ATG::Application
{
public:
	static CFFSDIApp& GetInstance()
	{
		static CFFSDIApp singleton;
		return singleton;
	}


	
protected:
	CFFSDIApp();								// Private constructor
	~CFFSDIApp() {}								// Private destructor
	CFFSDIApp(const CFFSDIApp&);			// Prevent copy-construction
	CFFSDIApp& operator=(const CFFSDIApp&); // Prevent assignment

	// Rendering surfaces and textures
    D3DSurface*                 m_pBackBuffer;
    D3DSurface*                 m_pDepthBuffer;
    D3DTexture*                 m_pFrontBuffer[2];
	D3DTexture*					m_pFinalFrontBuffer;
    DWORD                       m_dwCurFrontBuffer;
	
	// ATG Application interface
	HRESULT Update();
	HRESULT Initialize();
	HRESULT Render();

private:
	// ATG Functionality for font and timer
	ATG::Timer m_FrameRateTimer;
	ATG::Timer m_CPUTimer;

#ifdef SHOW_FRAME_INFO
	ATG::Font m_OverlayFont;
	DWORD m_dwFrameCounter;
	DWORD m_dwAvailableRam;
	float m_fRenderTime;
	VOID RenderOverlays();
#endif

	// Rendering functions
	HRESULT RenderScene();
	HRESULT CreateRenderTargets( void );
};
