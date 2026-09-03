
#ifndef _KERNEL_DEFINES_H
#define _KERNEL_DEFINES_H

#include "types.h"

#ifdef _XBOX
#else
#include <windows.h>
#endif

#define CONSTANT_OBJECT_STRING(s)   { strlen( s ) / sizeof( OCHAR ), (strlen( s ) / sizeof( OCHAR ))+1, s }
#define MAKE_STRING(s)   {(USHORT)(strlen(s)), (USHORT)((strlen(s))+1), s}

#define SYS_STRING	"\\System??\\%s"
#define USR_STRING	"\\??\\%s"
#define FUSR_STRING	"\\??\\f%s"

//#define IN
//#define OUT
//#define OPTIONAL
#define STATUS_SUCCESS	0
#define FILE_SYNCHRONOUS_IO_NONALERT	0x20

#define NT_EXTRACT_ST(Status)			((((ULONG)(Status)) >> 30)& 0x3)
#define NT_SUCCESS(Status)              (((NTSTATUS)(Status)) >= 0)
#define NT_INFORMATION(Status)          (NT_EXTRACT_ST(Status) == 1)
#define NT_WARNING(Status)              (NT_EXTRACT_ST(Status) == 2)
#define NT_ERROR(Status)                (NT_EXTRACT_ST(Status) == 3)

// Valid values for the Attributes field
#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000010L
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_OPENLINK            0x00000100L
#define OBJ_VALID_ATTRIBUTES    0x000001F2L

// Directory Stuff
#define DIRECTORY_QUERY                 (0x0001)
#define DIRECTORY_TRAVERSE              (0x0002)
#define DIRECTORY_CREATE_OBJECT         (0x0004)
#define DIRECTORY_CREATE_SUBDIRECTORY   (0x0008)

#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)

#define SYMBOLIC_LINK_QUERY (0x0001)

// object type strings
#define OBJ_TYP_SYMBLINK	0x626d7953 // Symb
#define OBJ_TYP_DIRECTORY	0x65726944 // Dire
#define OBJ_TYP_DEVICE		0x69766544 // Devi
#define OBJ_TYP_EVENT		0x76657645 // Evev
#define OBJ_TYP_DEBUG		0x63706d64 // dmpc

typedef long			NTSTATUS;
typedef ULONG			ACCESS_MASK;

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} STRING, *PSTRING;
typedef struct _CSTRING {
	USHORT Length;
	USHORT MaximumLength;
	CONST char *Buffer;
} CSTRING, *PCSTRING;
typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
typedef STRING			OBJECT_STRING;
typedef CSTRING			COBJECT_STRING;
typedef PSTRING			POBJECT_STRING;
typedef PCSTRING		PCOBJECT_STRING;
typedef STRING			OEM_STRING;
typedef PSTRING			POEM_STRING;
typedef CHAR			OCHAR;
typedef CHAR*			POCHAR;
typedef PSTR			POSTR;
typedef PCSTR			PCOSTR;
typedef CHAR*			PSZ;
typedef CONST CHAR*		PCSZ;
typedef STRING			ANSI_STRING;
typedef PSTRING			PANSI_STRING;
typedef CSTRING			CANSI_STRING;
typedef PCSTRING		PCANSI_STRING;
#define ANSI_NULL		((CHAR)0)     // winnt
typedef CONST UNICODE_STRING*	PCUNICODE_STRING;
#define UNICODE_NULL			((WCHAR)0) // winnt

#define OTEXT(quote) __OTEXT(quote)


typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    } st;
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef VOID (NTAPI *PIO_APC_ROUTINE) (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    );

typedef struct _OBJECT_ATTRIBUTES {
    HANDLE RootDirectory;
    POBJECT_STRING ObjectName;
    ULONG Attributes;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _OBJECT_DIRECTORY_INFORMATION{
	STRING Name;
	DWORD Type;
} OBJDIR_INFORMATION, *POBJDIR_INFORMATION; // 12b

#define InitializeObjectAttributes( p, n, a, r){ \
	(p)->RootDirectory = r;                             \
	(p)->Attributes = a;                                \
	(p)->ObjectName = n;                                \
}

