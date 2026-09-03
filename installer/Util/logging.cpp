#include <xtl.h>
#include <stdio.h>
#include "xkelib.h"
#include "logging.h"

#define EXCEPT_FILE_NAME	"Game:\\icrash.log"
static const char sep[] = "------------------------------------------------------------------------\n";
#define BACKTRACE_MAX_LOOP	25
static PWCHAR exceptNotifyLog = L"CRASH - BEING LOGGED!";
static PWCHAR exceptUnlogged = L"CRASH - NOT LOGGED!";

#ifdef LOG_DEBUG_OUT
#define STATIC_FILE_NAME	"Game:\\installer.log"

DbgLog::DbgLog()
{
	HANDLE fhand;
	DeleteFile(STATIC_FILE_NAME);
	fhand = CreateFile(STATIC_FILE_NAME, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fhand != INVALID_HANDLE_VALUE)
	{
		CloseHandle(fhand);
		InitializeCriticalSection(&writeLock);
		logToFile = TRUE;
	}
}

void DbgLog::appendLog(int len, char* data)
{
	HANDLE fhand;
	EnterCriticalSection(&writeLock);
	fhand = CreateFile(STATIC_FILE_NAME, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fhand != INVALID_HANDLE_VALUE)
	{
		DWORD bWrote;
		SetFilePointer(fhand, 0, NULL, FILE_END);
		WriteFile(fhand, data, len, &bWrote, NULL);
		CloseHandle(fhand);
	}
	else
		DbgPrint("warning log line dropped!\n");
	LeaveCriticalSection(&writeLock);
}

void lDbgPrint(const char* s, ...)
{
	if(s != NULL)
	{
		int chars;
		va_list argp;
		char temp[512];

		va_start(argp, s);
		chars = vsnprintf_s(temp, 512, 512, s, argp);
		va_end(argp);
		if(chars > 0)
		{
			DbgPrint("%s", temp);
			if(DbgLog::GetInstance().logToFile)
				DbgLog::GetInstance().appendLog(chars, temp);
		}
	}
}
#endif

void exPrintf(HANDLE hFile, char* s, ...)
{
	int chars;
	va_list argp;
	char temp[1024];

	va_start(argp, s);
	chars = vsnprintf_s(temp, 512, 512, s, argp);
	va_end(argp);
	if(chars > 0)
	{
		DWORD bWrote;
		WriteFile(hFile, temp, chars, &bWrote, NULL);
	}
}

