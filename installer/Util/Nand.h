#pragma once
#include <xtl.h>
#include "xkelib.h"

#define MAX_PATCH_SIZE	0x4000

typedef enum {
	BL_TYPE_XENON = 0,
	BL_TYPE_ZEPHYR,
	BL_TYPE_FALCON,
	BL_TYPE_JASPER,
	BL_TYPE_TRINITY,
	BL_TYPE_CORONA,
	BL_TYPE_WINCHESTER,
	BL_TYPE_UNKNOWN
} BL_TYPE;

class Nand
{
public:	
	static Nand& GetInstance(){static Nand singleton; return singleton;}
	VOID Init(VOID);
	NTSTATUS WriteFileToFlash(BYTE* buffer, char* fileName, DWORD len);
	NTSTATUS DeleteFileFlash(char* fileName);

	BOOL IsJtag(VOID){return w_IsJtag;}
	BOOL IsPatchUpdateAvail(VOID){return w_PatchesNeeded;}
	DWORD GetConsoleType(VOID){return w_ConsoleType;}
	PWCHAR GetFlashModelWchar(VOID);
	PWCHAR GetHardwareModelWchar(VOID);
	PWCHAR GetFlashTypeWchar(VOID);
	BOOL Uninstall(VOID);
	BOOL WriteLhelperToFlash(VOID);
	BOOL WriteLaunchToFlash(VOID);
	BOOL UpdateLaunchXex(VOID);
	BOOL UpdatePatches(VOID);

private:
	BOOL w_IsJtag;
	BOOL w_IsGlitch2;
	BOOL w_IsGlitch2m;
	BOOL w_PatchesNeeded;
	DWORD w_ConsoleType;

	BOOL w_PatchInfo;
	DWORD w_embedMask;
	DWORD w_patchFileSize;
	PBYTE w_patchFileData;
	PBYTE w_currPatchData;
	char embedPatchName[16];
	char extPatchName[MAX_PATH];

	VOID SetPatchNames(VOID);
	VOID CheckPatches(VOID);
	NTSTATUS DoWriteFlashFile(BYTE* buffer, char* fileName, DWORD len);

	Nand();
	~Nand() {}
	Nand(const Nand&);                 // Prevent copy-construction
	Nand& operator=(const Nand&);      // Prevent assignment
};


