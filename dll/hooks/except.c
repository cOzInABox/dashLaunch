#include "_hook_includes.h"
#include <stdio.h>
#include <string.h>

typedef struct _stopEnum{
	DWORD stopCode;
	char reason[40];
}stopEnum;

typedef struct _TRAP_EXCEPTION_RECORD {
	DWORD ExceptionCode; // 0
	DWORD ExceptionFlags; // 4
	struct _EXCEPTION_RECORD *ExceptionRecord; // 8
	PVOID ExceptionAddress; // 12
	DWORD NumberParameters; // 16
	WORD unk1; // 20
	WORD unk2; // 22
} TRAP_EXCEPTION_RECORD, *PTRAP_EXCEPTION_RECORD;

typedef void (*KEBUGCHECKEXFUN)(DWORD stopCode, PDWORD Parm1, PDWORD Parm2, PDWORD Parm3, PDWORD Parm4);
typedef BOOL (*KDPTRAP)(PVOID Tf, PTRAP_EXCEPTION_RECORD Er, PCONTEXT Context, DWORD chance);

#define BACKTRACE_MAX_LOOP	25
extern keydata launch[MAX_NUM_BUTTONS];
static const char sep[] = "------------------------------------------------------------------------\n";
static DWORD exceptOutLock = 0;
static DWORD g_DbgLock = 0;

void dbgPut(DWORD ch)
{
	while((__loadvolatilewordbytereverse(0, DBG_SERIAL_STS)&0xFFFFFFFC) != 0)
	{
		YieldProcessor(); // db16cyc
		KeStallExecutionProcessor(0xA);
	}
	while((__loadvolatilewordbytereverse(0, DBG_SERIAL_STS)&2) == 0)
	{
		YieldProcessor(); // db16cyc
		KeStallExecutionProcessor(0xA);
	}
	__storevolatilewordbytereverse(ch, 0, DBG_SERIAL_XMIT);
	__eieio();
}

void dbgOutputString(PBYTE data, WORD len)
{
	WORD i;
	for(i = 0; i < len; i++)
	{
		if((data[i]&0xFF)== 0xA) // add in cr for missing linefeeds
			dbgPut(0xD);
		dbgPut((data[i]&0xFF));
	}
}

void dbgPrintFake(const char* s, ...)
{
	va_list argp;
	WORD len;
	char temp[512];
	BYTE irqSave;

	va_start(argp, s);
	RtlVsnprintf(temp, 512, s, argp);
	va_end(argp);
	len = (WORD)(strlen(temp)&0xFFFF);
	irqSave = KfRaiseIrql(0x7C);
	KeAcquireSpinLockAtRaisedIrql(&g_DbgLock);
	// Context->Gpr3&0xFFFFFFFF = string address
	// Context->Gpr4&0xFFFF = byte len, if len is zero process and skip
	dbgOutputString((PBYTE)temp, len);
	KeReleaseSpinLockFromRaisedIrql(&g_DbgLock);
	KfLowerIrql(irqSave);
}

#define NUM_LISTED_STOPS 19
stopEnum stops[] = {
	// reasons
	{0x0, "USER_CALLED_KE_BUG_CHECK"}, // KeBugCheck
	{0x9, "IRQL_NOT_GREATER_OR_EQUAL"}, // KiBugCheckIrqlNotGreaterOrEqual KeRetireDpcList
	{0x1E, "KMODE_EXCEPTION_NOT_HANDLED"}, // "ecode", "address", "param1", "param2" // KiDispatchException2
	{0x20, "KERNEL_APC_PENDING_DURING_EXIT"}, // ExTerminateThread
	{0x2B, "PANIC_STACK_SWITCH"}, // KiHandleStackOverflow
	{0x2C, "PORT_DRIVER_INTERNAL"}, // Sfcx Sata Mmcx Usb initializers
	{0x32, "PHASE1_INITIALIZATION_FAILED"}, // XampAllocCommitAndCreateHeap
	{0x35, "NO_MORE_IRP_STACK_LOCATIONS"}, // IoCallDriver
	{0x44, "MULTIPLE_IRP_COMPLETE_REQUESTS"}, // IoCompleteRequest IoFreeIrp
	{0x4C, "FATAL_UNHANDLED_HARD_ERROR"}, // VdDisplayFatalError
	{0xB4, "VIDEO_DRIVER_INIT_FAILURE"}, // VdpInitializeGraphicsInterrupts VdDriverEntry
	{0xB8, "ATTEMPTED_SWITCH_FROM_DPC"}, // KiSwapIdle
	{0xC7, "TIMER_OR_DPC_INVALID"}, // KeCheckForTimer
	{0xC8, "IRQL_UNEXPECTED_VALUE"}, // KiDeliverApc KiDeliverUserApc
	{0xD2, "BUGCODE_ID_DRIVER"}, // XampPhase1Initialization VdRetrainEDRAMWorker
	{0xF0, "FPU_UNAVAILABLE"}, // KiHandleFpuUnavailableInterrupt
	{0xF1, "SCSI_VERIFIER_DETECTED_VIOLATION"}, // KeLeaveUserMode
	{0xF3, "DISORDERLY_SHUTDOWN"}, // KeGetCurrentProcessType (process not idle)
	{0xF4, "CRITICAL_OBJECT_TERMINATION"}, // xam heap
};

