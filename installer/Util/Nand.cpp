#include <xtl.h>
#include <stdio.h>
#include "Nand.h"
#include "updSvr.h"
#include "XboxUtil.h"
#include "Resource.h"
#include "patchinfo.h"
#include "logging.h"

#define DEVICE_FLASH	"\\Device\\Flash"

static WCHAR* consoleModel[] = {
	L"Xenon",
	L"Zephyr",
	L"Falcon",
	L"Jasper",
	L"Trinity",
	L"Corona",
	L"Winchester",
	L"Unknown"
};

static WCHAR consoleJtag[] = L"JTAG";
static WCHAR consoleGlitch[] = L"Glitch";
static WCHAR consoleGlitch2[] = L"Glitch2";
static WCHAR consoleGlitch2m[] = L"Glitch2m";

static CHAR * PatchNames[] = {
	"xenon",
	"zephyr",
	"falcon",
	"jasper",
	"trinity",
	"corona",
	"winchester",
	"Unknown"
};

Nand::Nand()
{
	w_IsJtag = FALSE;
	w_PatchesNeeded = FALSE;
	w_ConsoleType = BL_TYPE_UNKNOWN;
	w_patchFileData = NULL;
	w_currPatchData = NULL;
	HRESULT hr = XboxUtil::GetInstance().MountPath("target:", "\\Device\\Flash\\");
}

//Nand::~Nand()
//{
//	if(w_patchFileData != NULL)
//		delete [] w_patchFileData;
//	if(w_currPatchData != NULL)
//		delete [] w_currPatchData;
//}

PWCHAR Nand::GetFlashModelWchar(VOID)
{
	return consoleModel[w_ConsoleType];
}

PWCHAR Nand::GetHardwareModelWchar(VOID)
{
	return consoleModel[((XboxHardwareInfo->Flags >>28)&0xF)];
}

PWCHAR Nand::GetFlashTypeWchar(VOID)
{
	if(w_IsJtag)
		return consoleJtag;
	else
	{
		if(w_IsGlitch2)
		{
			if(w_IsGlitch2m)
				return consoleGlitch2m;
			else
				return consoleGlitch2;
		}
		else if(w_IsGlitch2m)
			return consoleGlitch2m;
		else
			return consoleGlitch;
	}
}

VOID Nand::Init()
{
	DWORD tmask;
	BLDR_HEADER cbHdr;
	ZeroMemory(&cbHdr, sizeof(BLDR_HEADER));
	w_IsJtag = FALSE;
	w_IsGlitch2 = FALSE;
	w_IsGlitch2m = FALSE;
	w_embedMask = 0;

	tmask = Updsvr::GetInstance().getConType();
	if(tmask != 0)
	{
		w_ConsoleType = tmask&0xF;

		if((tmask&PATCH_TYPE_MASK) == PATCH_MASK_GLITCH) // fat glitch
		{
			w_embedMask = PATCH_MASK_GLITCH;
		}
		else // all others
		{
			w_embedMask = tmask;
			if(w_embedMask&PATCH_MASK_JTAG)
				w_IsJtag = TRUE;
			if(w_embedMask&PATCH_MASK_GLITCH2)
				w_IsGlitch2 = TRUE;
			if(w_embedMask&PATCH_MASK_GLITCH2MFG)
				w_IsGlitch2m = TRUE;
		}
		w_PatchInfo = TRUE;
		lDbgPrint("mask 0x%08x type: %S JTAG: %s GLITCH2: %s mfg: %s\n", w_embedMask, consoleModel[w_ConsoleType], w_IsJtag ? "yes":"no", w_IsGlitch2 ? "yes":"no", w_IsGlitch2m ? "yes":"no");
		SetPatchNames();
		CheckPatches();
	}
}

