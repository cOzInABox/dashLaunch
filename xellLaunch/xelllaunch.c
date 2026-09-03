//--------------------------------------------------------------------------------------
//	start xell from a xex, required documented hv patch below to work
//--------------------------------------------------------------------------------------
#include <xtl.h>
#include <stdio.h>
#include <ppcintrinsics.h>
#include "kernel.h"
#include "../_common.h"

#define XELL_OFFSET_JTAG	0x80000200C8095060ULL
#define XELL_OFFSET_GLITCH	0x80000200C8070000ULL
#define XELL_DEST_GLITCH_1F	0x800000001c000000ULL // xell 1f
#define XELL_DEST_JTAG_2F	0x800000001c040000ULL // xell 2f
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
DWORD __declspec(naked) HvxSetState(DWORD key, DWORD mode, UINT64 dest, UINT64 src, UINT64 lenInU32)
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

PBYTE xell_buf;
DWORD xellsize;

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

void startXell(DWORD useNand, UINT64 usedest)
{
//	UINT64 dest = 0x8000000013000000ULL; // xell-ll
//	UINT64 dest = 0x800000001c000000ULL; // xell-1f
	UINT64 dest = usedest; // 0x800000001c040000ULL xell-2f
	UINT64 src = 0x8000000000000000ULL;

	DbgPrint("shut down USB\n");
	usb_shutdown();
	if(useNand != 0)
	{
		UINT64 len = 0x0000000000010000ULL;
		if(useNand == CON_JTAG)
		{
			src = XELL_OFFSET_JTAG; // 0x80000200C8095060ULL
			DbgPrint("syscall - starting xell nand (jtag)\n");
		}
		else
		{
			src = XELL_OFFSET_GLITCH; // 0x80000200C8070000ULL
			dest = XELL_DEST_GLITCH_1F; // 0x800000001c000000ULL xell-1f
			DbgPrint("syscall - starting xell nand (glitch)\n");
		}
		HvxSetState(SYSCALL_KEY, 4, dest, src, len);
	}
	else
	{
		DWORD sz = (xellsize+3)&~3;
		UINT64 len = 0ULL+((sz/4)& 0xFFFFFFFF);
		src = src+((DWORD)MmGetPhysicalAddress(xell_buf));
		DbgPrint("syscall - starting xell file\n");
		HvxSetState(SYSCALL_KEY, 4, dest, src, len);
	}
}

DWORD getSize(const char* path)
{
	WIN32_FILE_ATTRIBUTE_DATA att;
	if(!GetFileAttributesEx(path, GetFileExInfoStandard, &att))
	{
		DbgPrint("GetFileAttributesEx failed, err %08x (%s)\n", GetLastError(), path);
		return 0;
	}
	return att.nFileSizeLow;
}

DWORD readFile(const char* path)
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
		xell_buf = (PBYTE)XPhysicalAlloc(size, MAXULONG_PTR, 0, MEM_LARGE_PAGES|PAGE_READWRITE|PAGE_NOCACHE);
		if(xell_buf == NULL)
		{
			DbgPrint("error allocating phy buffer for file read\n");
			CloseHandle(file);
			return 0;
		}
		ZeroMemory(xell_buf, size);
		ReadFile(file, xell_buf, size, &read, NULL);
		CloseHandle(file);
	}
	return size;
}

