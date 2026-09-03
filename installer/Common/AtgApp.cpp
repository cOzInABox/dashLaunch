//--------------------------------------------------------------------------------------
// AtgApp.cpp
//
// Application class for samples
//
// Xbox Advanced Technology Group.
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "stdafx.h"
#include "AtgApp.h"
#include "AtgUtil.h"

// Ignore warning about "unused" pD3D variable
#pragma warning( disable: 4189 )

namespace ATG
{

//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------

// Global access to the main D3D device
D3DDevice* g_pd3dDevice = NULL;

// Private access to the main D3D device
D3DDevice* Application::m_pd3dDevice = NULL;

// The device-creation presentation params with reasonable defaults
D3DPRESENT_PARAMETERS Application::m_d3dpp =
{
    1280,                // BackBufferWidth;
    720,                // BackBufferHeight;
    D3DFMT_A8R8G8B8,    // BackBufferFormat;
    1,                  // BackBufferCount;
    D3DMULTISAMPLE_NONE,// MultiSampleType;
    0,                  // MultiSampleQuality;
    D3DSWAPEFFECT_DISCARD, // SwapEffect;
    NULL,               // hDeviceWindow;
    FALSE,              // Windowed;
    FALSE,               // EnableAutoDepthStencil;
    D3DFMT_D24S8,       // AutoDepthStencilFormat;
    0,                  // Flags;
    0,                  // FullScreen_RefreshRateInHz;
    D3DPRESENT_INTERVAL_DEFAULT, //D3DPRESENT_INTERVAL_IMMEDIATE, // FullScreen_PresentationInterval;
TRUE, // 		BOOL                            DisableAutoBackBuffer;      // Xbox 360 extension
TRUE, // 		BOOL                            DisableAutoFrontBuffer;     // Xbox 360 extension
( D3DFORMAT )MAKESRGBFMT( D3DFMT_LE_X8R8G8B8 ),// 		D3DFORMAT                       FrontBufferFormat;          // Xbox 360 extension
// 		D3DCOLORSPACE                   FrontBufferColorSpace;      // Xbox 360 extension
// 		D3DRING_BUFFER_PARAMETERS       RingBufferParameters;       // Xbox 360 extension
// 		D3DVIDEO_SCALER_PARAMETERS      VideoScalerParameters;      // Xbox 360 extension
};


// Extra flags to use at Direct3D device creation time
DWORD Application::m_dwDeviceCreationFlags = D3DCREATE_BUFFER_2_FRAMES | D3DCREATE_CREATE_THREAD_ON_0;

//--------------------------------------------------------------------------------------
// Name: Run()
// Desc: Creates the D3D device, calls Initialize() and enters an infinite loop
//       calling Update() and Render()
//--------------------------------------------------------------------------------------
VOID Application::Run()
{
    HRESULT hr;

    // Create Direct3D
    LPDIRECT3D9 pD3D = Direct3DCreate9( D3D_SDK_VERSION );

	//XVIDEO_MODE VideoMode;
	//ZeroMemory( &VideoMode, sizeof(VideoMode) );
	//XGetVideoMode( &VideoMode );
	//m_d3dpp.BackBufferWidth = VideoMode.dwDisplayWidth;
	//m_d3dpp.BackBufferHeight = VideoMode.dwDisplayHeight;

    // Create the D3D device
    if( FAILED( hr = pD3D->CreateDevice( 0, D3DDEVTYPE_HAL, NULL,
                                         m_dwDeviceCreationFlags,
                                         &m_d3dpp, ( ::D3DDevice** )&m_pd3dDevice ) ) )
    {
        ATG_PrintError( "Could not create D3D device!\n" );
        DebugBreak();
    }

    pD3D->Release();

    // Allow global access to the device
    g_pd3dDevice = m_pd3dDevice;

    // Initialize the app's device-dependent objects
    if( FAILED( hr = Initialize() ) )
    {
        ATG_PrintError( "Call to Initialize() failed!\n" );
        DebugBreak();
    }

    // Run the game loop
    for(; ; )
    {
        // Update the scene
        Update();

        // Render the scene
        Render();
    }
}

} // namespace ATG