char dayOfWeek[7][4]={"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
char month[12][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

HANDLE startLog(void)
{
	DWORD dwPos;
	HANDLE hLogFile = INVALID_HANDLE_VALUE;
	if(launch[DUMPFILE].dev != INVALID_ITEM)
	{
		mountToPath(launch[DUMPFILE].dev, MOUNT_DUMP);
		hLogFile = CreateFile(launch[DUMPFILE].launchpath, GENERIC_ALL, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hLogFile != INVALID_HANDLE_VALUE) // we need to seek to the end if it already exists...
		{
			if(GetLastError() == ERROR_ALREADY_EXISTS)
			{
				dwPos = SetFilePointer(hLogFile, 0, NULL, FILE_END);
				if(dwPos == INVALID_SET_FILE_POINTER)
					DbgPrint("startLog invalid set file pointer %d\n", GetLastError());
			}
		}
	}
	return hLogFile;
}

BOOL endLog(HANDLE hLogFile)
{
	if(hLogFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hLogFile);
		deleteLink(MOUNT_DUMP, FALSE);
		return TRUE;
	}
	return FALSE;
}

void logPrint(HANDLE hLogFile, const char* format, ...)
{
	DWORD dwWritten;
	va_list argp;
	int len;
	char temp[512];

	va_start(argp, format); 
	len = RtlVsnprintf(temp, 512, format, argp);
// 	len = vsnprintf_s(temp, 512, _TRUNCATE, format, argp);
	va_end(argp);
	if(hLogFile != INVALID_HANDLE_VALUE)
	{
		WriteFile(hLogFile, temp, len, &dwWritten, NULL);
	}
	DbgPrint(temp);
}

//void dbgPrintLine(const char* format, ...)
//{
//	BYTE irqSave;
//	va_list argp;
//	int len;
//	char temp[512];
//
//	irqSave = KfRaiseIrql(0x7C);
//	KeAcquireSpinLockAtRaisedIrql(&g_DbgLock);
//	va_start(argp, format); 
//	len = vsnprintf_s(temp, 512, _TRUNCATE, format, argp);
//	DbgPrint(temp);	
//	dbgOutputString((PBYTE)(temp), (WORD)(len));
//	KeReleaseSpinLockFromRaisedIrql(&g_DbgLock);
//	KfLowerIrql(irqSave);
//}

// returns true if continuable
BOOL showExceptInfo(PEXCEPTION_POINTERS eps, DWORD ecode)
{
	PCONTEXT pcontx;
	FILETIME time;
	SYSTEMTIME sTime;
	HANDLE hLogFile = INVALID_HANDLE_VALUE;
	DWORD contx, i, stackPtr;
	PLDR_DATA_TABLE_ENTRY ldat = NULL;
	hLogFile = startLog();
	KeQuerySystemTime(&time);
	FileTimeToSystemTime(&time, &sTime);
	logPrint(hLogFile, "%s", sep);
	logPrint(hLogFile, "TIME: %02d:%02d:%02d GMT %s, %s %d, %d\n", sTime.wHour, sTime.wMinute, sTime.wSecond, dayOfWeek[sTime.wDayOfWeek], month[(sTime.wMonth-1)], sTime.wDay, sTime.wYear);
	logPrint(hLogFile, "%s", sep);
	logPrint(hLogFile, "Unhandled Exception (last chance):\n");
	switch(ecode)
	{
		case EXCEPTION_ACCESS_VIOLATION: 
			logPrint(hLogFile, "The thread tried to read from or write to a virtual address for which\nit does not have access");
			break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: 
			logPrint(hLogFile, "The thread tried to access an array element that is out of bounds");
			break;
		case EXCEPTION_BREAKPOINT: 
			logPrint(hLogFile, "A breakpoint was encountered");
			break;
		case EXCEPTION_DATATYPE_MISALIGNMENT: 
			logPrint(hLogFile, "The thread tried to read or write data that is misaligned"); //For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on.");
			break;
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			logPrint(hLogFile, "One of the operands in a floating-point operation is denormal");
			break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO: 
			logPrint(hLogFile, "The thread tried to divide a floating-point value by zero");
			break;
		case EXCEPTION_FLT_INEXACT_RESULT:
			logPrint(hLogFile, "The result of a floating-point operation cannot be represented exactly\nas a decimal fraction");
			break;
		case EXCEPTION_FLT_INVALID_OPERATION: 
			logPrint(hLogFile, "This exception represents any floating-point exception not included in\nthis list");
			break;
		case EXCEPTION_FLT_OVERFLOW: 
			logPrint(hLogFile, "The exponent of a floating-point operation is greater than the\nmagnitude allowed by the corresponding type");
			break;
		case EXCEPTION_FLT_STACK_CHECK: 
			logPrint(hLogFile, "The stack overflowed or underflowed as the result of a\nfloating-point operation");
			break;
		case EXCEPTION_FLT_UNDERFLOW: 
			logPrint(hLogFile, "The exponent of a floating-point operation is less than the magnitude\nallowed by the corresponding type");
			break;
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			logPrint(hLogFile, "The thread tried to execute an invalid instruction");
			break;
		case EXCEPTION_IN_PAGE_ERROR: 
			logPrint(hLogFile, "The thread tried to access a page that was not present, and the system\nwas unable to load the page");// For example, this exception might occur if a network connection is lost while running a program over the network.");
			break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			logPrint(hLogFile, "The thread tried to divide an integer value by zero");
			break;
		case EXCEPTION_INT_OVERFLOW: 
			logPrint(hLogFile, "The result of an integer operation caused a carry out of the most\nsignificant bit of the result");
			break;
		case EXCEPTION_INVALID_DISPOSITION:
			logPrint(hLogFile, "An exception handler returned an invalid disposition to the\nexception dispatcher"); // Programmers using a high-level language such as C should never encounter this exception.");
			break;
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			logPrint(hLogFile, "The thread tried to continue execution after a non-continuable\nexception occurred");
			break;
		case EXCEPTION_PRIV_INSTRUCTION: 
			logPrint(hLogFile, "The thread tried to execute an instruction whose operation is not\nallowed in the current machine mode");
			break;
		case EXCEPTION_SINGLE_STEP: 
			logPrint(hLogFile, "A trace trap or other single-instruction mechanism signaled that one\ninstruction has been executed");
			break;
		case EXCEPTION_STACK_OVERFLOW: 
			logPrint(hLogFile, "The thread used up its stack");
			break;
		default:
			logPrint(hLogFile, "Unknown reason %08X", ecode);
	}
	logPrint(hLogFile, ".\n%s", sep);

	if(XexGetModuleHandle(NULL, (PHANDLE)&ldat) == 0)
	{
		if(ldat->XexHeaderBase != NULL)
		{
			PXEX_HEADER_STRING peName = (PXEX_HEADER_STRING)RtlImageXexHeaderField(ldat->XexHeaderBase, XEX_HEADER_PE_MODULE_NAME);
			if(peName != NULL)
			{
				char* imgPath = (char*)resolveFunct(MODULE_KERNEL, 431);
				if(peName->Data != NULL)
				{
					logPrint(hLogFile, "Title Module Info:\n");
					logPrint(hLogFile, "\tTitle PE name  : %s\n", peName->Data);
					if(imgPath != NULL)
					{
						if((imgPath[0]&0xFF) != 0x0)
							logPrint(hLogFile, "\tTitle Path     : %s\n", imgPath);
					}
					logPrint(hLogFile, "%s", sep);
				}
			}
		}
	}

	XexPcToFileHeader(eps->ExceptionRecord->ExceptionAddress, &ldat);
	if(ldat != NULL)
	{
		PXEX_HEADER_STRING peName = NULL;
		if(ldat->XexHeaderBase != NULL)
			peName = (PXEX_HEADER_STRING)RtlImageXexHeaderField(ldat->XexHeaderBase, XEX_HEADER_PE_MODULE_NAME);

		logPrint(hLogFile, "Faulting Module Info:\n");
		if((ldat->FullDllName.Buffer != NULL) && (ldat->FullDllName.Buffer[0] != 0))
			logPrint(hLogFile, "\tFullName       : %S\n", ldat->FullDllName.Buffer);
		if((ldat->BaseDllName.Buffer != NULL) && (ldat->BaseDllName.Buffer[0] != 0))
			logPrint(hLogFile, "\tBaseName       : %S\n", ldat->BaseDllName.Buffer);
		if((peName != NULL) && (peName->Data[0] != 0))
				logPrint(hLogFile, "\tPEName         : %s\n", peName->Data);
		logPrint(hLogFile, "\tImageBase      : %08X\n",ldat->ImageBase);
		logPrint(hLogFile, "\tSizeOfFullImage: %08X\n",ldat->SizeOfFullImage);		
		logPrint(hLogFile, "\tEntryPoint     : %08X\n",ldat->EntryPoint);		
		//logPrint(hLogFile, "\tSizeOfNtImage  : %08X\n",ldat->SizeOfNtImage);		
		//logPrint(hLogFile, "\tNtHeadersBase  : %08X\n",ldat->NtHeadersBase);
	}

	logPrint(hLogFile, "%s", sep);
	logPrint(hLogFile, "\tEADDR: 0x%08X\n", eps->ExceptionRecord->ExceptionAddress);
	logPrint(hLogFile, "\tECODE: 0x%08X\n", eps->ExceptionRecord->ExceptionCode);
	logPrint(hLogFile, "\tEFLAG: %i (%s)\n", eps->ExceptionRecord->ExceptionFlags, (eps->ExceptionRecord->ExceptionFlags==0)? "cont":"non-cont" ); // EXCEPTION_NONCONTINUABLE
	logPrint(hLogFile, "\tNPARM: %i\n", eps->ExceptionRecord->NumberParameters);

	if((eps->ExceptionRecord->NumberParameters > 0)&&(eps->ExceptionRecord->NumberParameters < EXCEPTION_MAXIMUM_PARAMETERS))
	{
		for(i=0; i < eps->ExceptionRecord->NumberParameters; i++)
			logPrint(hLogFile, "\t     PARAM %02d: 0x%08X\n",i, eps->ExceptionRecord->ExceptionInformation[i]);
	}

	pcontx = eps->ContextRecord;
	contx = pcontx->ContextFlags;
	logPrint(hLogFile, "\tCFLAG: 0x%08X\n",contx);
	logPrint(hLogFile, "%s", sep);
	if(contx & CONTEXT_CONTROL)
	{
		logPrint(hLogFile, "Control Registers:\n");
		logPrint(hLogFile, "\tMSR: 0x%08X IAR: 0x%08X\n", pcontx->Msr, pcontx->Iar);
		logPrint(hLogFile, "\tLR : 0x%08X CTR: 0x%016I64X\n", pcontx->Lr, pcontx->Ctr);
	}
	logPrint(hLogFile, "%s", sep);
	if(contx & CONTEXT_INTEGER)
	{
		logPrint(hLogFile, "Integer Registers:\n");
		logPrint(hLogFile, "\tCR : 0x%08X XER: 0x%08X\n", pcontx->Cr, pcontx->Xer);
		logPrint(hLogFile, "\tr0 : 0x%016I64X r1 : 0x%016I64X r2 : 0x%016I64X\n", pcontx->Gpr0, pcontx->Gpr1, pcontx->Gpr2);
		logPrint(hLogFile, "\tr3 : 0x%016I64X r4 : 0x%016I64X r5 : 0x%016I64X\n", pcontx->Gpr3, pcontx->Gpr4, pcontx->Gpr5);
		logPrint(hLogFile, "\tr6 : 0x%016I64X r7 : 0x%016I64X r8 : 0x%016I64X\n", pcontx->Gpr6, pcontx->Gpr7, pcontx->Gpr8);
		logPrint(hLogFile, "\tr9 : 0x%016I64X r10: 0x%016I64X r11: 0x%016I64X\n", pcontx->Gpr9, pcontx->Gpr10, pcontx->Gpr11);
		logPrint(hLogFile, "\tr12: 0x%016I64X r13: 0x%016I64X r14: 0x%016I64X\n", pcontx->Gpr12, pcontx->Gpr13, pcontx->Gpr14);
		logPrint(hLogFile, "\tr15: 0x%016I64X r16: 0x%016I64X r17: 0x%016I64X\n", pcontx->Gpr15, pcontx->Gpr16, pcontx->Gpr17);
		logPrint(hLogFile, "\tr18: 0x%016I64X r19: 0x%016I64X r20: 0x%016I64X\n", pcontx->Gpr18, pcontx->Gpr19, pcontx->Gpr20);
		logPrint(hLogFile, "\tr21: 0x%016I64X r22: 0x%016I64X r23: 0x%016I64X\n", pcontx->Gpr21, pcontx->Gpr22, pcontx->Gpr23);
		logPrint(hLogFile, "\tr24: 0x%016I64X r25: 0x%016I64X r26: 0x%016I64X\n", pcontx->Gpr24, pcontx->Gpr25, pcontx->Gpr26);
		logPrint(hLogFile, "\tr27: 0x%016I64X r28: 0x%016I64X r29: 0x%016I64X\n", pcontx->Gpr27, pcontx->Gpr28, pcontx->Gpr29);
		logPrint(hLogFile, "\tr30: 0x%016I64X r31: 0x%016I64X\n", pcontx->Gpr30, pcontx->Gpr31);
	}
	//if(contx & CONTEXT_FLOATING_POINT)
	//{
	//}
	//if(contx & CONTEXT_VECTOR)
	//{
	//}
	// Dump call stack
	logPrint(hLogFile, "%s", sep);
	logPrint(hLogFile, "Call Stack:\n");
	logPrint(hLogFile, "\t0x%08X (EADDR)\n", eps->ExceptionRecord->ExceptionAddress);
	logPrint(hLogFile, "\t0x%08X (LR)\n", pcontx->Lr);
	stackPtr = (DWORD)pcontx->Gpr1;
	for(i = 0; i < BACKTRACE_MAX_LOOP; i++)
	{
		stackPtr = *(DWORD*)stackPtr;
		if(stackPtr != 0)
		{
			DWORD lr = *((DWORD*)(stackPtr - 0x08));
			logPrint(hLogFile, "\t0x%08X\n", lr);
		}
		else
			i = BACKTRACE_MAX_LOOP;
	}
	logPrint(hLogFile, "%s", sep);
	logPrint(hLogFile, "Dump Done\n");
	logPrint(hLogFile, "%s", sep);
	if(endLog(hLogFile))
		hLogFile = INVALID_HANDLE_VALUE;
	// if ExceptionFlags is 0 then the exception was non-fatal
	return (eps->ExceptionRecord->ExceptionFlags == 0);
}

void doShutdown(void)
{
	if(!getOpt(OPT_FATAL_FREEZE))
	{
		if(getOpt(OPT_FATAL_REBOOT))
			HalReturnToFirmware(HalRebootQuiesceRoutine);
		else
			HalReturnToFirmware(HalPowerDownRoutine);
	}
}

//DWORD __declspec(naked) GetProcessor(void)
//{ 
//	__asm {
//		mfpvr      r3
//		blr
//	}
//}

void exceptRelaunchDash(void)
{
	Sleep(1000);
	//	DbgPrint("doing fatal dash relaunch now\n");
	XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
}

static QWORD exceptRecoverLast = 0;
static WCHAR relaunchNotify[] = {'F','a','t','a','l',' ','C','r','a','s','h',' ','I','n','t','e','r','c','e','p','t','e','d','!',0x0};

void exceptHandleRecoverableCrash(void)
{
	QWORD stime = KeTimeStampBundle->SystemTime.QuadPart;
	if((exceptRecoverLast == 0)||((stime-exceptRecoverLast) > 0x2430000)) // approx 4 seconds
	{
		// start a new title thread for relaunching and tell kernel the except was handled
		HANDLE hThread;
		DWORD dwThreadId;
		exceptRecoverLast = stime;
		XNotifyQueueUI(XNOTIFYUI_TYPE_AVOID_REVIEW, 0xFF, XNOTIFY_SYSTEM, relaunchNotify, NULL);
		hThread = CreateThread( 0, 0, (LPTHREAD_START_ROUTINE)exceptRelaunchDash, 0, CREATE_SUSPENDED, &dwThreadId );
//HANDLE thread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)XamReadTileToTextureThread, (LPVOID)xamTileData, 0, NULL );
//WaitForSingleObject( thread, INFINITE );
		XSetThreadProcessor(hThread, 4);
		ResumeThread(hThread);
		CloseHandle(hThread);
	}
}

// this is never called...
void __declspec(naked) kdpTrapSaveVar(void)
{
	__asm{
		li r3, EXCEPTTRAP_VAL
		nop
		nop
		nop
		nop
		nop
		nop
		blr
	}
}
KDPTRAP kdpTrapSave = (KDPTRAP)kdpTrapSaveVar;

// returns true if handled...
// kernel startup puts the address of this function in kernel data space, the handler loads that address and jumps there
// the hook just replaces that address as it is never refreshed aside from being set once at boot time
BOOL kdRetrap(PVOID Tf, PTRAP_EXCEPTION_RECORD Er, PCONTEXT Context, DWORD chance)
{
	//if(chance == 1)//last chance exception
	//#define STATUS_GUARD_PAGE_VIOLATION      ((DWORD   )0x80000001L)    
	//#define STATUS_DATATYPE_MISALIGNMENT     ((DWORD   )0x80000002L)    
	//#define STATUS_BREAKPOINT                ((DWORD   )0x80000003L)    
	//#define STATUS_SINGLE_STEP               ((DWORD   )0x80000004L)    
	//#define DBG_EXCEPTION_NOT_HANDLED        ((DWORD   )0x80010001L) 
	if(Er->ExceptionCode == STATUS_BREAKPOINT)
	{
		//cprintf("unk: %04x gpr3: %016I64X addr: %08x gpr4: %016I64X len: %04x\n", Er->unk2, Context->Gpr3, Context->Gpr3&0xFFFFFFFF, Context->Gpr4, Context->Gpr4&0xFFFF);
		if((Er->unk2 == 0x14)&&(getOpt(OPT_DEBUG_OUTPUT))) // 0x14 is DbgPrint breakpoint output
		{
			if((Context->Gpr4&0xFFFF) != 0)
			{
				BYTE irqSave;
				irqSave = KfRaiseIrql(0x7C);
				KeAcquireSpinLockAtRaisedIrql(&g_DbgLock);
				// Context->Gpr3&0xFFFFFFFF = string address
				// Context->Gpr4&0xFFFF = byte len, if len is zero process and skip
				dbgOutputString((PBYTE)((DWORD)Context->Gpr3&0xFFFFFFFF), (WORD)(Context->Gpr4&0xFFFF));
				KeReleaseSpinLockFromRaisedIrql(&g_DbgLock);
				KfLowerIrql(irqSave);
			}

		}
		else if(Er->unk2 == 0x1C) // 0x1c is serial kd packet data?
			Context->Gpr3 = 0;
		Context->Iar += 4;
		return TRUE;
	}
	else if(chance == 1) // it wasn't dispatched to a SEH
	{
		EXCEPTION_POINTERS excp;
		BOOL nonFatal = FALSE;
		BYTE irql;
		excp.ContextRecord = Context;
		excp.ExceptionRecord = (PEXCEPTION_RECORD)Er;

		// gain (hopefully, it's not always perfect in parallel execution on different cores) exclusive access and dump info...
		irql = KfAcquireSpinLock(&exceptOutLock);
		nonFatal = showExceptInfo(&excp, (DWORD)Er->ExceptionCode);
		KfReleaseSpinLock(&exceptOutLock, irql);

		Context->Iar += 4; // increment the instruction to get over the fault
		if(nonFatal)
		{
			exceptHandleRecoverableCrash();
			return TRUE;
		}
		else 
			doShutdown();
	}
	return FALSE;
}

void showKtrapInfo(PKTRAP_FRAME trp, PDWORD eaddr)
{
	int i;
	DWORD stackPtr;
	DbgPrint("Call Stack:\n");
	DbgPrint("\t0x%08X (EADDR)\n", eaddr);
	DbgPrint("\t0x%08X (LR)\n", trp->Lr);
	stackPtr = (DWORD)trp->Header.BackChain;
	for(i = 0; i < BACKTRACE_MAX_LOOP; i++)
	{
		stackPtr = *(DWORD*)stackPtr;
		if(stackPtr != 0)
		{
			DWORD lr = *((DWORD*)(stackPtr - 0x08));
			DbgPrint("\t0x%08X\n", lr);
		}
		else
			i = BACKTRACE_MAX_LOOP;
	}
	DbgPrint("%s", sep);
	DbgPrint("Dump Done\n");
}

// ************* keBugCheckExSaveVar hook start *************
// ------------------------------------------------------------------------------------------------------------
void __declspec(naked) keBugCheckExSaveVar(void)
{
	__asm{
		li r3, BUGCHECK_VAL
		nop
		nop
		nop
		nop
		nop
		nop
		blr
	}
}
KEBUGCHECKEXFUN keBugCheckExSave = (KEBUGCHECKEXFUN)keBugCheckExSaveVar;
// OPT_FATAL_FREEZE bit is set when freeze is not avoided
// OPT_FATAL_REBOOT when above is set, this will cause the box to reboot instead of shutoff
void keBugCheckExHook(DWORD stopCode, PDWORD Parm1, PDWORD Parm2, PDWORD Parm3, PDWORD Parm4)
{
	int i, stopName = -1;
	for(i = 0; i < NUM_LISTED_STOPS; i++)
	{
		if(stops[i].stopCode == stopCode)
		{
			stopName = i;
			i = NUM_LISTED_STOPS;
		}
	}
	DbgPrint("\n%s", sep);
	DbgPrint("*** Fatal System Error\n  stop code: 0x%x ", stopCode);
	if(stopName != -1)
		DbgPrint("(%s)", stops[stopName].reason);
	DbgPrint("\n    (0x%p,0x%p,0x%p,0x%p)\n", Parm1, Parm2, Parm3, Parm4);
	DbgPrint("%s", sep);
	if(stopCode == 0x2b)
	{
		showKtrapInfo((PKTRAP_FRAME)Parm3, Parm2);
	}
	doShutdown();
	keBugCheckExSave(stopCode, Parm1, Parm2, Parm3, Parm4);
}
// ------------------------------------------------------------------------------------------------------------
// ************* keBugCheckExSaveVar hook end *************

static BOOL isTrapHooked = FALSE;
static PDWORD trapAddr;
static DWORD trapOld[4];
void exceptTrapHook(PDWORD addr)
{
	// hook at the debugger trap to prevent as many fatalities as possible
	if(isTrapHooked == FALSE)
	{
		trapAddr = addr;
		hookFunctionStart(trapAddr, (PDWORD)kdpTrapSaveVar, trapOld, (DWORD)kdRetrap);
		isTrapHooked = TRUE;
	}
}

void exceptTrapUnhook(void)
{
	if(isTrapHooked)
	{
		unhookFunctionStart(trapAddr, trapOld);
		isTrapHooked = FALSE;
	}
}

static BOOL isExceptBugHooked = FALSE;
static DWORD exceptBugSave[4];
void exceptBugcheckHook(void)
{
	if(!isExceptBugHooked)
	{
		// hook at bugcheckex to take over shutdown/hard reset
		hookFunctionStartOrd(MODULE_KERNEL, kernelExp_KeBugCheckEx, (PDWORD)keBugCheckExSaveVar, exceptBugSave, (DWORD)keBugCheckExHook);
		isExceptBugHooked = TRUE;
	}
}

void exceptBugcheckUnhook(void)
{
	if(isExceptBugHooked)
	{
		unhookFunctionStartOrd(MODULE_KERNEL, kernelExp_KeBugCheckEx, exceptBugSave);
		isExceptBugHooked = FALSE;
	}
}
