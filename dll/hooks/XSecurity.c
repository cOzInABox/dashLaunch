#include "_hook_includes.h"

#define XAM_VERIFY_CREATE_ORD			0x9BB // 2491 XamMediaVerificationCreate
#define XAM_VERIFY_CLOSE_ORD			0x9BC // 2492 XamMediaVerificationClose
#define XAM_VERIFY_VERIFY_ORD			0x9BD // 2493 XamMediaVerificationVerify
#define XAM_VERIFY_FAILED_ORD			0x9BE // 2494 XamMediaVerificationFailedBlocks

static BOOL isHooked = FALSE;
typedef DWORD (*MEDIAVER_CREATE)(DWORD dwHardwareThread);
typedef DWORD (*MEDIAVER_CLOSE)(void);
typedef DWORD (*MEDIAVER_VERIFY)(DWORD dwMilliseconds, LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
typedef DWORD (*MEDIAVER_FAILINF)(PXSECURITY_FAILURE_INFORMATION pFailureInformation);

MEDIAVER_CREATE MediaVerificationCreate = NULL;
MEDIAVER_CLOSE MediaVerificationClose = NULL;
MEDIAVER_VERIFY MediaVerificationVerify = NULL;
MEDIAVER_FAILINF MediaVerificationFailedBlocks = NULL;

//DWORD XamMediaVerificationCreate(DWORD dwHardwareThread) @2491
//DWORD XamMediaVerificationClose(void) @2492
//DWORD XamMediaVerificationVerify(DWORD dwMilliseconds, LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine) @2493
//DWORD XamMediaVerificationFailedBlocks(PXSECURITY_FAILURE_INFORMATION pFailureInformation) @2494
//XamMediaVerificationInject @2495 << disabled on retail

// Xam 0x9BB(Open)
NTSTATUS XSecurityOpenHook(DWORD dwHardwareThread)
{
	if(MediaVerificationCreate != NULL)
	{
#ifdef DEBUG_XSECURITY_OUT
		DWORD ret = MediaVerificationCreate(dwHardwareThread);
		dbgPrintFake("XSecurityOpenHook dw:%08x ret:%08x\n", dwHardwareThread, ret);
#else
		MediaVerificationCreate(dwHardwareThread);
#endif
	}
	return NO_ERROR;
}

// Xam 0x9BC(Close)
NTSTATUS XSecurityCloseHook(VOID)
{
	if(MediaVerificationClose != NULL)
	{
#ifdef DEBUG_XSECURITY_OUT
		DWORD ret = MediaVerificationClose();
		dbgPrintFake("XSecurityCloseHook ret: %x\n", ret);
#else
		MediaVerificationClose();
#endif
	}
	return NO_ERROR;
}

VOID myOvlComplete(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
#ifdef DEBUG_XSECURITY_OUT
	dbgPrintFake("verify complete, err:%x byte: %x lpo: %x\n", dwErrorCode, dwNumberOfBytesTransfered, lpOverlapped);
#endif
}

// Xam 0x9BD(Verify)
DWORD XSecurityVerifyHook(DWORD dwMilliseconds, LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	DWORD ret = 0;
	if(MediaVerificationVerify != NULL)
	{
		if(lpCompletionRoutine != NULL)
			ret = MediaVerificationVerify(0, NULL, (LPOVERLAPPED_COMPLETION_ROUTINE)myOvlComplete);
		else
			ret = MediaVerificationVerify(0, NULL, lpCompletionRoutine);
		//XSecurityVerifyHook ms: 500 lpo: 0 lpc: 821964e0 (821964e0)
#ifdef DEBUG_XSECURITY_OUT
		dbgPrintFake("XSecurityVerifyHook ms: %d lpo: %x lpc: %x ret: %x\n", dwMilliseconds, lpOverlapped, lpCompletionRoutine, ret);
#endif
	}
	if(lpCompletionRoutine)
		lpCompletionRoutine(0, 0, lpOverlapped);
	return NO_ERROR;
}

// Xam 0x9BE(GetFailures)
DWORD XSecurityGetFailureInfoHook(PXSECURITY_FAILURE_INFORMATION pFailureInformation)
{
#ifdef DEBUG_XSECURITY_OUT
	dbgPrintFake("XSecurityGetFailureInfoHook\n");
#endif
	if(pFailureInformation == NULL) 
		return NO_ERROR;
	if(pFailureInformation->dwSize == 0x14)
	{
		pFailureInformation->dwBlocksChecked = 0x64;
		pFailureInformation->dwFailedHashes = 0;
		pFailureInformation->dwFailedReads = 0;
		pFailureInformation->dwTotalBlocks = 0x64;
	}
	else if(pFailureInformation->dwSize == sizeof(XSECURITY_FAILURE_INFORMATION))
	{
		pFailureInformation->dwBlocksChecked = 0x100;
		pFailureInformation->dwFailedHashes = 0;
		pFailureInformation->dwFailedReads = 0;
		pFailureInformation->dwTotalBlocks = 0x100;
		pFailureInformation->fComplete = TRUE;
	}
	else
		return ERROR_NOT_ENOUGH_MEMORY;

	return NO_ERROR;
}

void XSecurityHook(void)
{
	if(isHooked == FALSE)
	{
		// turning off CIV... sorta
		MediaVerificationCreate = (MEDIAVER_CREATE)hookExportOrd(MODULE_XAM, XAM_VERIFY_CREATE_ORD, (DWORD)XSecurityOpenHook);
		MediaVerificationClose = (MEDIAVER_CLOSE)hookExportOrd(MODULE_XAM, XAM_VERIFY_CLOSE_ORD, (DWORD)XSecurityCloseHook);
		MediaVerificationVerify = (MEDIAVER_VERIFY)hookExportOrd(MODULE_XAM, XAM_VERIFY_VERIFY_ORD, (DWORD)XSecurityVerifyHook);
		MediaVerificationFailedBlocks = (MEDIAVER_FAILINF)hookExportOrd(MODULE_XAM, XAM_VERIFY_FAILED_ORD, (DWORD)XSecurityGetFailureInfoHook);
		isHooked = TRUE;
	}
}

void XSecurityUnhook(void)
{
	if(isHooked)
	{
		unhookExportOrd(MODULE_XAM, XAM_VERIFY_CREATE_ORD, (DWORD)MediaVerificationCreate);
		unhookExportOrd(MODULE_XAM, XAM_VERIFY_CLOSE_ORD, (DWORD)MediaVerificationClose);
		unhookExportOrd(MODULE_XAM, XAM_VERIFY_VERIFY_ORD, (DWORD)MediaVerificationVerify);
		unhookExportOrd(MODULE_XAM, XAM_VERIFY_FAILED_ORD, (DWORD)MediaVerificationFailedBlocks);
		isHooked = FALSE;
	}
}
