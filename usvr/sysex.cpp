#include <xtl.h>
#include <xkelib.h>
#include "logging.h"

static char hddSysEx[] = "\\Device\\Harddisk0\\SystemExtPartition\\";
static char hddSysAux[] = "\\Device\\Harddisk0\\SystemAuxPartition\\";
static char hddSysComp[] = "\\Device\\Harddisk0\\SystemPartition\\";

DWORD threadWaitStatus = 0;
DWORD formatStatus = 0;

NTSTATUS wipePartition(char* path)
{
	NTSTATUS sts = -1;
	STRING devi;
	HANDLE hDevi;
	OBJECT_ATTRIBUTES oat;
	IO_STATUS_BLOCK ios;
	RtlInitAnsiString(&devi, path);
	oat.RootDirectory = 0;
	oat.ObjectName = &devi;
	oat.Attributes = FILE_ATTRIBUTE_DEVICE;

	sts = NtOpenFile(&hDevi, GENERIC_WRITE|GENERIC_READ|SYNCHRONIZE, &oat, &ios, OPEN_EXISTING, FILE_SYNCHRONOUS_IO_NONALERT);
	//sts = NtCreateFile(&hDevi, GENERIC_READ|GENERIC_WRITE|SYNCHRONIZE, &oat, &ios, 0, FILE_ATTRIBUTE_NORMAL, OPEN_EXISTING, 1, FILE_SYNCHRONOUS_IO_NONALERT|8);
	if(sts >= 0)
	{
		BYTE buf[0x4000];
		LARGE_INTEGER lint;
		lint.QuadPart = 0ULL;
		XMemSet(buf, 0, 0x1000);
		//sta = NtWriteFile(hFile, 0, 0, 0, &ioFlash, src, writeSize, &lOffset);
		sts = NtWriteFile(hDevi, NULL, NULL, NULL, &ios, buf, 0x1000, &lint);
// 		if(sts >= 0)
// 			DbgLog::GetInstance().log("wipe partition %s succeeded\n", path);
// 		else
// 			DbgLog::GetInstance().log("wipe partition failed to overwrite header: 0x%08x\n", sts);
		NtClose(hDevi);
	}
// 	else
// 	{
// 		DbgLog::GetInstance().log("wipe partition %s failed to open the partition: 0x%08x\n", path, sts);
// 	}
	return sts;
}

NTSTATUS formatPartition(char* path)
{
	STRING devi;
	DWORD val;
	RtlInitAnsiString(&devi, path);
	val = (DWORD)devi.Length;
	val = (val+0x10000)-1;
	devi.Length = val&0xFFFF;
	return XapiFormatFATVolume(&devi);
}

void delinkPartitions(void)
{
	HRESULT res;
	STRING LinkName;
	RtlInitAnsiString(&LinkName, "\\sap");
	res = ObDeleteSymbolicLink(&LinkName);
// 	DbgLog::GetInstance().log("delete \\sap result %x\n", res);
	RtlInitAnsiString(&LinkName, "\\sep");
	res = ObDeleteSymbolicLink(&LinkName);
// 	DbgLog::GetInstance().log("delete \\sep result %x\n", res);
}

void relinkPartitions(char* sepdev, char* sapdev)
{
	HRESULT res;
	STRING LinkName;
	STRING DeviName;
	RtlInitAnsiString(&LinkName, "\\sep");
	RtlInitAnsiString(&DeviName, sepdev);
	res = ObCreateSymbolicLink(&LinkName, &DeviName);
// 	DbgLog::GetInstance().log("create \\sep result %x\n", res);

	RtlInitAnsiString(&LinkName, "\\sap");
	RtlInitAnsiString(&DeviName, sapdev);
	res = ObCreateSymbolicLink(&LinkName, &DeviName);
// 	DbgLog::GetInstance().log("create \\sap result %x\n", res);
}

void formatThread(void)
{
	NTSTATUS ret;
	formatStatus = (DWORD)-1;
// 	DbgLog::GetInstance().log("format thread started\n");
	delinkPartitions();
	ret = wipePartition(hddSysEx);
// 	DbgLog::GetInstance().log("ex wipe  : 0x%x\n", ret);
	ret = formatPartition(hddSysEx);
// 	DbgLog::GetInstance().log("ex format: 0x%x\n", ret);
	if(ret >= 0)
	{
		ret = wipePartition(hddSysAux);
// 		DbgLog::GetInstance().log("ax wipe  : 0x%x\n", ret);
		ret = formatPartition(hddSysAux);
// 		DbgLog::GetInstance().log("ax format: 0x%x\n", ret);
		if(ret >= 0)
		{
			relinkPartitions(hddSysEx, hddSysAux);
			formatStatus = 0;
		}
	}
	doLightSync(&formatStatus);
	threadWaitStatus = 0;
	doLightSync(&threadWaitStatus);
}

BOOL doFormatPartitions(void)
{
	HANDLE pthread;
	DWORD pthreadid;
	threadWaitStatus = 1;
	doLightSync(&threadWaitStatus);
// 	DbgLog::GetInstance().log("starting format thread\n");
	ExCreateThread(&pthread, 0x10000, &pthreadid, (PVOID) XapiThreadStartup , (LPTHREAD_START_ROUTINE)formatThread, NULL, 0x2);
	XSetThreadProcessor(pthread, 4);
	ResumeThread(pthread);
	CloseHandle(pthread);
	while(threadWaitStatus == 1){Sleep(400);}
	if(formatStatus == 0)
	{
// 		DbgLog::GetInstance().log("format completed successfully!\n");
		return TRUE;
	}
// 	DbgLog::GetInstance().log("format failed horribly!\n");
	return FALSE;
}

void formatCompatThread(void)
{
	NTSTATUS ret;
	formatStatus = (DWORD)-1;
// 	DbgLog::GetInstance().log("formatCompat thread started\n");
	ret = wipePartition(hddSysComp);
// 	DbgLog::GetInstance().log("ex wipe  : 0x%x\n", ret);
	ret = formatPartition(hddSysComp);
// 	DbgLog::GetInstance().log("ex format: 0x%x\n", ret);
	if(ret >= 0)
	{
		formatStatus = 0;
	}
	doLightSync(&formatStatus);
	threadWaitStatus = 0;
	doLightSync(&threadWaitStatus);
}

BOOL doFormatCompatPartition(void)
{
	HANDLE pthread;
	DWORD pthreadid;
	threadWaitStatus = 1;
	doLightSync(&threadWaitStatus);
// 	DbgLog::GetInstance().log("starting format thread\n");
	ExCreateThread(&pthread, 0x10000, &pthreadid, (PVOID) XapiThreadStartup , (LPTHREAD_START_ROUTINE)formatCompatThread, NULL, 0x2);
	XSetThreadProcessor(pthread, 4);
	ResumeThread(pthread);
	CloseHandle(pthread);
	while(threadWaitStatus == 1){Sleep(400);}
	if(formatStatus == 0)
	{
// 		DbgLog::GetInstance().log("format completed successfully!\n");
		return TRUE;
	}
// 	DbgLog::GetInstance().log("format failed horribly!\n");
	return FALSE;
}