DWORD readFlash(DWORD offset, PBYTE buf, DWORD len)
{
	HANDLE hFile;
	OBJECT_ATTRIBUTES atFlash;
	IO_STATUS_BLOCK ioFlash;
	DWORD bRead = 0, dwPos;
	DWORD sta;
	STRING nFlash = MAKE_STRING("\\Device\\Flash");
	atFlash.RootDirectory = 0;
	atFlash.ObjectName = &nFlash;
	atFlash.Attributes = FILE_ATTRIBUTE_DEVICE;
	sta = NtOpenFile(&hFile, GENERIC_WRITE|GENERIC_READ|SYNCHRONIZE, &atFlash, &ioFlash, OPEN_EXISTING, FILE_SYNCHRONOUS_IO_NONALERT);
	if(sta != 0)
	{
		DbgPrint("NtOpenFile(flash): %08x (err: %08x)\n", sta, GetLastError());
		return FALSE;
	}

	dwPos = SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
	if(dwPos != INVALID_SET_FILE_POINTER)
	{
		ZeroMemory(buf, len);
		ReadFile(hFile, buf, len, &bRead, NULL);
		NtClose(hFile);
	}
	else
		DbgPrint("SetFilePointer(flash) invalid! %08x\n", sta, GetLastError());

	return bRead;
}
DWORD getConType(void)
{
	BYTE cBuf[4];
	DWORD rlen;
	USHORT ver;
	DWORD ret = CON_UNK;
	// read 8 bytes from 0x8000 offset in NAND
	memset(cBuf, 0x0, 4);
	rlen = readFlash(CB_ZP_OFFSET, cBuf, 4);

	ver = ((cBuf[0]&0xFF)<<8)|(cBuf[1]&0xFF);
	if(ver != 0x4342)//0x43 0x42 = C B
		return 0xFFFFFFFF;
	ver = ((cBuf[2]&0xFF)<<8)|(cBuf[3]&0xFF);
	DbgPrint("CB%04d found, type: ", ver);
	switch(ver)
	{
		case 1921:
			DbgPrint("xenon (jtag)\n");
			ret = CON_JTAG;
			break;
		case 4558:
			DbgPrint("zephyr (jtag)\n");
			ret = CON_JTAG;
			break;
		case 5770:
			DbgPrint("falcon (jtag)\n");
			ret = CON_JTAG;
			break;
		case 6723:
			DbgPrint("jasper (jtag)\n");
			ret = CON_JTAG;
			break;
		case 4577:
		case 4579: // zephyr glitch
			DbgPrint("zephyr (glitch)\n");
			ret = CON_GLITCH;
			break;
		case 5771: // falcon glitch
		case 5772:
			DbgPrint("falcon (glitch)\n");
			ret = CON_GLITCH;
			break;
		case 6750: // jasper glitch
		case 6752:
			DbgPrint("jasper (glitch)\n");
			ret = CON_GLITCH;
			break;
		case 9188: // trinity glitch
			DbgPrint("trinity (glitch)\n");
			ret = CON_GLITCH;
			break;
		case 13121: // corona glitch
			DbgPrint("corona (glitch)\n");
			ret = CON_GLITCH;
			break;

		default:
			DbgPrint("Unknown\n", cBuf[2], cBuf[3]);
			ret = CON_UNK;
			break;
	}
	return ret;
}

VOID __cdecl main()
{
	int i;
	DWORD typ;
	DbgPrint("\n\nXellLaunch v.%d.%02d (%d) started\n", VER_MAJ, VER_MIN, VER_SVN);
	// check for xell.bin on mass0, mass1, mass2, hdd then CD; if found load it
	for(i = 0; i < 5; i++)
	{
		Mount("xl:", devices[i]);
		xellsize = readFile("xl:\\xell.bin");
		if(xellsize != 0)
			startXell(FALSE, XELL_DEST_GLITCH_1F); // XELL_DEST_JTAG_2F
		else
		{
			xellsize = readFile("xl:\\xell-1f.bin");
			if(xellsize != 0)
				startXell(FALSE, XELL_DEST_GLITCH_1F);
			else
			{
				xellsize = readFile("xl:\\xell-2f.bin");
				if(xellsize != 0)
					startXell(FALSE, XELL_DEST_JTAG_2F);
			}

		}
	}

	// if not check in the launched xex path
	xellsize = readFile("GAME:\\xell.bin");
	if(xellsize != 0)
		startXell(FALSE, XELL_DEST_GLITCH_1F);
	xellsize = readFile("GAME:\\xell-1f.bin");
	if(xellsize != 0)
		startXell(FALSE, XELL_DEST_GLITCH_1F);
	xellsize = readFile("GAME:\\xell-2f.bin");
	if(xellsize != 0)
		startXell(FALSE, XELL_DEST_JTAG_2F);

	typ = getConType();
	
	// if not use NAND
	if(typ == CON_JTAG) 
		startXell(typ, XELL_DEST_JTAG_2F);
	else if(typ == CON_GLITCH) 
		startXell(typ, XELL_DEST_GLITCH_1F);

	DbgPrint("could not decide what type of xell to launch from flash, so I'm going back to dash\n");

	// all else fails (it'll crash first unless the patches are missing) just drop back to dash
	//XLaunchNewImage(XLAUNCH_KEYWORD_DEFAULT_APP, 0);
}
