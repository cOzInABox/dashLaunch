//--------------------------------------------------------------------------------------
//	start xell from a xex, required documented hv patch below to work
// maybe try to expand 16 * 1024 * 1024 to 32MB instead
//--------------------------------------------------------------------------------------
#include <xtl.h>
#include <stdio.h>
#include <ppcintrinsics.h>
#include "kernel.h"

#define XELL_OFFSET_JTAG	0x80000200C8095060ULL
#define XELL_OFFSET_GLITCH	0x80000200C8070000ULL
#define XELL_DEST_JTAG		0x800000001c040000ULL
#define XELL_DEST_GLITCH	0x800000001c000000ULL
#define SYSCALL_KEY			0x72627472 // rbtr
#define CB_ZP_OFFSET		0x8000

#define CON_UNK			0
#define CON_JTAG		1
#define CON_GLITCH		2

HRESULT Mount(PCHAR szDrive, PCHAR szDevice)
{
	STRING DeviceName, LinkName;
	CHAR szDestinationDrive[MAX_PATH];
	sprintf_s(szDestinationDrive, MAX_PATH, "\\??\\%s", szDrive);
	RtlInitAnsiString(&DeviceName, szDevice);
	RtlInitAnsiString(&LinkName, szDestinationDrive);
	ObDeleteSymbolicLink(&LinkName);
	return (HRESULT)ObCreateSymbolicLink(&LinkName, &DeviceName);
}

// one more modification to our favorite syscall... should be possible to use this
// to exec arbitrary code in hv state, registers r7-r10 will be forwarded
// just make sure to preserve the link register and blr when done
// (r4 = 4, r5 = dest/entry, r6 = src, r7 = lenInU32)
DWORD __declspec(naked) HvxSetState(DWORD key, DWORD mode, UINT64 dest, UINT64 src, UINT64 linInU32)
{ 
	__asm {
		li      r0, 0x0
		sc
		blr
	}
}
/* the code that is actually called here...
		cmplwi  cr6, %r3, 4			# xell loader
		beq		cr6, bin_launch
	...
bin_launch:
		mflr	%r12  # stow on stack
		std		%r12, -0x8(%sp)
		stdu    %sp, -0x10(%sp)
		mtlr 	%r4
		mtctr   %r6
		# Copy src = %r5, dst = %r6
lcopyloop:
		lwz     %r3, 0(%r5)
		stw     %r3, 0(%r4)
		dcbst   %r0, %r4	
		icbi	%r0, %r4
		sync	0
		isync
		addi    %r4, %r4, 4
		addi    %r5, %r5, 4
		bdnz    lcopyloop
		blr
		addi	%sp, %sp, 0x10
		ld		%r12, -0x8(%sp)
		mtlr	%r12	#loaded from stack
		blr	
*/

char* devices[] = {
	"\\Device\\Mass0\\",
	"\\Device\\Mass1\\",
	"\\Device\\Mass2\\",
	"\\Device\\Harddisk0\\Partition1\\",
	"\\Device\\Cdrom0\\",
};
char fileName[] = "xl:\\xell.bin";

PBYTE Buffer;
DWORD ExecSize;

#define write32(x,y) (__storewordbytereverse(y, 0, (void*)x))
#define read32(x) (__loadwordbytereverse(0, (void*)x))

void usb_shutdown(void)
{
	// EHCI
	// disable interrupts
	write32(0x7FEA3028,0);
	write32(0x7FEA5028,0);
	// halt
	write32(0x7FEA3020,0);
	write32(0x7FEA5020,0);

	// OHCI
	// disable interrupts
	write32(0x7FEA2014,1 << 31);
	write32(0x7FEA4014,1 << 31);
	// halt
	write32(0x7FEA2004,0);
	read32(0x7FEA2004);
	write32(0x7FEA4004,0);
	read32(0x7FEA4004);
}

void startXell(DWORD useNand)
{
//	UINT64 dest = 0x8000000013000000ULL; // xell-ll
//	UINT64 dest = 0x800000001c000000ULL; // xell-1f
	UINT64 dest = XELL_DEST_JTAG; // 0x800000001c040000ULL xell-2f
	UINT64 src = 0x8000000000000000ULL;
	UINT64 len = 0ULL+((ExecSize/4)& 0xFFFFFFFF);

	DbgPrint("shut down USB\n");
	usb_shutdown();
	
	
	src = src+((DWORD)MmGetPhysicalAddress(Buffer));
	DbgPrint("syscall - starting xell file\n");
	HvxSetState(SYSCALL_KEY, 4, dest, src, len);
}



DWORD getSize(const char* path)
{
	WIN32_FILE_ATTRIBUTE_DATA att;
	if(!GetFileAttributesEx(path, GetFileExInfoStandard, &att))
	{
		DbgPrint("unable to get attr, err %08x\n", GetLastError());
		return 0;
	}
	return att.nFileSizeLow;
}

DWORD readFile(const char* path, PBYTE dest)
{
	DWORD size = 0;
	size = getSize(path);
	//DbgPrint("'%s' size is %x\n", path, size);
	if(size != 0)
	{
		DWORD read = 0;
		HANDLE file;
		file = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(file == INVALID_HANDLE_VALUE)
			return 0;
		if(dest == NULL)
		{
			DbgPrint("error allocating phy buffer for file read\n");
			CloseHandle(file);
			return 0;
		}
		ZeroMemory(dest, size);
		ReadFile(file, dest, size, &read, NULL);
		CloseHandle(file);
	}
	return size;
}

int addFooter(PBYTE dest){
	//"xxxxxxxxxxxxxxxx"
	int i =0;
	for(i=0;i<0x10;i++){
		dest[i]='x';
	}
	return 0x10;
}

void save_to_dd(const char *path){
	FILE * fd = fopen(path,"wb");
	fwrite(Buffer,ExecSize,1,fd);
	fclose(fd);
}

VOID __cdecl main()
{
	PBYTE xell_buff = NULL;
	PBYTE elf_buff = NULL;
	int loadersize,buffersize,elfsize;

	Mount("xl:",devices[0]);
	DbgPrint("\n\nXellLaunch v.%d.%02d (%d) started\n", 0, 1, 2);

	// Allocate general buffer
	Buffer = (PBYTE)XPhysicalAlloc(16*1024*1024, MAXULONG_PTR, 0, MEM_LARGE_PAGES|PAGE_READWRITE|PAGE_NOCACHE);
	memset(Buffer,0,16*1024*1024);
	xell_buff = Buffer;
	// if not check in the launched xex path
	loadersize = readFile("GAME:\\1stage.bin",xell_buff);	
	elf_buff = Buffer + 16384;
	//elfsize = readFile("GAME:\\xenon.elf.gz",elf_buff);
	elfsize = readFile("xl:\\mplayer_xenon.elf32",elf_buff);
	buffersize = addFooter(Buffer + ((16*1024*1024) - 0x10));
	ExecSize = elfsize+loadersize;

	ExecSize = 16*1024*1024;

	//save_to_dd("game:\\test.bin");

	if(ExecSize != 0)
		startXell(FALSE);
	
	DbgPrint("could not decide what type of xell to launch from flash, so I'm going back to dash\n");
}