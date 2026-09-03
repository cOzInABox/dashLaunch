#include "dlDrives.h"
#include <stdio.h>
#include "dashLaunch.h"
#include "XboxUtil.h"
#include "Strings.h"
#include "logging.h"

typedef struct _INST_DRIVES {
	char* mPoint; // mount point
	char* mFriend; // friendly name
	char* mPath; // mount path
} INST_DRIVES, *PINST_DRIVES;

static char invalidDriveName[60] = "\\Unavailable\\";
static char invalidMountName[60] = "Unavailable";

INST_DRIVES idAlt [] = {
	{"Usb",		"Usb0:",	"\\Device\\Mass0\\"},
	{"Usb",		"Usb1:",	"\\Device\\Mass1\\"},
	{"Usb",		"Usb2:",	"\\Device\\Mass2\\"},
	{"Hdd:",	"Hdd:",		"\\Device\\Harddisk0\\Partition1\\"},
	{"FlashMu:","NandMu:",	"\\Device\\BuiltInMuSfc\\"}, // flash memory unit big block jasper.. maybe others?
	{"IntMu:",	"SlimMu:",	"\\Device\\BuiltInMuUsb\\Storage\\"}, // flash memory unit slim arcade internal
	{"MmcMu:",	"MmcMu:",	"\\Device\\BuiltInMuMmc\\Storage\\"}, // flash memory unit slim corona 4g arcade internal
	{"Sfc:",	"Flash:",	"\\SystemRoot\\"},
	{"Mu:",		"Mu0:",		"\\Device\\Mu0\\"},
	{"Mu:",		"Mu1:",		"\\Device\\Mu1\\"},
	{"UsbMu:",	"UsbMu0:",	"\\Device\\Mass0PartitionFile\\Storage\\"},
	{"UsbMu:",	"UsbMu1:",	"\\Device\\Mass1PartitionFile\\Storage\\"},
	{"UsbMu:",	"UsbMu2:",	"\\Device\\Mass2PartitionFile\\Storage\\"},
	{"Xfer:",	"Xfer:",	"\\Device\\TransferCable\\"},
	{"Dvd:",	"Dvd:",		"\\Device\\Cdrom0\\"},
};

#define NUM_ALT_DRIVES (sizeof(idAlt)/sizeof(INST_DRIVES))

dlDrives::dlDrives(VOID)
{
	LPCWSTR wtmp = Strings::GetInstance().Lookup(L"none");
	if(wtmp)
	{
		sprintf_s(invalidDriveName, "\\%S\\", wtmp);
		sprintf_s(invalidMountName, "%S:", wtmp);
	}
	m_dList.numItems = 0;
	m_dList.pItemps = NULL;
}

VOID dlDrives::EnumDeviceId(VOID)
{
	DWORD ret, i;
	HANDLE hEnum;
	XDEVICE_DATA devi;
	DWORD enumCnt =0;
	DWORD pcbbuf = 0; // outputs to; how big 1 item of the enum type is
	memset(&devi, 0x0, (sizeof(XDEVICE_DATA)));
	ret = XContentCreateDeviceEnumerator(XCONTENTDEVICE_ALL, XCONTENTDEVICE_ALL, 1, &pcbbuf, &hEnum);
	if(ret == ERROR_SUCCESS)
	{
		while(XEnumerate(hEnum, &devi, sizeof(XDEVICE_DATA), &enumCnt, NULL) == ERROR_SUCCESS)
		{
			CHAR path[MAX_PATH];
#ifdef LOG_EXTRA_OUT
			lDbgPrint("EnumDeviceId: ID: %08x type: %08x MiB: %u free %u\n", devi.DeviceID, devi.DeviceType , (devi.ulDeviceBytes/1048576) , (devi.ulDeviceFreeBytes/1048576));
			lDbgPrint("EnumDeviceId: name: %S\n",devi.wszFriendlyName);
#endif
			if(XamContentGetDeviceVolumePath(devi.DeviceID, path, MAX_PATH, FALSE) == ERROR_SUCCESS)
			{
				//lDbgPrint("path(t): %s ret %08x\n", path, ret);
				for(i = 0; i < m_dList.numItems; i++)
				{
					if(stricmp(path, m_dList.pItemps[i].mountPath) == 0)
					{
#ifdef LOG_EXTRA_OUT
						lDbgPrint("EnumDeviceId: setting device %s ID to 0x%08x\n", m_dList.pItemps[i].mountPath, devi.DeviceID);
#endif
						m_dList.pItemps[i].deviceId = devi.DeviceID;
					}
				}
			}
		}
#ifdef LOG_EXTRA_OUT
		lDbgPrint("EnumDeviceId: enum ended\n");
#endif
		CloseHandle(hEnum);
	}
	else
		lDbgPrint("EnumDeviceId: error creating enumerator\n");
}

