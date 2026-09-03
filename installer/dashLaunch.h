#ifndef __DASHLAUNCH_H
#define __DASHLAUNCH_H

#include <xtl.h>
#include "xkelib.h"
#include "..\_common.h"

// totalOpts - OUT if provided a pointer to a int it will provide the number of total options
// returns the total number of options
typedef int (*DLAUNCHGETNUMOPTS)(int* totalOpts);
// opt - IN, the number of the option you want info for
// optType - OUT when given a pointer to a DWORD will copy type as enumerated above in DL_OPT_TYPES
// outStr - OUT when given a pointer to a char array (will never need larger than 20 bytes) dash launch will copy the option ini name there
// currVal - OUT when given a pointer to a DWORD it will copy the current value of that option
// defValue - OUT when given a pointer to a DWORD it will copy the default/no ini found value of that option
// optMask - OUT when given a pointer to a DWORD it will copy the option mask (or 0 if there is none) that is reflected in ldata.options
typedef int (*DLAUNCHGETOPTINFO)(int opt, PDWORD optType, PCHAR outStr, PDWORD currVal, PDWORD defValue, PDWORD optMask);
// lets you get/set option values by their number, returns TRUE if successful and FALSE if not
// use dlaunchGetNumOpts and dlaunchGetOptInfo to parse option numbers which will likely change from release to release
typedef BOOL (*DLAUNCHGETOPTVAL)(int opt, PDWORD val);
typedef BOOL (*DLAUNCHSETOPTVAL)(int opt, PDWORD val);
// lets you get/set option values by their ini name, returns TRUE if successful and FALSE if not
typedef BOOL (*DLAUNCHGETOPTVALBYNAME)(char* optName, PDWORD val);
typedef BOOL (*DLAUNCHSETOPTVALBYNAME)(char* optName, PDWORD val);
// causes dash launch first run sequence to start, where it scans for an ini file, parses it an applies it
// path - OUT when given a pointer to a device path it will attempt to load that ini immediately
// note that runtime ini loads do not process ini items that have no use after boot time, including patches and plugins
// an acceptable path is a mount point like "\\Device\\Mass0\\", launch.ini will be sought at the root of that mount point
typedef VOID (*DLAUNCHFORCEINILOAD)(PCHAR path);
// can be used to start a system module from a system thread, returns the NTSTATUS of XexLoadModule
typedef DWORD (*DLAUNCHSTARTSYSMODULE)(char* modPath);
// causes dash launch to remove all its hooks in preparation for unloading
typedef VOID (*DLAUNCHSHUTDOWN)(VOID);
// allows one to fetch the drive list that dash launch uses internally, dev corresponds to keydata dev as well
// dev - IN the number of the device in the list
// devDest - OUT OPTIONAL destination where the path will be copied
// mountName - OUT OPTIONAL the mount name that corresponds to ini file path settings for this device (multiple devices of the same type share the same name)
// friendlyName - OUT OPTIONAL a unique mount name based on mountname when more than one type of a device is present
// returns total number in list; calling with (0, NULL, NULL) will fetch the list size
//		if requesting a dev that does not exist, returns 0
typedef DWORD (*DLAUNCHGETDRIVELIST)(DWORD dev, PCHAR devDest, PCHAR mountName, PCHAR friendlyName);
// gets info about the drive list in DashLaunch
// maxIniDrives - OUT OPTIONAL total number of drives in the list from first item capable of being scanned for ini files by dashlaunch
// maxDevLen - OUT OPTIONAL returns the longest strlen() of the device mount paths
// returns total number of drives in the list
typedef DWORD (*DLAUNCHGETDRIVEINFO)(PDWORD maxIniDrives, PDWORD maxDevLen);


class DashLaunch
{
// protected:
public:
	static DashLaunch& GetInstance(){static DashLaunch singleton; return singleton;}

	pldata m_ldat;
	DLAUNCHSTARTSYSMODULE dlaunchStartSysModule;
	DLAUNCHSHUTDOWN dlaunchShutdown;
	DLAUNCHFORCEINILOAD dlaunchForceIniLoad;
	DLAUNCHGETNUMOPTS dlaunchGetNumOpts;
	DLAUNCHGETOPTINFO dlaunchGetOptInfo;
	DLAUNCHGETOPTVAL dlaunchGetOptVal;
	DLAUNCHSETOPTVAL dlaunchSetOptVal;
	DLAUNCHGETOPTVALBYNAME dlaunchGetOptValByName;
	DLAUNCHSETOPTVALBYNAME dlaunchSetOptValByName;
	DLAUNCHGETDRIVELIST dlaunchGetDriveList;
	DLAUNCHGETDRIVEINFO dlaunchGetDriveInfo;

	BOOL isLoaded() {return m_isLoaded;} // whether launch.xex is running in memory
	BOOL isUpToDate() {return m_isUpToDate;} // whether launch.xex in memory is up to date
	BOOL canUseImports() {return m_importsGood;} // whether launch.xex in memory is V3/imports capable&resolved
	BOOL requiresReboot() {return m_requiresReboot;} // whether updating from V2 or older version
	PWCHAR getCurrDlVer() {return m_currVerString;}
	BOOL kernelVersionCheck(VOID);
	DWORD GetIniPathValue(VOID);
	BOOL IsUseFarenheit(VOID);
	PWCHAR getCurrIni(VOID);

	BOOL CheckLoad(VOID);
	VOID Unload(VOID);
	VOID Load(VOID);

	VOID LaunchItem(DWORD dev, DWORD flag, PCHAR path);

private:
	BOOL m_isLoaded;
	BOOL m_isUpToDate;
	BOOL m_importsGood;
	BOOL m_requiresReboot;
	WCHAR m_currVerString[64];
	WCHAR m_currIniPath[MAX_PATH];
	HANDLE m_handle;

	VOID ResolveEntities(VOID);
	BOOL CheckVersion(VOID);

	DashLaunch();
	~DashLaunch() {}
	DashLaunch(const DashLaunch&);                 // Prevent copy-construction
	DashLaunch& operator=(const DashLaunch&);      // Prevent assignment
};

#endif //__DASHLAUNCH_H