NTSTATUS Nand::DoWriteFlashFile(BYTE* buffer, char* fileName, DWORD len)
{
	HANDLE hFlashFile;
	NTSTATUS sta = 0;
	char destStr[MAX_PATH] = "sysmedia:\\"; // "\\Device\\Flash\\"; 
	strcat_s(destStr, MAX_PATH, fileName);
	hFlashFile = CreateFile(destStr, GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFlashFile != INVALID_HANDLE_VALUE)
	{
		PBYTE buf = buffer;
		DWORD tlen = len;
		DWORD writeSz;
		DWORD bWritten;
		while(tlen > 0)
		{
			if(tlen > 0x4000)
				writeSz = 0x4000;
			else
				writeSz = tlen;
			lDbgPrint("%s: writing 0x%x bytes of 0x%x\n", fileName, writeSz, tlen);
			bWritten = 0;
			if(WriteFile(hFlashFile, buf, writeSz, &bWritten, NULL))
			{
				if(bWritten == writeSz)
				{
					tlen -= writeSz;
					buf += writeSz; 
					lDbgPrint("%s: writefile written: 0x%x 0x%x rem: 0x%x sta: %d\n", fileName, bWritten, len-tlen, tlen, GetLastError());
				}
				else
				{
					lDbgPrint("%s: write timed out! len 0x%x sta %d\n", fileName, len, GetLastError());
					len = 0;
					sta = -1;
				}
			}
			else
			{
				sta = -1;
				lDbgPrint("%s: write failed to complete... len 0x%x sta %d\n", fileName, len, GetLastError());
			}
		}
		if(FlushFileBuffers(hFlashFile) != 0)
		{
			lDbgPrint("flush failed, error: %d\n", GetLastError());
			sta = -4;
		}
		CloseHandle(hFlashFile);
	}
	else
	{
		lDbgPrint("%s: unable to OpenFile for writing: %s:0x%x\n", fileName, GetLastError());
		sta = -3;
	}
	return sta;
}

// this is doing it the hard way, regular CreateFile and WriteFile work so long as you
// write in multiples of 0x4000 bytes until the last file chunk
NTSTATUS Nand::WriteFileToFlash(BYTE* buffer, char* fileName, DWORD len)
{
	ANSI_STRING nFlash;
	HANDLE hFlashDev;
	OBJECT_ATTRIBUTES atFlash;
	IO_STATUS_BLOCK ioFlash;
	NTSTATUS sta;
	RtlInitAnsiString(&nFlash, DEVICE_FLASH);
	InitializeObjectAttributes(&atFlash,&nFlash,OBJ_CASE_INSENSITIVE,NULL);

	sta = NtOpenFile(&hFlashDev, GENERIC_WRITE|SYNCHRONIZE|FILE_READ_ATTRIBUTES, &atFlash, &ioFlash, OPEN_EXISTING, FILE_SYNCHRONOUS_IO_NONALERT);
	if(sta >= 0)
	{
		sta = NtDeviceIoControlFile(hFlashDev, 0, 0, 0, &ioFlash, 0x3C8044, 0, 0, 0, 0); // sync/async? begin update transaction
		if(sta >= 0)
		{
			DWORD iocode = 0x3c8048; // end update transaction
			sta = DoWriteFlashFile(buffer, fileName, len);
			if(sta < 0)
				iocode = 0x3c804c; // begin update transaction ?? !!
			NtDeviceIoControlFile(hFlashDev, 0, 0, 0, &ioFlash, iocode, 0, 0, 0, 0);// end sync/async and flush? end update transaction
		}
		NtClose(hFlashDev);
	}
	else
	{
		lDbgPrint("unable to OpenFile for ioclt: %s:%x\n", nFlash.Buffer, sta);
	}
	return sta;
}

NTSTATUS Nand::DeleteFileFlash(char* fileName)
{
	char delStr[MAX_PATH] = "sysmedia:\\";
	strcat_s(delStr, MAX_PATH, fileName);
	if(DeleteFileA(delStr))
		return 0;
	return -1;
}

BOOL Nand::Uninstall(VOID)
{
	if(XboxUtil::GetInstance().IsFileExist("sysmedia:\\lhelper.xex"))
		DeleteFileFlash("lhelper.xex");
	if(XboxUtil::GetInstance().IsFileExist("sysmedia:\\launch.xex"))
		DeleteFileFlash("launch.xex");
	return !(XboxUtil::GetInstance().IsFileExist("sysmedia:\\lhelper.xex")||XboxUtil::GetInstance().IsFileExist("sysmedia:\\launch.xex"));
}

