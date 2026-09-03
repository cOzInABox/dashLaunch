#include <xtl.h>
#include <assert.h>
#include <stdio.h>
#include <ppcintrinsics.h>
#include "kernel.h"
#include "mounts.h"
#include "util.h"

extern BOOL CntrlFtpd(int);
extern void FtpdSetDevices();

HANDLE m_hNotification;

MOUNTS mnt[] = {
//	{"fxcon:","", FALSE}, // EXTRA

	// hard disk
	{"fHdd:","\\Device\\Harddisk0\\Partition1", FALSE},
	{"fHddSys:","\\Device\\Harddisk0\\SystemPartition", FALSE},
	{"fHddSysEx:","\\Device\\Harddisk0\\SystemExtPartition", FALSE},
	{"fHddAux:","\\Device\\Harddisk0\\SystemAuxPartition", FALSE},

	// nand devices
	{"fFlash:","\\Device\\Flash", FALSE},
	{"fNandMu:","\\Device\\BuiltInMuSfc", FALSE},
	{"fNandMuSys:","\\Device\\BuiltInMuSfcSystem", FALSE},
	{"fNandMuSysEx:","\\Device\\FileSystemExtPartition2", FALSE},

	// slim internal mu
	{"fIntMu:","\\Device\\BuiltInMuUsb\\Storage", FALSE},
	{"fIntMuSys:","\\Device\\BuiltInMuUsb\\StorageSystem", FALSE},
	{"fIntMuSysEx:","\\Device\\BuiltInMuUsb\\SystemExtPartition", FALSE},

	// corona internal mu
	{"fMmcMu:","\\Device\\BuiltInMuMmc\\Storage", FALSE},
	{"fMmcMuSys:","\\Device\\BuiltInMuMmc\\StorageSystem", FALSE},
	{"fMmcMuSysEx:","\\Device\\BuiltInMuMmc\\SystemExtPartition", FALSE},

	// memory units
	{"fMu0:","\\Device\\Mu0", FALSE},
	{"fMu1:","\\Device\\Mu1", FALSE},
	
	// mass and usb mu 0
	{"fUsb0:","\\Device\\Mass0", FALSE},
	{"fUsbMu0:","\\Device\\Mass0PartitionFile\\Storage", FALSE},
	{"fUsbMu0Sys:","\\Device\\Mass0PartitionFile\\SystemPartition", FALSE},
	{"fUsbMu0SysExt:","\\Device\\Mass0PartitionFile\\SystemExtPartition", FALSE},
	{"fUsbMu0Aux:","\\Device\\Mass0PartitionFile\\SystemAuxPartition", FALSE},

	// mass and usb mu 1
	{"fUsb1:","\\Device\\Mass1", FALSE},
	{"fUsbMu1:","\\Device\\Mass1PartitionFile\\Storage", FALSE},
	{"fUsbMu1Sys:","\\Device\\Mass1PartitionFile\\SystemPartition", FALSE},
	{"fUsbMu1SysExt:","\\Device\\Mass1PartitionFile\\SystemExtPartition", FALSE},
	{"fUsbMu1Aux:","\\Device\\Mass1PartitionFile\\SystemAuxPartition", FALSE},

	// mass and usb mu 2
	{"fUsb2:","\\Device\\Mass2", FALSE},
	{"fUsbMu2:","\\Device\\Mass2PartitionFile\\Storage", FALSE},
	{"fUsbMu2Sys:","\\Device\\Mass2PartitionFile\\SystemPartition", FALSE},
	{"fUsbMu2SysExt:","\\Device\\Mass2PartitionFile\\SystemExtPartition", FALSE},
	{"fUsbMu2Aux:","\\Device\\Mass2PartitionFile\\SystemAuxPartition", FALSE},

	// DVD drive
	{"fCDvd:","\\Device\\Cdrom0", FALSE},
};
int nummnt = (sizeof(mnt)/sizeof(MOUNTS));
#define EXTRAMOUNTS 0

extern "C" const TCHAR szModuleName[] = TEXT( "ftpdll.dll" ); // probably don't need to export this

BOOL Mount(CHAR* szDrive, CHAR* szDevice) {

	char tmp[MAX_PATH];
	// Create a symbolic link to this drive
	if(ObMount(szDrive, szDevice) != S_OK)
		return FALSE;
	// Check if it exists now
	strcpy_s(tmp, MAX_PATH, szDrive);
	strcat_s(tmp, MAX_PATH, "\\");
	if(GetFileAttributes(tmp) == 0xFFFFFFFF)
	{
		deleteLink(szDrive);
		return FALSE;
	}
	return TRUE;
}

void setDeviceList(void)
{
	int i;
	for(i = EXTRAMOUNTS; i < nummnt; i++)
	{
		mnt[i].isMounted = Mount(mnt[i].name, mnt[i].path);
		if(mnt[i].isMounted)
		{
			//DbgPrint("mounted '%s' to '%s'\n", mnt[i].path, mnt[i].name);
			mnt[i].isMounted = TRUE;
		}
	}
}

void startFtp(void)
{
	//DbgPrint("start thread started\n");
	Sleep(0x1000);

	while((XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE) == 0)
		Sleep(500);
	while(CntrlFtpd(1) == 0)
	{
			Sleep(500);
	}
	setDeviceList();
	FtpdSetDevices();
	// Install removable device notification listener
	m_hNotification = XNotifyCreateListener(XNOTIFY_SYSTEM);
	if((m_hNotification != INVALID_HANDLE_VALUE) || (m_hNotification != NULL))
	{
		//DbgPrint( "listener handle good, %08x\n", m_hNotification); 
		while(m_hNotification != INVALID_HANDLE_VALUE)
		{
			DWORD dwNotificationId;
			ULONG ulParam;
			if( XNotifyGetNext( m_hNotification, 0, &dwNotificationId, &ulParam ) )
			{
				switch( dwNotificationId )
				{
					case XN_SYS_STORAGEDEVICESCHANGED:
						//DbgPrint("device change fired\n");
						setDeviceList();
						FtpdSetDevices();
						break;
				}
			}
			Sleep(0x1000);
		}
	}
	else
	{
		DbgPrint( "listener handle bad, %08x %d\n", m_hNotification, GetLastError()); 
	}
}

BOOL APIENTRY DllMain(HANDLE hInstDLL, DWORD reason, LPVOID lpReserved)
{
	switch(reason)
	{
		case DLL_PROCESS_ATTACH:
			HANDLE pthread;
			DWORD pthreadid;
			DWORD sta;
			//DbgPrint("ftpdllMain: DLL_PROCESS_ATTACH\n");
			// fire a thread to start it, in case it takes a long while...
			sta = ExCreateThread(&pthread, 0, &pthreadid, (VOID*) XapiThreadStartup , (LPTHREAD_START_ROUTINE)startFtp, 0, 0x2);
			break;
		case DLL_THREAD_ATTACH:
			//DbgPrint("ftpdllMain: DLL_THREAD_ATTACH\n");
			break;
		case DLL_THREAD_DETACH:
			//DbgPrint("ftpdllMain: DLL_THREAD_DETACH\n");
			break;
		case DLL_PROCESS_DETACH:
			//DbgPrint("ftpdllMain: DLL_PROCESS_DETACH\n");
			break;
	}
	return TRUE;
}