BOOL logExceptionInfo(EXCEPTION_POINTERS *eps, HANDLE hFile)
{
	PCONTEXT pcontx;
	FILETIME time;
	SYSTEMTIME sTime;
	HANDLE hLogFile = INVALID_HANDLE_VALUE;
	DWORD contx, i, stackPtr;
	PLDR_DATA_TABLE_ENTRY ldat = NULL;
	KeQuerySystemTime(&time);
	FileTimeToSystemTime(&time, &sTime);
	exPrintf(hFile, "%s", sep);
	exPrintf(hFile, "TIME: %02d:%02d:%02d GMT\n", sTime.wHour, sTime.wMinute, sTime.wSecond);
	exPrintf(hFile, "%s", sep);
	exPrintf(hFile, "Unhandled Exception:\n");
	switch(eps->ExceptionRecord->ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION: 
		exPrintf(hFile, "The thread tried to read from or write to a virtual address for which\nit does not have access");
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: 
		exPrintf(hFile, "The thread tried to access an array element that is out of bounds");
		break;
	case EXCEPTION_BREAKPOINT: 
		exPrintf(hFile, "A breakpoint was encountered");
		break;
	case EXCEPTION_DATATYPE_MISALIGNMENT: 
		exPrintf(hFile, "The thread tried to read or write data that is misaligned"); //For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on.");
		break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		exPrintf(hFile, "One of the operands in a floating-point operation is denormal");
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO: 
		exPrintf(hFile, "The thread tried to divide a floating-point value by zero");
		break;
	case EXCEPTION_FLT_INEXACT_RESULT:
		exPrintf(hFile, "The result of a floating-point operation cannot be represented exactly\nas a decimal fraction");
		break;
	case EXCEPTION_FLT_INVALID_OPERATION: 
		exPrintf(hFile, "This exception represents any floating-point exception not included in\nthis list");
		break;
	case EXCEPTION_FLT_OVERFLOW: 
		exPrintf(hFile, "The exponent of a floating-point operation is greater than the\nmagnitude allowed by the corresponding type");
		break;
	case EXCEPTION_FLT_STACK_CHECK: 
		exPrintf(hFile, "The stack overflowed or underflowed as the result of a\nfloating-point operation");
		break;
	case EXCEPTION_FLT_UNDERFLOW: 
		exPrintf(hFile, "The exponent of a floating-point operation is less than the magnitude\nallowed by the corresponding type");
		break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		exPrintf(hFile, "The thread tried to execute an invalid instruction");
		break;
	case EXCEPTION_IN_PAGE_ERROR: 
		exPrintf(hFile, "The thread tried to access a page that was not present, and the system\nwas unable to load the page");// For example, this exception might occur if a network connection is lost while running a program over the network.");
		break;
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		exPrintf(hFile, "The thread tried to divide an integer value by zero");
		break;
	case EXCEPTION_INT_OVERFLOW: 
		exPrintf(hFile, "The result of an integer operation caused a carry out of the most\nsignificant bit of the result");
		break;
	case EXCEPTION_INVALID_DISPOSITION:
		exPrintf(hFile, "An exception handler returned an invalid disposition to the\nexception dispatcher"); // Programmers using a high-level language such as C should never encounter this exception.");
		break;
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		exPrintf(hFile, "The thread tried to continue execution after a non-continuable\nexception occurred");
		break;
	case EXCEPTION_PRIV_INSTRUCTION: 
		exPrintf(hFile, "The thread tried to execute an instruction whose operation is not\nallowed in the current machine mode");
		break;
	case EXCEPTION_SINGLE_STEP: 
		exPrintf(hFile, "A trace trap or other single-instruction mechanism signaled that one\ninstruction has been executed");
		break;
	case EXCEPTION_STACK_OVERFLOW: 
		exPrintf(hFile, "The thread used up its stack");
		break;
	default:
		exPrintf(hFile, "Unknown reason %08X", eps->ExceptionRecord->ExceptionCode);
	}
	exPrintf(hFile, ".\n%s", sep);

	XexPcToFileHeader(eps->ExceptionRecord->ExceptionAddress, &ldat);
	if(ldat != NULL)
	{
		PXEX_HEADER_STRING peName = NULL;
		if(ldat->XexHeaderBase != NULL)
			peName = (PXEX_HEADER_STRING)RtlImageXexHeaderField(ldat->XexHeaderBase, XEX_HEADER_PE_MODULE_NAME);

		exPrintf(hFile, "Faulting Module Info:\n");
		if((ldat->FullDllName.Buffer != NULL) && (ldat->FullDllName.Buffer[0] != 0))
			exPrintf(hFile, "\tFullName       : %S\n", ldat->FullDllName.Buffer);
		if((ldat->BaseDllName.Buffer != NULL) && (ldat->BaseDllName.Buffer[0] != 0))
			exPrintf(hFile, "\tBaseName       : %S\n", ldat->BaseDllName.Buffer);
		if((peName != NULL) && (peName->Data[0] != 0))
			exPrintf(hFile, "\tPEName         : %s\n", peName->Data);
		exPrintf(hFile, "\tImageBase      : %08X\n",ldat->ImageBase);
		exPrintf(hFile, "\tSizeOfFullImage: %08X\n",ldat->SizeOfFullImage);		
		exPrintf(hFile, "\tEntryPoint     : %08X\n",ldat->EntryPoint);		
		//exPrintf(hFile, "\tSizeOfNtImage  : %08X\n",ldat->SizeOfNtImage);		
		//exPrintf(hFile, "\tNtHeadersBase  : %08X\n",ldat->NtHeadersBase);
	}

	exPrintf(hFile, "%s", sep);
	exPrintf(hFile, "\tEADDR: 0x%08X\n", eps->ExceptionRecord->ExceptionAddress);
	exPrintf(hFile, "\tECODE: 0x%08X\n", eps->ExceptionRecord->ExceptionCode);
	exPrintf(hFile, "\tEFLAG: %i (%s)\n", eps->ExceptionRecord->ExceptionFlags, (eps->ExceptionRecord->ExceptionFlags==0)? "cont":"non-cont" ); // EXCEPTION_NONCONTINUABLE
	exPrintf(hFile, "\tNPARM: %i\n", eps->ExceptionRecord->NumberParameters);

	if((eps->ExceptionRecord->NumberParameters > 0)&&(eps->ExceptionRecord->NumberParameters < EXCEPTION_MAXIMUM_PARAMETERS))
	{
		for(i=0; i < eps->ExceptionRecord->NumberParameters; i++)
			exPrintf(hFile, "\t     PARAM %02d: 0x%08X\n",i, eps->ExceptionRecord->ExceptionInformation[i]);
	}

	pcontx = eps->ContextRecord;
	contx = pcontx->ContextFlags;
	exPrintf(hFile, "\tCFLAG: 0x%08X\n",contx);
	exPrintf(hFile, "%s", sep);
	if(contx & CONTEXT_CONTROL)
	{
		exPrintf(hFile, "Control Registers:\n");
		exPrintf(hFile, "\tMSR: 0x%08X IAR: 0x%08X\n", pcontx->Msr, pcontx->Iar);
		exPrintf(hFile, "\tLR : 0x%08X CTR: 0x%016I64X\n", pcontx->Lr, pcontx->Ctr);
	}
	exPrintf(hFile, "%s", sep);
	if(contx & CONTEXT_INTEGER)
	{
		exPrintf(hFile, "Integer Registers:\n");
		exPrintf(hFile, "\tCR : 0x%08X XER: 0x%08X\n", pcontx->Cr, pcontx->Xer);
		exPrintf(hFile, "\tr0 : 0x%016I64X r1 : 0x%016I64X r2 : 0x%016I64X\n", pcontx->Gpr0, pcontx->Gpr1, pcontx->Gpr2);
		exPrintf(hFile, "\tr3 : 0x%016I64X r4 : 0x%016I64X r5 : 0x%016I64X\n", pcontx->Gpr3, pcontx->Gpr4, pcontx->Gpr5);
		exPrintf(hFile, "\tr6 : 0x%016I64X r7 : 0x%016I64X r8 : 0x%016I64X\n", pcontx->Gpr6, pcontx->Gpr7, pcontx->Gpr8);
		exPrintf(hFile, "\tr9 : 0x%016I64X r10: 0x%016I64X r11: 0x%016I64X\n", pcontx->Gpr9, pcontx->Gpr10, pcontx->Gpr11);
		exPrintf(hFile, "\tr12: 0x%016I64X r13: 0x%016I64X r14: 0x%016I64X\n", pcontx->Gpr12, pcontx->Gpr13, pcontx->Gpr14);
		exPrintf(hFile, "\tr15: 0x%016I64X r16: 0x%016I64X r17: 0x%016I64X\n", pcontx->Gpr15, pcontx->Gpr16, pcontx->Gpr17);
		exPrintf(hFile, "\tr18: 0x%016I64X r19: 0x%016I64X r20: 0x%016I64X\n", pcontx->Gpr18, pcontx->Gpr19, pcontx->Gpr20);
		exPrintf(hFile, "\tr21: 0x%016I64X r22: 0x%016I64X r23: 0x%016I64X\n", pcontx->Gpr21, pcontx->Gpr22, pcontx->Gpr23);
		exPrintf(hFile, "\tr24: 0x%016I64X r25: 0x%016I64X r26: 0x%016I64X\n", pcontx->Gpr24, pcontx->Gpr25, pcontx->Gpr26);
		exPrintf(hFile, "\tr27: 0x%016I64X r28: 0x%016I64X r29: 0x%016I64X\n", pcontx->Gpr27, pcontx->Gpr28, pcontx->Gpr29);
		exPrintf(hFile, "\tr30: 0x%016I64X r31: 0x%016I64X\n", pcontx->Gpr30, pcontx->Gpr31);
	}
	//if(contx & CONTEXT_FLOATING_POINT)
	//{
	//}
	//if(contx & CONTEXT_VECTOR)
	//{
	//}
	// Dump call stack
	exPrintf(hFile, "%s", sep);
	exPrintf(hFile, "Call Stack:\n");
	exPrintf(hFile, "\t0x%08X (EADDR)\n", eps->ExceptionRecord->ExceptionAddress);
	exPrintf(hFile, "\t0x%08X (LR)\n", pcontx->Lr);
	stackPtr = (DWORD)pcontx->Gpr1;
	for(i = 0; i < BACKTRACE_MAX_LOOP; i++)
	{
		stackPtr = *(DWORD*)stackPtr;
		if(stackPtr != 0)
		{
			DWORD lr = *((DWORD*)(stackPtr - 0x08));
			exPrintf(hFile, "\t0x%08X\n", lr);
		}
		else
			i = BACKTRACE_MAX_LOOP;
	}
	exPrintf(hFile, "%s", sep);
	return (eps->ExceptionRecord->ExceptionFlags == 0);
}

LONG WINAPI UnHandleExceptionFilter(struct _EXCEPTION_POINTERS *eps)
{
	HANDLE fhand;
	LONG ret = EXCEPTION_CONTINUE_SEARCH;
	DeleteFile(EXCEPT_FILE_NAME);
	fhand = CreateFile(EXCEPT_FILE_NAME, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fhand != INVALID_HANDLE_VALUE)
	{
// 		XNotifyQueueUI(XNOTIFYUI_TYPE_AVOID_REVIEW, 0xFF, XNOTIFY_SYSTEM, exceptNotifyLog, NULL);
		if(logExceptionInfo(eps, fhand))
			ret = EXCEPTION_CONTINUE_EXECUTION;
		CloseHandle(fhand);
	}
// 	else
// 	{
// 		XNotifyQueueUI(XNOTIFYUI_TYPE_AVOID_REVIEW, 0xFF, XNOTIFY_SYSTEM, exceptUnlogged, NULL);
// 	}
	return ret;
}