VOID dlDrives::RefreshList(VOID)
{
	DWORD i;
	if(m_dList.pItemps != NULL)
	{
		delete [] m_dList.pItemps;
		m_dList.pItemps = NULL;
	}
	m_dList.numItems = 0;
	if(DashLaunch::GetInstance().canUseImports())
	{
		DWORD items = DashLaunch::GetInstance().dlaunchGetDriveInfo(&m_maxIniDrives, NULL);
#ifdef LOG_EXTRA_OUT
		lDbgPrint("RefreshList: dl drives: %d\n", items);
#endif
		m_dList.pItemps = new DL_DRIVE_ITEMS[items];
		for(i = 0; i < items; i++)
		{
			m_dList.pItemps[i].deviceId = 0xFFFFFFFF;
			DashLaunch::GetInstance().dlaunchGetDriveList(i, m_dList.pItemps[i].mountPath, m_dList.pItemps[i].mountPoint, m_dList.pItemps[i].friendlyName);
#ifdef LOG_EXTRA_OUT
			lDbgPrint("RefreshList: added %d '%s' at '%s' (%s)\n", i, m_dList.pItemps[i].mountPoint, m_dList.pItemps[i].mountPath, m_dList.pItemps[i].friendlyName);
#endif
		}
		m_dList.numItems = items;
	}
	else
	{
		DWORD items = NUM_ALT_DRIVES;
#ifdef LOG_EXTRA_OUT
		lDbgPrint("RefreshList: fake drives: %d\n", items);
#endif
		m_dList.pItemps = new DL_DRIVE_ITEMS[items];
		for(i = 0; i < items; i++)
		{
			m_dList.pItemps[i].deviceId = 0xFFFFFFFF;
			strcpy(m_dList.pItemps[i].mountPath, idAlt[i].mPath);
			strcpy(m_dList.pItemps[i].mountPoint, idAlt[i].mPoint);
			strcpy(m_dList.pItemps[i].friendlyName, idAlt[i].mFriend);
#ifdef LOG_EXTRA_OUT
			lDbgPrint("RefreshList: added %d '%s' at '%s' (%s)\n", i, m_dList.pItemps[i].mountPoint, m_dList.pItemps[i].mountPath, m_dList.pItemps[i].friendlyName);
#endif
		}
		m_dList.numItems = items;
	}
	if(m_dList.numItems != 0)
	{
#ifdef LOG_EXTRA_OUT
		lDbgPrint("RefreshList: enumerating IDs\n");
#endif
		EnumDeviceId();
	}
} 

VOID dlDrives::ShowListDebug(VOID)
{
	DWORD i;
	for(i = 0; i < m_dList.numItems; i++)
	{
		lDbgPrint("show %d '%s' at '%s'\n", i, m_dList.pItemps[i].mountPoint, m_dList.pItemps[i].mountPath);
	}
}

