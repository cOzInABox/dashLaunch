/* protos and defines for (only) launch.c */

#ifndef _LAUNCH_H_
#define _LAUNCH_H_

#include "opts.h"

// if it wasn't for xbox1 emulator needing protection off, these could be done in DWORD arrays
#define LAUNCHTITLEEX_VAL		0
#define XNOTIFYBROADCAST_VAL	1
#define LOADPREPSAVE_VAL		2
#define LICMASK_VAL				3
#define LICHOOK_VAL				4
#define BUGCHECK_VAL			5
#define XNETDNS_VAL				6
#define XAMAPPLOAD_VAL			7
#define XAMSIGNINSTATE_VAL		8
#define XAMSIGNINGINFO_VAL		9
#define XAMLIVEHIVE_VAL			10
#define XAMLIVEHIVEA_VAL		11
#define XEXVERIFYHEAD_VAL		12
#define XEXLOADIMAGE_VAL		13
#define HALRETURNFIRM_VAL		14
#define EXCEPTTRAP_VAL			15

typedef struct _XHTTP_PATCH_ADDR {
	DWORD XampXAuthStartup;
	DWORD XampXAuthStartupDest;
	PDWORD XamWaitForNSAL;
	PDWORD XamRequestToken;
	PDWORD LookupAppliesTo;
	PDWORD XAuthValidateURL;
} XHTTP_PATCH_ADDR, *PXHTTP_PATCH_ADDR;

typedef struct _KERN_PATCH_ADDR {
	DWORD DebugRoutine;		// KiDebugRoutine->KdpTrap
	DWORD DebugTrap;		// KdpTrap
	DWORD DebugStepPatch;	// KiDebugRoutine address to patch out 'skip step exceptions'
} KERN_PATCH_ADDR, *PKERN_PATCH_ADDR;

typedef struct _PATCH_ADDRS {
	USHORT kernelBuild;
	USHORT kpatch;
	PXHTTP_PATCH_ADDR pxhttpPat;
	DWORD LoaderPrepare;	// TitleLoaderPrepareLoadExecutableFile
	DWORD PingLimit;		// PingLimitPatch
	DWORD LicenseCheck;		// ContentEvaluateLicense XContent::ContentEvaluateLicense
	DWORD HudDisable;		// XenonButton handler, patch xamapprequestloadex call, seek hud.xex string, 4th from the bottom
	DWORD XamRevokePatch;   // DetermineTitleParamsFromXexHeader
	PDWORD XamContLivePirs; // XContent__EvaluateContent
	PDWORD XamContDevice;   // XContent__EvaluateContent
	PDWORD XamKinectHealth; // XamSetKinectHealthReminderState call
	PDWORD NoNewUpdate;		// XampSystemUpdateIsPresent
} PATCH_ADDRS, *PPATCH_ADDRS;

#define PLUGIN_LOAD_PATH_MAGIC 0x504C5041 // 'PLPA' (PLugin PAth)
typedef struct _PLUGIN_LOAD_PATH{
	DWORD magic; // will be PLUGIN_LOAD_PATH_MAGIC _ONLY_ when other items are stuffed
	const char* devicePath;
	const char* iniPath;
}PLUGIN_LOAD_PATH, *PPLUGIN_LOAD_PATH;


/* a useful struct template for an import pointer */
#define XBOX_HW_FLAG_HDD		0x00000020 // if this bit is set the box has a hdd

// dummy DWORD pad defs for console power button and guide power button
#define XINPUT_DUMMY_CONSPWR	0x00010000
#define XINPUT_DUMMY_GUIDEPWR	0x00020000
#define XINPUT_DUMMY_CONFIG		0x00040000
#define XINPUT_DUMMY_FAKEANIM	0x00080000
#define XINPUT_DUMMY_FORCE_DASH 0x00100000
// all usable quick launch buttons as the controller transmits them...
#define XINPUT_ALLBUTTONSMASK (									\
	XINPUT_GAMEPAD_A|XINPUT_GAMEPAD_B|XINPUT_GAMEPAD_X|			\
	XINPUT_GAMEPAD_Y|XINPUT_GAMEPAD_START|XINPUT_GAMEPAD_BACK|	\
	XINPUT_GAMEPAD_LEFT_SHOULDER|XINPUT_GAMEPAD_RIGHT_SHOULDER)

typedef struct {
	const char* iniName;
	const int nextCat;
	const char* friendlyName;
	const char* mountPath;
} DRIVE_LIST, *PDRIVE_LIST;

typedef struct _BlockRule {
	char* startsWith;
	char * endsWith;
} BlockRule;

typedef struct _DnsBlocks {
	int numRules;
	BlockRule* bRules;
} DnsBlocks;

/* defines, enums and names for the devices launch will be using */
#define MOUNT_HDD			3
#define MOUNT_TRIN_INTMU	5
#define MOUNT_MAX_INI		8

#define DUMP_PATH		3
#define MOUNT_DUMP		"dlcrash:"
#define MOUNT_ALIVE		"alive:"
#define BUTTON_KEY		"QuickLaunchButtons"
#define PLUGIN_KEY		"Plugins"
#define SETTING_KEY		"Settings"
#define INI_NAME		"\\launch.ini"
#define PATCH_NAME		"\\kxam.patch"
#define TYPE_STR_LIVE	"LIVE"
#define TYPE_STR_XEX	"XEX2"
#define XBOX_XEX		"\\Device\\Harddisk0\\SystemPartition\\Compatibility"
#define DASH_XEX		"\\SystemRoot\\dash.xex"
#define HELPER_XEX		"\\Device\\Flash\\lhelper.xex"
#define LIVE_REPL		"localhost"

// max number of u32's that will be allocated for loading kxam.patch
#define MAX_PATCH_SZ	0x1000

/* state defines */
#define PATCHSTATE_WAITING  0
#define PATCHSTATE_RUNNING  1
#define PATCHSTATE_COMPLETE 2

/* Functions */
DWORD getButtons(DWORD reps, BOOL stall, WORD debounce);
char* enumButtons(DWORD pad, char* xex);
BOOL isXexDash(const char* xexName);
DWORD mountToPath(DWORD mountItem, char* mountName);
const char* getMountPathItem(int num);
void showLaunchData(void);
BOOL getLaunchData(void);
BOOL checkLaunchData(const char * arg1, const char * arg2, const char * arg3, DWORD flags);
DWORD LaunchTitleEx(const char * arg1, const char * arg2, const char * arg3, DWORD flags);
void loadPlugins(void);
void doPatches(void);
BOOL loadPatches(char* path);
void setupPkeyPath(const char* inistr, pkeydata pkey, DWORD type);
void seekIniPatch(PBOOL config, PBOOL patch);
void runtimeRunTasks(PCHAR path);
char* firstRunTasks(const char* xex);
void launchSysModThread(char* modPath);
//extern DWORD dlaunchStartSysModule(char* modPath);
//BOOL APIENTRY DllMain(HANDLE hInstDLL, DWORD reason, LPVOID lpReserved)


#endif //_LAUNCH_H_