// returned by a call to 'NtQueryInformationFile' with 0x22 = FileNetworkOpenInformation
typedef struct _FILE_NETWORK_OPEN_INFORMATION {
  LARGE_INTEGER  CreationTime;
  LARGE_INTEGER  LastAccessTime;
  LARGE_INTEGER  LastWriteTime;
  LARGE_INTEGER  ChangeTime;
  LARGE_INTEGER  AllocationSize;
  LARGE_INTEGER  EndOfFile;
  ULONG  FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;


typedef struct _XBOX_HARDWARE_INFO{
	unsigned long Flags;
	unsigned char NumberOfProcessors;
	unsigned char PCIBridgeRevisionID;
	unsigned char Reserved[6];
	unsigned short BldrMagic;
	unsigned short BldrFlags;
} XBOX_HARDWARE_INFO, *PXBOX_HARDWARE_INFO;

typedef struct _DISPLAY_INFO{
	USHORT timing1; // 0x0
	USHORT timing2; // 0x2
	BYTE colorspace; // 0x4
	BYTE colorformat; // 0x5
	BYTE padb_1[2]; // 0x6 pad
	DWORD pitch; // 0x8
	DWORD format; // 0xC
	DWORD offsetx; // 0x10
	DWORD offsety; // 0x14
	DWORD sw; // 0x18
	DWORD sh; // 0x1c
	DWORD dwUnk1;
	BYTE baUnk1[12];
	DWORD dwUnk2;
	BYTE baUnk2[12];
	USHORT waUnk1[6]; // last two may be important
	DWORD dwAsFloat; // is a float?
	DWORD dwUnk3;
	BYTE padb_2[2]; //pad
	USHORT wUnk1;	
} DISPLAY_INFO, *PDISPLAY_INFO; // total size 0x58 bytes
C_ASSERT(sizeof(DISPLAY_INFO) == 0x58);

typedef struct _KTIME_STAMP_BUNDLE { 
	LARGE_INTEGER volatile InterruptTime; // 0
	LARGE_INTEGER volatile SystemTime; // 8
	DWORD volatile TickCount; // 10
} KTIME_STAMP_BUNDLE, *PKTIME_STAMP_BUNDLE;

typedef ULONG_PTR KIPI_BROADCAST_WORKER (
	__in ULONG_PTR Argument
	);
typedef KIPI_BROADCAST_WORKER *PKIPI_BROADCAST_WORKER;

typedef struct _MM_STATISTICS{
	DWORD Length;
	DWORD TotalPhysicalPages;
	DWORD KernelPages;
	DWORD TitleAvailablePages;
	DWORD TitleTotalVirtualMemoryBytes;
	DWORD TitleReservedVirtualMemoryBytes;
	DWORD TitlePhysicalPages;
	DWORD TitlePoolPages;
	DWORD TitleStackPages;
	DWORD TitleImagePages;
	DWORD TitleHeapPages;
	DWORD TitleVirtualPages;
	DWORD TitlePageTablePages;
	DWORD TitleCachePages;
	DWORD SystemAvailablePages;
	DWORD SystemTotalVirtualMemoryBytes;
	DWORD SystemReservedVirtualMemoryBytes;
	DWORD SystemPhysicalPages;
	DWORD SystemPoolPages;
	DWORD SystemStackPages;
	DWORD SystemImagePages;
	DWORD SystemHeapPages;
	DWORD SystemVirtualPages;
	DWORD SystemPageTablePages;
	DWORD SystemCachePages;
	DWORD HighestPhysicalPage;
} MM_STATISTICS, *PMMSTATISTICS; // should be 104bytes in size
C_ASSERT(sizeof(MM_STATISTICS) == 104);

typedef struct _LDR_DATA_TABLE_ENTRY { 
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InClosureOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID NtHeadersBase;
	PVOID ImageBase;
	DWORD SizeOfNtImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	DWORD Flags;
	DWORD SizeOfFullImage;
	PVOID EntryPoint;
	WORD LoadCount;
	WORD ModuleIndex;
	PVOID DllBaseOriginal;
	DWORD CheckSum;
	DWORD ModuleLoadFlags;
	DWORD TimeDateStamp;
	PVOID LoadedImports;
	PVOID XexHeaderBase;
	union{
		STRING LoadFileName;
		struct {
			PVOID ClosureRoot; // LDR_DATA_TABLE_ENTRY
			PVOID TraversalParent; // LDR_DATA_TABLE_ENTRY
		} asEntry;
	} inf;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _XEX_HEADER_STRING {
	ULONG Size;
	UCHAR Data[1];
} XEX_HEADER_STRING, *PXEX_HEADER_STRING;

typedef struct _XBOX_KRNL_VERSION{
	USHORT Major; // for 360 this is always 2
	USHORT Minor; // usually 0
	USHORT Build; // current version, for example 9199
	USHORT Qfe;
} XBOX_KRNL_VERSION, *PXBOX_KRNL_VERSION;

enum {
	THREADFLAGS_NONE = 0x0,
	THREADFLAGS_TYPE_SYNCHRONOUS = 0x01000000,
	THREADFLAGS_TYPE_DEDICATED = 0x10000000,
	THREADFLAGS_TYPE_POOLED = 0x20000000,
	THREADFLAGS_TYPE_UI = 0x40000000,
	THREADFLAGS_TYPE_SCHEDULER = 0x80000000,
};

#ifdef __cplusplus
extern "C" {
#endif
	extern PHANDLE XexExecutableModuleHandle;
	extern PXBOX_KRNL_VERSION XboxKrnlVersion;

	void DbgPrint(const char* s, ...);

	PVOID RtlImageXexHeaderField(
		IN		PVOID XexHeaderBase,
		IN		DWORD ImageField
		);

	NTSTATUS WINAPI NtOpenSymbolicLinkObject(
		__out  PHANDLE LinkHandle,
		__in   POBJECT_ATTRIBUTES ObjectAttributes
		);

	NTSTATUS WINAPI NtQuerySymbolicLinkObject(
		__in       HANDLE LinkHandle,
		__inout    PSTRING LinkTarget,
		__out_opt  PULONG ReturnedLength
		);

	NTSTATUS WINAPI NtClose(__in  HANDLE Handle);

	NTSTATUS WINAPI NtOpenDirectoryObject( // 222
		__out  PHANDLE DirectoryHandle,
		__in   POBJECT_ATTRIBUTES ObjectAttributes
		);

	NTSTATUS WINAPI NtQueryDirectoryObject( // 229
		__in       HANDLE DirectoryHandle,
		__out_opt  PVOID Buffer,
		__in       ULONG Length,
		__in       BOOLEAN ReturnSingleEntry,
		//__in       BOOLEAN RestartScan,
		__inout    PULONG Context,
		__out_opt  PULONG ReturnLength
		);
	

	DWORD __stdcall ExCreateThread(PHANDLE pHandle, DWORD dwStackSize, LPDWORD lpThreadId, VOID* apiThreadStartup , LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, QWORD dwCreationFlagsMod);
//DWORD __stdcall ExCreateThread(HANDLE* pHandle, DWORD dwStackSize, DWORD* lpThreadId, VOID* apiThreadStartup , VOID* lpStartAddress, VOID* lpParameter, QWORD dwCreationFlagsMod);

	VOID __cdecl XapiThreadStartup(VOID (__cdecl *StartRoutine)(VOID *), VOID *StartContext);
	HANDLE __stdcall XapipCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, DWORD dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, DWORD dwThreadProcessor, LPDWORD lpThreadId);
	HRESULT __stdcall ObCreateSymbolicLink( STRING*, STRING*);
	HRESULT __stdcall ObDeleteSymbolicLink( STRING* );
	int __stdcall MmQueryAddressProtect(UINT64 Adr);
	UINT64 __stdcall MmGetPhysicalAddress(UINT64  BaseAddress);
	DWORD MmIsAddressValid(UINT64 addr);
	DWORD __stdcall MmSetAddressProtect(VOID *Adr, int Size, int Type);// PAGE_READWRITE
	DWORD __stdcall MmQueryStatistics(PMMSTATISTICS);
	VOID __stdcall HalSendSMCMessage(LPVOID input, LPVOID output);
	DWORD __stdcall XexGetModuleHandle(char* module, PVOID hand); //ie XexGetModuleHandle("xam.xex", &hand);// uint32 hand, returns 0 on success
	DWORD __stdcall XexGetProcedureAddress(HANDLE hand, DWORD, DWORD*);// ie XexGetProcedureAddress(hand ,0x50, &addr); // uint32 addr, returns 0 on success
	extern PXBOX_HARDWARE_INFO XboxHardwareInfo;
	extern PDWORD ExConsoleGameRegion;
	unsigned char KeGetCurrentProcessType(VOID);
	VOID __stdcall HalReturnToFirmware(DWORD mode); // 2 is hard reboot, 5 is power down
	//export # 442 
	VOID VdGetCurrentDisplayInformation(PDISPLAY_INFO dispInfo);
	//export # 173 (data var)
	extern PKTIME_STAMP_BUNDLE KeTimeStampBundle;
	//export # 97
	VOID KeFlushCacheRange(PVOID address, DWORD size);
	PVOID ExAllocatePool(DWORD NumberOfBytes);
	VOID ExFreePool(PVOID  pPool);
	PDWORD KeIpiGenericCall(PKIPI_BROADCAST_WORKER BroadcastFunction, PDWORD Context);
	unsigned char KeGetCurrentProcessType(void);
	VOID RtlInitAnsiString(PANSI_STRING DestinationString, PCSZ SourceString);
#ifdef __cplusplus
}
#endif




#endif	//_KERNEL_DEFINES_H