VOID dlDrives::GetDriveSize(DWORD item, PDWORD lowSize, PDWORD hiSize)
{
	ULARGE_INTEGER diskSize;
	diskSize.QuadPart = 0ULL;
	if(item > m_dList.numItems)
		return;
	XboxUtil::GetInstance().MountPath(DLDRIVEMOUNT, m_dList.pItemps[item].mountPath);
	GetDiskFreeSpaceExA(DLDRIVE, NULL, &diskSize, NULL);
#ifdef LOG_EXTRA_OUT
	lDbgPrint("GetDriveSize: drive %s space %016I64x\n", m_dList.pItemps[item].mountPath, diskSize.QuadPart);
#endif
	XboxUtil::GetInstance().DeleteLink(DLDRIVEMOUNT);
	*lowSize = diskSize.LowPart;
	*hiSize = diskSize.HighPart;
}

ULONGLONG dlDrives::GetDriveSize(DWORD item)
{
	ULARGE_INTEGER diskSize;
	diskSize.QuadPart = 0ULL;
	if(item > m_dList.numItems)
		return 0;
	XboxUtil::GetInstance().MountPath(DLDRIVEMOUNT, m_dList.pItemps[item].mountPath);
	GetDiskFreeSpaceExA(DLDRIVE, NULL, &diskSize, NULL);
#ifdef LOG_EXTRA_OUT
	lDbgPrint("GetDriveSize2: drive %s space %016I64x\n", m_dList.pItemps[item].mountPath, diskSize.QuadPart);
#endif
	XboxUtil::GetInstance().DeleteLink(DLDRIVEMOUNT);
	return diskSize.QuadPart;
}

ULONGLONG dlDrives::GetDriveSizeNoMount(DWORD item)
{
	ULARGE_INTEGER diskSize;
	diskSize.QuadPart = 0ULL;
	GetDiskFreeSpaceExA(DLDRIVE, NULL, &diskSize, NULL);
#ifdef LOG_EXTRA_OUT
	lDbgPrint("GetDriveSizeNoMount: drive %s space %016I64x\n", m_dList.pItemps[item].mountPath, diskSize.QuadPart);
#endif
	return diskSize.QuadPart;
}

VOID dlDrives::GetDriveSizeWchar(DWORD item, PWCHAR dest)
{
	if(item > m_dList.numItems)
	{
		wcscpy(dest, L"<Invalid>");
		return;
	}
	else
	{
		long double sz = (long double)GetDriveSize(item);
		if(sz < 1024.0)
			wsprintfW(dest, L"%4.1f%s", sz, Strings::GetInstance().Lookup(L"symbytes"));
		else // bytes to kilobytes
		{
			sz = sz/1024;
			if(sz < 1024.0)
				wsprintfW(dest, L"%4.1f%s", sz, Strings::GetInstance().Lookup(L"symkiloby"));
			else // kilobytes to MB
			{
				sz = sz/1024.0;
				if(sz < 1024.0)
					wsprintfW(dest, L"%4.1f%s", sz, Strings::GetInstance().Lookup(L"symmegaby"));
				else // MB to GB
				{
					sz = sz/1024.0;
					wsprintfW(dest, L"%4.1f%s", sz, Strings::GetInstance().Lookup(L"symgigaby"));
				}
			}
		}
	}
}

VOID dlDrives::GetDriveSizeWcharNoMnt(DWORD item, PWCHAR dest)
{
	if(item > m_dList.numItems)
	{
		wcscpy(dest, L"<Invalid>");
		return;
	}
	else
	{
		long double sz = (long double)GetDriveSizeNoMount(item);
		if(sz < 1024.0)
			wsprintfW(dest, L"%4.1f%s", sz, Strings::GetInstance().Lookup(L"symbytes"));
		else // bytes to kilobytes
		{
			sz = sz/1024;
			if(sz < 1024.0)
				wsprintfW(dest, L"%4.1f%s", sz, Strings::GetInstance().Lookup(L"symkiloby"));
			else // kilobytes to MB
			{
				sz = sz/1024.0;
				if(sz < 1024.0)
					wsprintfW(dest, L"%4.1f%s", sz, Strings::GetInstance().Lookup(L"symmegaby"));
				else // MB to GB
				{
					sz = sz/1024.0;
					wsprintfW(dest, L"%4.1f%s", sz, Strings::GetInstance().Lookup(L"symgigaby"));
				}
			}
		}
	}
}

