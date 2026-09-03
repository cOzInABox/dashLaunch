#ifndef _VERSION_H
#define _VERSION_H
#include "svnversion.h"

#define VER_MAJ 3
#define VER_MIN 21
#define VER_SVN _SVNVERSION

//#define RELEASE_IS_BETA	1

#define INSTALLER_TID	0xFFFF011D

#define LHELPER_CON		0
#define LHELPER_XEX		1

#define LHELPOPT_SIGNIN		1
#define LHELPOPT_FIRSTBUT	2
#define LHELPOPT_FAKEANIM	4
#define LHELPOPT_FORCEDASH	8

#define DEFAULT_XEX		"default.xex"
#define MOUNT_NAME		"dlaunch:"
#define MOUNT_DRIVE		"dlaunch:\\"
#define MOUNT_OBJECT	"\\??\\dlaunch:"

#define LAUNCH_DATA_ID	'DL30'
#define MODULE_LAUNCH	"launch.xex"

enum{
	INVALID_PATH = 0, // not really used for anything anymore...
	XEX_PATH,
	CON_PATH,
	ELF_PATH,
	PLUGIN_LOADED = 0xFB,
	PLUGIN_NOTFOUND = 0xFC,
	FORCED_ITEM = 0xFD,
	ITEM_FOUND = 0xFE,
	INVALID_ITEM = 0xFF
};

enum{
	OPT_MIN = 0,
	OPT_CAT_PATHS, // various paths including quick launch buttons
	OPT_CAT_BEHAVIOR, // settings that change xbox behavior
	OPT_CAT_NETWORK, // settings that impact network
	OPT_CAT_TIMERS, // settings for things that run on timers
	OPT_CAT_PLUGINS, // plugin paths
	OPT_CAT_EXTERNAL, // options for the configurator
	OPT_CAT_MAX // not a real category, just a placeholder for array stuff
};

enum{
	DEFAULT = 0,
	BUT_A,
	BUT_B,
	BUT_X,
	BUT_Y,
	START,
	BACK,
	LBUMP,
	LTHUMB,
	RTHUMB,
	GUIDEPWR,
	CONSPWR,
	BOOTANIM,
	DUMPFILE,
	CONFIGFILE,
	MIN_PLUGINS,
	PLUGIN1 = MIN_PLUGINS,
	PLUGIN2,
	PLUGIN3,
	PLUGIN4,
	PLUGIN5,
	MAX_PLUGINS,
	MAX_NUM_BUTTONS = MAX_PLUGINS
};
#define NUM_PLUGINS_SUPPORTED (MAX_PLUGINS-MIN_PLUGINS)

typedef struct _ldata{
	DWORD ID;
	USHORT lhelpOpts;
	USHORT ltype;
	char link[MAX_PATH];
	char dev[MAX_PATH];
	USHORT versionMaj;
	USHORT versionMin;
	USHORT targetKernel;
	USHORT svnVer;
	DWORD options; // for external apps that want to know what dash launch has set/parsed
	DWORD DebugRoutine; // for external apps that want to recursively hook and call the first/last chance exception trap on their own
	DWORD DebugStepPatch; // address to path single step exception to not be skipped (write 0x60000000/nop to this address to enable it)
	PBYTE tempData; // DL will monitor temps, a copy of the smc temp data is placed here, 0x10 bytes in len
	DWORD iniPathSel; // the path corresponding to this number can be gotten via dlaunchGetDriveList
} ldata, *pldata;

typedef struct _keydata {
	char launchpath[MAX_PATH];
	DWORD flags;
	DWORD dev;
	DWORD rootDev;
} keydata, *pkeydata;

#define DL_OPT_TYPE_ALLEXEC 0xFFFFFFFF
typedef enum {
	// DWORD containing a value between 0 and 1
	DL_OPT_TYPE_BOOL = 0,
	// WORD containing a value between 0 and 0xFFFF
	DL_OPT_TYPE_WORD,
	DL_OPT_TYPE_WORDREGION, // 0-0x7FFF
	DL_OPT_TYPE_WORDPORT, // 1 - 0xFFFF
	// DWORD containing a value between 0 and 0xFFFFFFFF
	DL_OPT_TYPE_DWORD,
	DL_OPT_TYPE_DWORDTIME, // in seconds
	// anything >= this can't be changed easily
	DL_OPT_TYPE_MAX_ACCESS,
	// DWORD containing a memory address/pkeydata element
	DL_OPT_TYPE_PATH = DL_OPT_TYPE_MAX_ACCESS,
	DL_OPT_TYPE_PATHQLB,
	DL_OPT_TYPE_PATHPLUGIN,
} DL_OPT_TYPES;

typedef enum {
	DL_ORDINALS_LDAT = 1,
	DL_ORDINALS_STARTSYSMOD = 2,
	DL_ORDINALS_SHUTDOWN = 3,
	DL_ORDINALS_FORCEINILOAD = 4,
	DL_ORDINALS_GETNUMOPTS = 5,
	DL_ORDINALS_GETOPTINFO = 6,
	DL_ORDINALS_GETOPTVAL = 7,
	DL_ORDINALS_SETOPTVAL = 8,
	DL_ORDINALS_GETOPTVALBYNAME = 9,
	DL_ORDINALS_SETOPTVALBYNAME = 10,
	DL_ORDINALS_GETDRIVELIST = 11,
	DL_ORDINALS_GETDRIVEINFO = 12,
	DL_ORDINALS_PLUGINPATH = 14,
} DL_ORDINALS;



#define KVERS_SUPPORTED		27
#define _KVER_0				9199
#define _KVER_1				12611
#define _KVER_2				12625
#define _KVER_3				13146
#define _KVER_4				13599
#define _KVER_5				13604
#define _KVER_6				14699
#define _KVER_7				14717
#define _KVER_8				14719
#define _KVER_9				15574
#define _KVER_10			16197
#define _KVER_11			16202
#define _KVER_12			16203
#define _KVER_13			16537
#define _KVER_14			16547
#define _KVER_15			16747
#define _KVER_16			16756
#define _KVER_17			16767
#define _KVER_18			17148
#define _KVER_19			17150
#define _KVER_20			17349
#define _KVER_21			17489
#define _KVER_22			17502
#define _KVER_23			17511
#define _KVER_24			17526
#define _KVER_25			17544
#define _KVER_26			17559

#define _KVER_MACRO		_KVER_0, _KVER_1, _KVER_2, _KVER_3, _KVER_4, _KVER_5, _KVER_6, _KVER_7, _KVER_8, _KVER_9, _KVER_10, _KVER_11, _KVER_12, _KVER_13, _KVER_14, _KVER_15, _KVER_16, _KVER_17, _KVER_18, _KVER_19, _KVER_20, _KVER_21, _KVER_22, _KVER_23, _KVER_24, _KVER_25, _KVER_26

#endif _VERSION_H
