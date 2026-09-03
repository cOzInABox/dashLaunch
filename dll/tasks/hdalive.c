#include "_task_includes.h"

static HXAMTASK hHddAlive = INVALID_HANDLE_VALUE;
#define HDALIVE_MIN_TIME	10

void hddDoFile(int item)
{
	const char* path = getMountPathItem(item);
	MountPath("alive:", path, FALSE);
	if(driveExists("alive:\\"))
	{
		// 			DbgPrint("drive exists %s\n", path);
		if(fileExists("alive:\\alive.txt"))
		{
			HANDLE fhandle;
			// 				DbgPrint("alive.txt found on drive %s\n", path);
			fhandle = CreateFile("alive:\\alive.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if(fhandle != INVALID_HANDLE_VALUE)
			{
				DWORD written;
				BYTE rand[0x10];
				XeCryptRandom(rand, 0x10);
				WriteFile(fhandle, rand, 0x10, &written, NULL);
				// 					DbgPrint("%d bytes written to alive.txt\n", written);
				CloseHandle(fhandle);
			}
		}
	}
	deleteLink(MOUNT_ALIVE, FALSE);
}

void hddKeepAlive(void)
{
	int i;
// 	DbgPrint("keep alive\n");
	for(i = 0; i < MOUNT_HDD; i++)
		hddDoFile(i);
	hddDoFile(MOUNT_TRIN_INTMU);
}

void scheduleHdAliveTask(DWORD timeInS)
{
	if(hHddAlive == INVALID_HANDLE_VALUE)
	{
		XAMTASKATTRIBUTES xta;
		HRESULT hr;
		RtlZeroMemory(&xta, sizeof(xta)); // XAMPROPERTY_MISC_ONSYSTEMBEHALF
		xta.dwProperties = XAMPROPERTY_TYPE_PERIODIC | XAMPROPERTY_CPUUSAGE_LO | XAMPROPERTY_DURATION_SHORT | XAMPROPERTY_PRI_NORMAL | XAMPROPERTY_MISC_ONSYSTEMBEHALF;

		if(timeInS <= HDALIVE_MIN_TIME) // minimum is 1 min...
			xta.typ.dwPeriod = (1000*HDALIVE_MIN_TIME); 
		else
			xta.typ.dwPeriod = (timeInS*1000);
		hr = XamTaskSchedule((PXAMTASKPROC)hddKeepAlive, NULL, &xta, &hHddAlive);
// 		if(SUCCEEDED(hr))
// 			dbgPrintFake("xam task scheduled at %dms interval\n", xta.typ.dwPeriod);
// 		else
// 			dbgPrintFake("failed to schedule task at %dms intervals\n", xta.typ.dwPeriod);
	}
}

void modifyHdAliveTimer(DWORD timeInS)
{
	if(hHddAlive != INVALID_HANDLE_VALUE)
	{
		XAMTASKATTRIBUTES xta;
		RtlZeroMemory(&xta, sizeof(xta));
		XamTaskGetAttributes(hHddAlive, &xta);
		if(timeInS <= HDALIVE_MIN_TIME) // minimum is 1 min...
			xta.typ.dwPeriod = (1000*HDALIVE_MIN_TIME); 
		else
			xta.typ.dwPeriod = (timeInS*1000);
		XamTaskModify(hHddAlive, XAMTASKMODIFY_ATTRIBUTES, NULL, NULL, &xta);
	}
}

void endHdAliveTask(void)
{
	if(hHddAlive != INVALID_HANDLE_VALUE)
	{
		XamTaskCancel(hHddAlive);
		//XamTaskWaitOnCompletion(hHddAlive); // wait for cancel to go through
		XamTaskCloseHandle(hHddAlive);
		hHddAlive = INVALID_HANDLE_VALUE;
	}
}