PCHAR dlDrives::GetDriveMount(DWORD item)
{
	if(item > m_dList.numItems)
		return invalidMountName;
	else
		return m_dList.pItemps[item].mountPoint;
}

PCHAR dlDrives::GetDriveFriendly(DWORD item)
{
	if(item > m_dList.numItems)
		return invalidMountName;
	else
		return m_dList.pItemps[item].friendlyName;
}

BOOL dlDrives::IsDriveFlash(DWORD item)
{
	BOOL ret = FALSE;
	if(item < m_dList.numItems)
	{
		if(stricmp(m_dList.pItemps[item].mountPath, "\\Device\\Flash\\") == 0)
			ret = TRUE;
		if(stricmp(m_dList.pItemps[item].mountPath, "\\SystemRoot\\") == 0)
			ret = TRUE;
	}
	return ret;
}

PCHAR dlDrives::GetDrivePath(DWORD item)
{
	if(item > m_dList.numItems)
		return invalidDriveName;
	else
		return m_dList.pItemps[item].mountPath;
}

DWORD dlDrives::CheckIniStatus(DWORD item)
{
	DWORD ret = DLDRIVE_NODISK;
	XboxUtil::GetInstance().MountPath(DLDRIVEMOUNT, m_dList.pItemps[item].mountPath);
	if(XboxUtil::GetInstance().IsDriveExist(DLDRIVE))
	{
		ret = DLDRIVE_NOINI;
		if(XboxUtil::GetInstance().IsFileExist(DLDRIVEINI))
			ret = DLDRIVE_INI;
	}
	if(DashLaunch::GetInstance().GetIniPathValue() == item)
		ret |= DLDRIVE_INI_CURR;
	XboxUtil::GetInstance().DeleteLink(DLDRIVEMOUNT);
	return ret;
}

DWORD dlDrives::CheckIniStatusNoUmt(DWORD item)
{
	DWORD ret = DLDRIVE_NODISK;
	XboxUtil::GetInstance().MountPath(DLDRIVEMOUNT, m_dList.pItemps[item].mountPath);
	if(XboxUtil::GetInstance().IsDriveExist(DLDRIVE))
	{
		ret = DLDRIVE_NOINI;
		if(XboxUtil::GetInstance().IsFileExist(DLDRIVEINI))
			ret = DLDRIVE_INI;
	}
	if(DashLaunch::GetInstance().GetIniPathValue() == item)
		ret |= DLDRIVE_INI_CURR;
	return ret;
}

PCHAR dlDrives::MountDrive(DWORD item)
{
	if(item < m_dList.numItems)
	{
		XboxUtil::GetInstance().MountPath(m_dList.pItemps[item].friendlyName, m_dList.pItemps[item].mountPath);
		if(XboxUtil::GetInstance().IsDriveExist(m_dList.pItemps[item].friendlyName))
			return m_dList.pItemps[item].friendlyName;
		XboxUtil::GetInstance().DeleteLink(m_dList.pItemps[item].friendlyName);
	}
	return NULL;
}

BOOL dlDrives::MountDrive(DWORD item, PCHAR mountPoint)
{
	if(item < m_dList.numItems)
	{
		XboxUtil::GetInstance().MountPath(mountPoint, m_dList.pItemps[item].mountPath);
		if(XboxUtil::GetInstance().IsDriveExist(mountPoint))
			return TRUE;
		XboxUtil::GetInstance().DeleteLink(mountPoint);
	}
	return FALSE;
}

VOID dlDrives::UnMountDrive(DWORD item)
{
	if(item < m_dList.numItems)
	{
		XboxUtil::GetInstance().DeleteLink(m_dList.pItemps[item].friendlyName);
	}
}

VOID dlDrives::UnMountDrive(PCHAR mountPoint)
{
	XboxUtil::GetInstance().DeleteLink(mountPoint);
}

DWORD dlDrives::GetDriveDeviceId(DWORD item)
{
	DWORD ret = 0;
	if(item < m_dList.numItems)
	{
		ret = m_dList.pItemps[item].deviceId;
	}
	return ret;
}
