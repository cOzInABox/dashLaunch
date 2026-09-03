#pragma once
#include <xtl.h>
#include "xkelib.h"

#define DLDRIVE_NODISK		0
#define DLDRIVE_NOINI		1
#define DLDRIVE_INI			2
#define DLDRIVE_INI_CURR	0x80000000
#define DLDRIVE_VALID		(DLDRIVE_NODISK|DLDRIVE_NOINI|DLDRIVE_INI)

#define DLDRIVEMOUNT	"dldrive:"
#define DLDRIVE			"dldrive:\\"
#define DLDRIVEINI		"dldrive:\\launch.ini"

typedef struct _DL_DRIVE_ITEMS{
	DWORD deviceId;
	CHAR mountPoint[16];
	CHAR friendlyName[16];
	CHAR mountPath[MAX_PATH];
} DL_DRIVE_ITEMS, *PDL_DRIVE_ITEMS;

typedef struct _DL_DRIVE_LIST{
	DWORD numItems;
	PDL_DRIVE_ITEMS pItemps;
} DL_DRIVE_LIST, *PDL_DRIVE_LIST;

class dlDrives
{
public:	
	static dlDrives& GetInstance(){static dlDrives singleton; return singleton;}
	VOID RefreshList(VOID);
	VOID ShowListDebug(VOID);
	PCHAR GetDriveMount(DWORD item);
	PCHAR GetDriveFriendly(DWORD item);
	PCHAR GetDrivePath(DWORD item);
	DWORD GetNumDrives(VOID) {return m_dList.numItems;}
	DWORD GetNumIniDrives(VOID) {return m_maxIniDrives;}
	DWORD CheckIniStatus(DWORD item);
	DWORD CheckIniStatusNoUmt(DWORD item);
	BOOL IsDriveFlash(DWORD item);
	PCHAR MountDrive(DWORD item);
	BOOL MountDrive(DWORD item, PCHAR mountPoint);
	VOID UnMountDrive(DWORD item);
	VOID UnMountDrive(PCHAR mountPoint);
	VOID GetDriveSize(DWORD item, PDWORD lowSize, PDWORD hiSize);
	ULONGLONG GetDriveSize(DWORD item);
	ULONGLONG dlDrives::GetDriveSizeNoMount(DWORD item);
	VOID GetDriveSizeWchar(DWORD item, PWCHAR dest);
	VOID GetDriveSizeWcharNoMnt(DWORD item, PWCHAR dest);
	DWORD GetDriveDeviceId(DWORD item);
private:
	DL_DRIVE_LIST m_dList;
	DWORD m_maxIniDrives;
	VOID EnumDeviceId(VOID);

	dlDrives();
	~dlDrives() {}
	dlDrives(const dlDrives&);                 // Prevent copy-construction
	dlDrives& operator=(const dlDrives&);      // Prevent assignment
};


