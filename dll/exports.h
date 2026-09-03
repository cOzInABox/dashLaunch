#ifndef __DASHLAUNCH_EXPORTS_
#define __DASHLAUNCH_EXPORTS_

typedef struct _ldata{
	DWORD ID;
	DWORD ltype;
	char link[MAX_PATH];
	char dev[MAX_PATH];
	USHORT versionMaj;
	USHORT versionMin;
	USHORT targetKernel;
	USHORT svnVer;
	DWORD options; // for external apps that want to know what dash launch has set/parsed
	DWORD DebugRoutine; // for external apps that want to recursively hook and call the first/last chance exception trap on their own
	DWORD DebugStepPatch; // address to path single step exception to not be skipped (write 0x60000000/nop to this address to enable it)
	PBYTE tempData; // if temp monitoring is occuring, this is where it gets updated to...
	DWORD tempFreq; // how often tempdata byte array is getting updated
} ldata, *pldata;

#ifdef __cplusplus
extern "C" {
#endif
	pldata ldat;

	DWORD dlaunchStartSysModule(char* modPath);
	BOOL dlaunchStartTemps(DWORD timeInS, DWORD port, BOOL broadcast);
	VOID dlaunchShutdown(VOID);


#ifdef __cplusplus
}
#endif

#endif // __DASHLAUNCH_EXPORTS_