BOOL Nand::WriteLhelperToFlash(VOID)
{
	PVOID data;
	DWORD len;
	if(Resource::GetInstance().GetEmbeddedFile("lhelper", &data, &len))
	{
		if(WriteFileToFlash((PBYTE)data, "lhelper.xex", len) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL Nand::WriteLaunchToFlash(VOID)
{
	PVOID data;
	DWORD len;
	if(Resource::GetInstance().GetEmbeddedFile("launch", &data, &len))
	{
		if(WriteFileToFlash((PBYTE)data, "launch.xex", len) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL Nand::UpdateLaunchXex(VOID)
{
	Uninstall();
	if(WriteLaunchToFlash())
	{
		if(WriteLhelperToFlash())
		{
			return TRUE;
		}
	}
	return FALSE;
}

VOID Nand::SetPatchNames(VOID)
{
	if(w_IsJtag)
		sprintf_s(extPatchName, "Game:\\%d\\patches_%s.bin", XboxKrnlVersion->Build, PatchNames[w_ConsoleType]);
	else
	{
		if(w_embedMask == PATCH_MASK_GLITCH)
			sprintf_s(extPatchName, "Game:\\%d\\patches_fat.bin", XboxKrnlVersion->Build);
		else
		{
			if(w_IsGlitch2m)
				sprintf_s(extPatchName, "Game:\\%d\\patches_g2m%s.bin", XboxKrnlVersion->Build, PatchNames[w_ConsoleType]);
			else
				sprintf_s(extPatchName, "Game:\\%d\\patches_g2%s.bin", XboxKrnlVersion->Build, PatchNames[w_ConsoleType]);
		}
	}
	sprintf_s(embedPatchName, "p%d", XboxKrnlVersion->Build); // jtag
	lDbgPrint("embed name  : %s\n", embedPatchName);
	lDbgPrint("ext name    : %s\n", extPatchName);
}

VOID Nand::CheckPatches(VOID)
{
	w_PatchesNeeded = FALSE;
// 	lDbgPrint("Nand::CheckPatches\n");
	if(w_PatchInfo)
	{
		w_patchFileData = XboxUtil::GetInstance().ReadFileToBuf(extPatchName, &w_patchFileSize);
// 		lDbgPrint("w_patchFileData %08x\n", w_patchFileData);
		if((w_patchFileData == NULL)||(w_patchFileSize == 0))// check for external patches
		{
			PVOID data;
			DWORD size;
			if(Resource::GetInstance().GetEmbeddedFile(embedPatchName, &data, &size))
			{
				DWORD i;
				PBYTE pbData = (PBYTE)data;
				PPATCHINFO_HEADER head = (PPATCHINFO_HEADER)data;
				pbData += head->headerSize;
// 				lDbgPrint("embedded patches contain %d patch sets, scanning for type 0x%08x\n", head->numEntries, w_embedMask);
				for(i = 0; i < head->numEntries; i++)
				{
					if(head->inf[i].consoleType == w_embedMask)
					{
// 						lDbgPrint("patches found at entry %d, offset 0x%x size 0x%x\n", i, head->inf[i].offset, head->inf[i].size);
						w_patchFileData = &pbData[head->inf[i].offset];
						w_patchFileSize = head->inf[i].size;
						i = head->numEntries;
					}
				}
			}
			else
				lDbgPrint("patch resource %s lookup failed!\n", embedPatchName);
		}
		if((w_patchFileData !=NULL)&&(w_patchFileSize != 0))
		{
			w_currPatchData = new BYTE[MAX_PATCH_SIZE];
			memset(w_currPatchData, 0, MAX_PATCH_SIZE);
			if(Updsvr::GetInstance().getPatches(w_currPatchData, MAX_PATCH_SIZE))
			{
				if(memcmp(w_patchFileData, w_currPatchData, (w_patchFileSize-4)) != 0)
				{
					w_PatchesNeeded = TRUE;
				}
			}
		}
// 		else
// 			lDbgPrint("no patch data... w_patchFileData %08x w_patchFileSize %08x", w_patchFileData, w_patchFileSize);
	}
// 	else
// 		lDbgPrint("No patch info!\n");
}

BOOL Nand::UpdatePatches(VOID)
{
	if(w_PatchesNeeded)
	{
		if(Updsvr::GetInstance().setPatches(w_patchFileData, w_patchFileSize))
		{
			CheckPatches();
			return TRUE;
		}
	}
	return FALSE;
}
