#include <xtl.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif
#include "Application/FSDIApp.h"
#include "logging.h"

VOID __cdecl main()
{
#ifdef _DEBUG
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	DWORD dwLaunchDataSize = 0;    
	DWORD dwStatus = XGetLaunchDataSize( &dwLaunchDataSize );
	if(( dwStatus == ERROR_SUCCESS ) && (dwLaunchDataSize != 0))
	{
		BYTE* pLaunchData = new BYTE [ dwLaunchDataSize ];
		dwStatus = XGetLaunchData( pLaunchData, dwLaunchDataSize );
		if (pLaunchData) delete pLaunchData;
	}
#ifdef LOG_EXTRA_OUT
	lDbgPrint("installer started\n");
#endif
	SetUnhandledExceptionFilter(UnHandleExceptionFilter);

	ATG::GetVideoSettings( &(CFFSDIApp::GetInstance().m_d3dpp.BackBufferWidth), &(CFFSDIApp::GetInstance().m_d3dpp.BackBufferHeight) );

	CFFSDIApp::GetInstance().m_d3dpp.BackBufferCount        = 1;
	CFFSDIApp::GetInstance().m_d3dpp.MultiSampleType        = D3DMULTISAMPLE_4_SAMPLES;
	CFFSDIApp::GetInstance().m_d3dpp.EnableAutoDepthStencil = FALSE;
	CFFSDIApp::GetInstance().m_d3dpp.DisableAutoBackBuffer  = TRUE;
	CFFSDIApp::GetInstance().m_d3dpp.DisableAutoFrontBuffer = TRUE;
	CFFSDIApp::GetInstance().m_d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
	CFFSDIApp::GetInstance().m_d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_ONE;

	CFFSDIApp::GetInstance().m_dwDeviceCreationFlags      |= D3DCREATE_BUFFER_2_FRAMES | D3DCREATE_CREATE_THREAD_ON_0;
#ifdef LOG_EXTRA_OUT
	lDbgPrint("video set up, starting run loop\n");
#endif

	CFFSDIApp::GetInstance().Run();
}
