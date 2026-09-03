#include <xtl.h>
#include <ppcintrinsics.h>
#include <string>
#include "xkelib.h"
#include "util.h"
#include "nand.h"
#include "sfcx.h"
#include "version.h"
#include "logging.h"

using namespace std;


#define MAKE_NAND_VER(x,y)	(((x&0xFFFFFF)<<8)|(y&0xFF))
#define USE_PEEK_VER	(nandInfo.structVer&0xFF)

#define DEVICE_FLASH	"\\Device\\Flash"
#define IS_MMC_DEVICE	((XboxHardwareInfo->Flags&0x20000) != 0)

#define PATCH_MASK_JTAG			(0x80000000)
#define PATCH_MASK_GLITCH		(0x40000000)
// #define PATCH_MASK_GLITCH2	(PATCH_MASK_GLITCH|0x20000000)
#define PATCH_MASK_GLITCH2		(0x20000000)
#define PATCH_MASK_GLITCH2MFG	(0x10000000)

typedef enum {
	BL_TYPE_XENON = 0,
	BL_TYPE_ZEPHYR,
	BL_TYPE_FALCON,
	BL_TYPE_JASPER,
	BL_TYPE_TRINITY,
	BL_TYPE_CORONA,
	BL_TYPE_WINCHESTER,
	BL_TYPE_RIDGEWAY,
	BL_TYPE_UNKNOWN
} BL_TYPE;

static CHAR* consoleModel[] = {
	"Xenon",
	"Zephyr",
	"Falcon",
	"Jasper",
	"Trinity",
	"Corona",
	"Winchester",
	"Unknown"
};

Nand::Nand()
{
	classIsReady = FALSE;
	nandData = NULL;
	phyBuf = NULL;
	workerState = WSTM_NORUN;
	workerEvent = NULL;
	kvBin = NULL;
	smcBin = NULL;
	g_blockBuf = NULL;
	freebootBin = NULL;
	w_currPatchData = NULL;
	w_currPatchSize = 0;
	w_PatchInfo = FALSE;
	w_extras = NULL;
	w_extraSize = 0;
	w_BlMod = NULL;
	w_BlModSz = 0;
	memset(&nandInfo, 0, sizeof(NAND_INFO));
	memset(headerDat, 0, 512);
	nandInfo.structVer = MAKE_NAND_VER(CURRENT_VERSION, HvxGetPeekVer());
	nandInfo.hwFlags = XboxHardwareInfo->Flags;
	if(enumerateNand())
	{
		if(readConsoleData())
		{
			classIsReady = TRUE;
		}
#ifdef FULL_DEBUG_OUT
		else
			DbgLog::GetInstance().log("Nand::readConsoleData failed!!\n");
#endif

	}
#ifdef FULL_DEBUG_OUT
	else
		DbgLog::GetInstance().log("Nand::enumerate NAND failed!!\n");

	if(!classIsReady)
		DbgLog::GetInstance().log("Nand::class init failed!!\n");
	else
		DbgLog::GetInstance().log("Nand::class init succeeded!\n");
#endif
}

Nand::~Nand()
{
	if(kvBin != NULL)
		VirtualFree(kvBin, 0, MEM_RELEASE);
	kvBin = NULL;

	if(smcBin != NULL)
		VirtualFree(smcBin, 0, MEM_RELEASE);
	smcBin = NULL;

	if(freebootBin != NULL)
		VirtualFree(freebootBin, 0, MEM_RELEASE);
	freebootBin = NULL;

	if(w_currPatchData != NULL)
		VirtualFree(w_currPatchData, 0, MEM_RELEASE);
	w_currPatchData = NULL;

	if(w_extras != NULL)
		VirtualFree(w_extras, 0, MEM_RELEASE);
	w_extras = NULL;

	if(w_BlMod != NULL)
		VirtualFree(w_BlMod, 0, MEM_RELEASE);
	w_BlMod = NULL;
}

// this starts the connection listener thread
static DWORD NandWorkerThreadStart(void* Param)
{
	Nand* This = (Nand*) Param;
	return This->workerThread();
}

void Nand::writereg(DWORD addr, DWORD data)
{
	*(volatile DWORD*)(NAND_DEVICE_BASE | addr) = _byteswap_ulong(data);
	__eieio();
// 	__emit(0x7C0006AC); // eieio
}

DWORD Nand::readreg(DWORD addr)
{
// 	return __loadvolatilewordbytereverse(NAND_DEVICE_BASE, NULL); // read in the flash config word
	return _byteswap_ulong(*(volatile unsigned int*)(NAND_DEVICE_BASE | addr));
}

DWORD Nand::GetFlashFsSize(void)
{
	HANDLE hFile;
	OBJECT_ATTRIBUTES atFlash;
	IO_STATUS_BLOCK ioFlash;
	DWORD dwPos1, dwPos2, rval = 0, sta;
	STRING nFlash;
	RtlInitAnsiString(&nFlash, DEVICE_FLASH);
	atFlash.RootDirectory = 0;
	atFlash.ObjectName = &nFlash;
	atFlash.Attributes = FILE_ATTRIBUTE_DEVICE;
	sta = NtOpenFile(&hFile, GENERIC_WRITE|GENERIC_READ|SYNCHRONIZE, &atFlash, &ioFlash, OPEN_EXISTING, FILE_SYNCHRONOUS_IO_NONALERT);
	if(sta != 0)
	{
		DbgLog::GetInstance().log("NtOpenFile(flash): %08x (err: %08x)\n", sta, GetLastError());
		return 0;
	}

	dwPos1 = SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	if(dwPos1 != INVALID_SET_FILE_POINTER)
	{
		dwPos2 = SetFilePointer(hFile, 0, NULL, FILE_END);
		if(dwPos2 != INVALID_SET_FILE_POINTER)
			rval = dwPos2-dwPos1;
		else
			DbgLog::GetInstance().log("SetFilePointer2(flash) invalid! %08x\n", sta, GetLastError());
	}
	else
		DbgLog::GetInstance().log("SetFilePointer(flash) invalid! %08x\n", sta, GetLastError());
	NtClose(hFile);
	return rval;
}

DWORD Nand::ReadFlash(DWORD offset, PBYTE buf, DWORD len, DWORD readSize, PDWORD pbRead)
{
	HANDLE hFile;
	OBJECT_ATTRIBUTES atFlash;
	IO_STATUS_BLOCK ioFlash;
	DWORD bRead = 0, ttlRead = 0, dwPos, sta;
	DWORD rdsz = readSize;
	STRING nFlash;
	RtlInitAnsiString(&nFlash, DEVICE_FLASH);
	atFlash.RootDirectory = 0;
	atFlash.ObjectName = &nFlash;
	atFlash.Attributes = FILE_ATTRIBUTE_DEVICE;
	sta = NtOpenFile(&hFile, GENERIC_WRITE|GENERIC_READ|SYNCHRONIZE, &atFlash, &ioFlash, OPEN_EXISTING, FILE_SYNCHRONOUS_IO_NONALERT);
	if(sta != 0)
	{
		DbgLog::GetInstance().log("NtOpenFile(flash): %08x (err: %08x)\n", sta, GetLastError());
		return 0;
	}

	dwPos = SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
	if(dwPos != INVALID_SET_FILE_POINTER)
	{
		ZeroMemory(buf, len);
		while(ttlRead < len)
		{
			if((len-ttlRead)< readSize)
				rdsz = len-ttlRead;
			ReadFile(hFile, &buf[ttlRead], rdsz, &bRead, NULL);
			ttlRead += bRead;
			if(pbRead != NULL)
			{
				if(workerState == WSTM_IDLE) // ABORT!
				{
					NtClose(hFile);
					return ttlRead;
				}
				*pbRead = ttlRead;
				doLightSync(pbRead);
			}
		}
	}
	else
		DbgLog::GetInstance().log("SetFilePointer(flash) invalid! %08x\n", sta, GetLastError());
	NtClose(hFile);

	return ttlRead;
}

// note while writing flash in this manner that write address must be block aligned
// writes must start at the beginning of a block
// and writes must occur in 1block chunks (or less, but you cannot write multiple times to the same block)
// system takes care of spare (lba/edc) and relocation if necessary
// offset addresses are without spare included
DWORD Nand::WriteFlash(DWORD offset, PBYTE buf, DWORD len, DWORD writeSize, PDWORD bAvail)
{
	HANDLE hFile;
	OBJECT_ATTRIBUTES atFlash;
	IO_STATUS_BLOCK ioFlash;
	DWORD bWrote = 0, tWrote = 0;
	DWORD szWrite = writeSize;
	DWORD sta;
	LARGE_INTEGER lOffset;
	STRING nFlash;
	RtlInitAnsiString(&nFlash, DEVICE_FLASH);
	atFlash.RootDirectory = 0;
	atFlash.ObjectName = &nFlash;
	atFlash.Attributes = FILE_ATTRIBUTE_DEVICE;
	sta = NtOpenFile(&hFile, GENERIC_WRITE|GENERIC_READ|SYNCHRONIZE, &atFlash, &ioFlash, OPEN_EXISTING, FILE_SYNCHRONOUS_IO_NONALERT);
	if(sta != 0)
	{
		DbgLog::GetInstance().log("NtOpenFile(flash): %08x (err: %08x)\n", sta, GetLastError());
		return 0;
	}

	lOffset.QuadPart = (LONGLONG)offset;
	while(tWrote < len)
	{
		if((len-tWrote) < writeSize)
			szWrite = len-tWrote;
		if(bAvail != NULL)
		{
			if(workerState == WSTM_IDLE) // ABORT!
			{
				NtClose(hFile);
				return tWrote;
			}
			while(*bAvail < (tWrote+szWrite))
				Sleep(10);
		}
		sta = NtWriteFile(hFile, 0, 0, 0, &ioFlash, &buf[tWrote], szWrite, &lOffset);
		if(sta >= 0)
			bWrote = szWrite;
		lOffset.QuadPart += bWrote;
		tWrote += bWrote;

	}
	NtClose(hFile);
	return tWrote;
}

BOOL Nand::enumerateNand(void)
{
	pagesPerBlock = 32;
	if(IS_MMC_DEVICE) // corona MMC
	{
		w_writeSize = 0x4000;
		nandInfo.blockSize = 0x20000;
		nandInfo.dumpSize = 0x3000000;
		dataSz = 0x3000000;
		spareSz = 0;
		w_updateSize = 0x8000; // two blocks
		w_blockOff = 0x90000; // start of the two blocks
		w_patchOff = 0x1000; // start of patches in the two blocks
#ifdef FULL_DEBUG_OUT
		DbgLog::GetInstance().log("MMC console detected\n");
#endif

	}
	else
	{
		configSave = readreg(SFCX_CONFIG);

		// defaults for 16/64M small block
		w_updateSize = 0x8000; // two blocks
		w_writeSize = 0x4000;
		w_blockOff = 0x90000; // start of the two blocks
		w_patchOff = 0x1000; // start of patches in the two blocks
		nandInfo.blockSize = 0x4200;
		isBigBlock = FALSE;
		isBigBlockCont = FALSE;

#ifdef FULL_DEBUG_OUT
		DbgLog::GetInstance().log("SFC config %08x ver: %d type: %d\n", configSave, ((configSave>>17)&3), ((configSave >> 4) & 0x3));
#endif

		switch((configSave>>17)&3) // 00043000
		{
			case 0: // small block flash controller
				switch ((configSave >> 4) & 0x3)
				{
// 					 case 0: // 8M, not really supported
// 						nandInfo.dumpSize = 0x840000;
// 						dataSz = 0x800000;
// 						spareSz = 0x40000;
// 						break;
					case 1: // 16MB
						nandInfo.dumpSize = 0x1080000;
						dataSz = 0x1000000;
						spareSz = 0x80000;
						break;
// 					 case 2: // 32M, not really supported
// 						nandInfo.dumpSize = 0x2100000;
// 						dataSz = 0x2000000;
// 						spareSz = 0x100000;
// 						break;
					case 3: // 64MB
						nandInfo.dumpSize = 0x4200000;
						dataSz = 0x4000000;
						spareSz = 0x200000;
						break;
					default:
						DbgLog::GetInstance().log("unknown T%s NAND size! (%x)\n", ((configSave >> 4) & 0x3), (configSave >> 4) & 0x3);
						return FALSE;
				}
				break;
			case 1: // big block flash controller
				switch ((configSave >> 4) & 0x3)// TODO: FIND OUT FOR 64M!!! IF THERE IS ONE!!!
				{
					case 1: // Small block 16MB setup
						isBigBlockCont = TRUE;
						nandInfo.dumpSize = 0x1080000;
						dataSz = 0x1000000;
						spareSz = 0x80000;
						break;
					case 2: // Large Block: Current Jasper 256MB and 512MB
						isBigBlockCont = TRUE;
						isBigBlock = TRUE;
						nandInfo.dumpSize = 0x4200000;
						nandInfo.blockSize = 0x21000;
						dataSz = 0x4000000;
						spareSz = 0x200000;
						pagesPerBlock = 256;
						w_updateSize = 0x20000; // one block 128KB
						w_writeSize = 0x20000;
						w_blockOff = 0x80000; // start of the block
						w_patchOff = 0x11000; // start of patches in the block
						break;
					default:
						DbgLog::GetInstance().log("unknown T%s NAND size! (%x)\n", ((configSave >> 4) & 0x3), (configSave >> 4) & 0x3);
						return FALSE;
				}
				break;
			case 2: // MMC capable big block flash controller ie: 16M corona 000431c4
				switch ((configSave >> 4) & 0x3) 
				{
					case 0: // 16M
						isBigBlockCont = TRUE;
						nandInfo.dumpSize = 0x1080000;
						dataSz = 0x1000000;
						spareSz = 0x80000;
						break;
					case 1: // 64M
						isBigBlockCont = TRUE;
						nandInfo.dumpSize = 0x4200000;
						dataSz = 0x4000000;
						spareSz = 0x200000;
						break;
					case 2: // Big Block
						isBigBlockCont = TRUE;
						isBigBlock = TRUE;
						nandInfo.dumpSize = 0x4200000;
						nandInfo.blockSize = 0x21000;
						dataSz = 0x4000000;
						spareSz = 0x200000;
						pagesPerBlock = 256;
						w_updateSize = 0x20000; // one block 128KB
						w_writeSize = 0x20000;
						w_blockOff = 0x80000; // start of the block
						w_patchOff = 0x11000; // start of patches in the block
						break;
					//case 3: // big block, but with blocks twice the size of known big blocks above...
					//	break;
					default:
						DbgLog::GetInstance().log("unknown T%s NAND size! (%x)\n", ((configSave >> 4) & 0x3), (configSave >> 4) & 0x3);
						return FALSE;
				}
				break;
			default:
				DbgLog::GetInstance().log("unknown NAND type! (%x)\n", (configSave>>17)&3);
				return FALSE;
		}
	}

#ifdef FULL_DEBUG_OUT
	DbgLog::GetInstance().log("isBigBlockCont : %s\n", isBigBlockCont == TRUE ? "TRUE":"FALSE");
	DbgLog::GetInstance().log("isBigBlock     : %s\n", isBigBlock == TRUE ? "TRUE":"FALSE");
	DbgLog::GetInstance().log("dumpSize       : 0x%x\n", nandInfo.dumpSize);
	DbgLog::GetInstance().log("blockSize      : 0x%x\n", nandInfo.blockSize);
	DbgLog::GetInstance().log("dataSz         : 0x%x\n", dataSz);
	DbgLog::GetInstance().log("spareSz        : 0x%x\n", spareSz);
	DbgLog::GetInstance().log("pagesPerBlock  : 0x%x\n", pagesPerBlock);
	DbgLog::GetInstance().log("w_updateSize   : 0x%x\n", w_updateSize);
	DbgLog::GetInstance().log("w_writeSize    : 0x%x\n", w_writeSize);
	DbgLog::GetInstance().log("w_blockOff     : 0x%x\n", w_blockOff);
	DbgLog::GetInstance().log("w_patchOff     : 0x%x\n", w_patchOff);
#endif
	return TRUE;
}

// returns 1 on bad block, 2 on unrecoverable ECC
// data NULL to skip keeping data
DWORD Nand::readBlock(PBYTE data, DWORD block)
{
	BYTE* nbCur = data;
	DWORD sta, bRead;
	DWORD curAddr = block*w_writeSize;
	if(data != NULL)
		ZeroMemory(data, pagesPerBlock*0x210);
	for(bRead = 0; bRead < w_writeSize; bRead += 0x2000, curAddr += 0x2000)
	{
// 		DbgLog::GetInstance().log("Reading DMA at %x\n", curAddr);
		writereg(SFCX_STATUS, readreg(SFCX_STATUS));
		writereg(SFCX_DATAPHYADDR, phyAddr);
		writereg(SFCX_SPAREPHYADDR, phyAddr+0xC000);
		writereg(SFCX_ADDRESS, curAddr);
		writereg(SFCX_COMMAND, DMA_PHY_TO_RAM);
		while(sta = readreg(SFCX_STATUS) & STATUS_BUSY);
		sta = readreg(SFCX_STATUS);
		if(sta&STATUS_ERROR)
		{
			DbgLog::GetInstance().log("error in status, %08x\n", sta);
			if(sta&STATUS_BB_ER)
			{
				DbgLog::GetInstance().log("Bad block error block 0x%x\n", block);
				return 1;
			}
			if(STSCHK_ECC_ERR(sta))
			{
				DbgLog::GetInstance().log("unrecoverable ECC error block 0x%x\n", block);
				return 2;
			}
		}
		if(data != NULL)
		{
			for(DWORD i = 0; i < 16; i++)
			{
				XMemCpy(nbCur, &phyBuf[i*0x200], 0x200);
				XMemCpy(&nbCur[0x200], &phyBuf[0xC000+(i*0x10)], 0x10);
				nbCur += 0x210;
			}
		}
	}
	return 0;
}

DWORD Nand::eraseBlock(DWORD block)
{
	DWORD sta;
	DWORD curAddr = block*w_writeSize;
	writereg(SFCX_STATUS, readreg(SFCX_STATUS));
	writereg(SFCX_COMMAND, UNLOCK_CMD_1);
	writereg(SFCX_COMMAND, UNLOCK_CMD_0);
	writereg(SFCX_ADDRESS, curAddr);
	writereg(SFCX_COMMAND, BLOCK_ERASE);
	while(sta = readreg(SFCX_STATUS) & STATUS_BUSY);
	sta = readreg(SFCX_STATUS);
	return sta;
}

// data NULL to erase and not write
DWORD Nand::writeBlock(PBYTE data, DWORD block)
{
	BYTE* nbCur = data;
	DWORD sta, i, bWrote;
	DWORD curAddr = block*w_writeSize;
	// one erase per block
	sta = eraseBlock(block);
	if(sta&STATUS_ERROR)
		DbgLog::GetInstance().log("error in erase status, %08x\n", sta);
	if(data != NULL)
	{
		for(bWrote = 0; bWrote < w_writeSize; bWrote += 0x2000, curAddr += 0x2000)
		{
			for(i = 0; i < 16; i++)
			{
				XMemCpy(&phyBuf[i*0x200], nbCur, 0x200);
				XMemCpy(&phyBuf[0xC000+(i*0x10)], &nbCur[0x200], 0x10);
				nbCur += 0x210;
			}
			writereg(SFCX_STATUS, readreg(SFCX_STATUS));
			writereg(SFCX_COMMAND, UNLOCK_CMD_0);
			writereg(SFCX_COMMAND, UNLOCK_CMD_1);
			writereg(SFCX_DATAPHYADDR, phyAddr);
			writereg(SFCX_SPAREPHYADDR, phyAddr+0xC000);
			writereg(SFCX_ADDRESS, curAddr);
			writereg(SFCX_COMMAND, DMA_RAM_TO_PHY);
			while(sta = readreg(SFCX_STATUS) & STATUS_BUSY);
			sta = readreg(SFCX_STATUS);
			if(sta&STATUS_ERROR)
				DbgLog::GetInstance().log("error in status, %08x\n", sta);
		}
	}
	return 0;
}

PBYTE Nand::readBlocks(DWORD block, int numBlocks, DWORD* osz)
{
	DWORD sz = (numBlocks*nandInfo.blockSize);
	PBYTE buf = (PBYTE)VirtualAlloc(0, (numBlocks*nandInfo.blockSize), MEM_COMMIT|MEM_LARGE_PAGES, PAGE_READWRITE);
	if(buf)
	{
		if(osz)
			*osz = sz;
// 		DbgLog::GetInstance().log("reading %d blocks starting at %x block, %x sz\n", numBlocks, block, sz);
		if(IS_MMC_DEVICE)
		{
			DWORD rd = ReadFlash(block*nandInfo.blockSize, buf, sz, nandInfo.blockSize, NULL);
			if(rd != sz)
			{
				DbgLog::GetInstance().log("trying to read mmc yielded 0x%x bytes instead of 0x%x!\n", rd, sz);
				VirtualFree(buf,0, MEM_RELEASE );
				return NULL;
			}
		}
		else
		{
			int blk;
			DWORD cfg = readreg(SFCX_CONFIG);
			DWORD tcfg = (cfg&~(0x3C0|CONFIG_INT_EN|CONFIG_WP_EN));
			if(isBigBlock)
				tcfg = tcfg|((4-1)<<6); // change to 4 pages, bb 4 pages = 16 small pages
			else
				tcfg = tcfg|((16-1)<<6); // change to 16 pages
			writereg(SFCX_CONFIG, tcfg);
			for(blk = 0; blk < numBlocks; blk++)
			{
// 				DbgLog::GetInstance().log("Reading block %x of %x at block %x (%x)\n", blk, numBlocks, blk+block, (blk+block)*nandInfo.blockSize);
				readBlock(&buf[blk*nandInfo.blockSize], blk+block);
			}
			writereg(SFCX_CONFIG, cfg);
		}
// 		DbgLog::GetInstance().log("flash read complete\n");
	}
	else
		DbgLog::GetInstance().log("could not allocate %x bytes for readblocks, error %d\n", (numBlocks*nandInfo.blockSize), GetLastError());
	return buf;
}

PBYTE Nand::getBootloaders(DWORD* osz)
{
	PBYTE buf;
	DWORD sz = fhdr->blHeader.Size+(w_writeSize-1);
	sz = sz &~ (w_writeSize-1);
	buf = (PBYTE)VirtualAlloc(0, sz, MEM_COMMIT|MEM_LARGE_PAGES, PAGE_READWRITE);
	if(buf)
	{
		memset(buf, 0, sz);
		if(osz)
			*osz = fhdr->blHeader.Size;
		DWORD rd = ReadFlash(0, buf, sz, w_writeSize, NULL);
		if(rd != sz)
		{
			DbgLog::GetInstance().log("trying to getBootloaders yielded 0x%x bytes instead of 0x%x!\n", rd, sz);
			VirtualFree(buf,0, MEM_RELEASE );
			return NULL;
		}
	}
	else
		DbgLog::GetInstance().log("could not allocate %x bytes for getBootloaders, error %d\n", sz, GetLastError());
	return buf;
}

BOOL Nand::BlockHasData(PBYTE data)
{
	for(DWORD i = 0; i < nandInfo.blockSize; i++)
		if((data[i]&0xFF) != 0xFF)
			return TRUE;
	return FALSE;
}

BOOL Nand::writeBlocks(PBYTE buf, DWORD block, int numBlocks)
{
	DWORD sz = (numBlocks*nandInfo.blockSize);
	if(((block+numBlocks)*nandInfo.blockSize)>nandInfo.dumpSize)
	{
		DbgLog::GetInstance().log("error, write exceeds system area!\n");
		return FALSE;
	}
// 	DbgLog::GetInstance().log("writing flash\n");
	if(IS_MMC_DEVICE)
	{
		DWORD wr;
		wr = WriteFlash(block*nandInfo.blockSize, buf, sz, w_writeSize, NULL);
		if(wr != sz)
		{
			DbgLog::GetInstance().log("trying to write mmc yielded 0x%x bytes instead of 0x%x!\n", wr, sz);
			return FALSE;
		}
	}
	else
	{
		DWORD blk;
		DWORD cfg = readreg(SFCX_CONFIG);
		DWORD tcfg = (cfg&~(0x3C0|CONFIG_INT_EN))|CONFIG_WP_EN; // for the write
		if(isBigBlock)
			tcfg = tcfg|((4-1)<<6); // change to 4 pages, bb 4 pages = 16 small pages
		else
			tcfg = tcfg|((16-1)<<6); // change to 16 pages
		writereg(SFCX_CONFIG, (tcfg));
		for(blk = 0; blk < (DWORD)numBlocks; blk++)
		{
			PBYTE pBlkDat = &buf[blk*nandInfo.blockSize];
			// check for bad block, do NOT modify bad blocks!!!
			if(readBlock(g_blockBuf, blk+block) == 0)
			{
				// check if data needs to be written
				if(memcmp(g_blockBuf, pBlkDat, nandInfo.blockSize) != 0)
				{
					// check if block has data to write, or if it's only an erase
					if(BlockHasData(pBlkDat))
					{
						//DbgLog::GetInstance().log("Writing block %x of %x at %x (%x)\n", blk, numBlocks, blk+block, (blk+block)*nandInfo.blockSize);
						writeBlock(pBlkDat, blk+block);
					}
					else
					{
						//DbgLog::GetInstance().log("Erase only block %x of %x at %x (%x)\n", blk, numBlocks, blk+block, (blk+block)*nandInfo.blockSize);
						writeBlock(NULL, blk+block);
					}
				}
				//else
				//	DbgLog::GetInstance().log("skipping write block %x at %x of %x, data identical\n", blk, blk*w_writeSize, dataSz/w_writeSize);
			}
			else
				DbgLog::GetInstance().log("not writing to block %x, it is bad!\n", blk+block);
		}
		writereg(SFCX_CONFIG, cfg);
	}
// 	DbgLog::GetInstance().log("flash write complete\n");
	return TRUE;
}

BOOL Nand::eraseBlocks(DWORD block, int numBlocks)
{
	DWORD sz = (numBlocks*nandInfo.blockSize);
	if(IS_MMC_DEVICE)
	{
		int i;
		memset(g_blockBuf, 0xFF, nandInfo.blockSize);
		for(i = 0; i < numBlocks; i++)
		{
			DWORD wr = WriteFlash((block+i)*nandInfo.blockSize, g_blockBuf, nandInfo.blockSize, w_writeSize, NULL);
			if(wr != sz)
			{
				DbgLog::GetInstance().log("trying to fake-erase mmc yielded 0x%x bytes instead of 0x%x!\n", wr, sz);
				return FALSE;
			}
		}
	}
	else
	{
		DWORD blk;
		DWORD cfg = readreg(SFCX_CONFIG);
		DWORD tcfg = (cfg&~(0x3C0|CONFIG_INT_EN))|CONFIG_WP_EN; // for the write
		if(isBigBlock)
			tcfg = tcfg|((4-1)<<6); // change to 4 pages, bb 4 pages = 16 small pages
		else
			tcfg = tcfg|((16-1)<<6); // change to 16 pages
		writereg(SFCX_CONFIG, (tcfg));
		for(blk = 0; blk < (DWORD)numBlocks; blk++)
		{
			DWORD sta = eraseBlock(blk+block);
			if(sta&STATUS_ERROR)
				DbgLog::GetInstance().log("error in erase status, %08x\n", sta);
		}
		writereg(SFCX_CONFIG, cfg);
		setWorkerBbList();
		waitWorkerBusy();
	}
	return TRUE;
}

BOOL Nand::WriteFullFlash(void)
{
// 	DbgLog::GetInstance().log("writing flash\n");
	if(IS_MMC_DEVICE)
	{
		DWORD wr;
		wr = WriteFlash(0, nandData, nandInfo.dumpSize, w_writeSize, &workerSize);
		if(wr != nandInfo.dumpSize)
		{
			DbgLog::GetInstance().log("trying to write mmc yielded 0x%x bytes instead of 0x%x!\n", wr, nandInfo.dumpSize);
			return FALSE;
		}
	}
	else
	{
		DWORD blk;
		DWORD cfg = readreg(SFCX_CONFIG);
		DWORD tcfg = (cfg&~(0x3C0|CONFIG_INT_EN))|CONFIG_WP_EN; // for the write
		if(isBigBlock)
			tcfg = tcfg|((4-1)<<6); // change to 4 pages, bb 4 pages = 16 small pages
		else
			tcfg = tcfg|((16-1)<<6); // change to 16 pages
		writereg(SFCX_CONFIG, (tcfg));
		for(blk = 0; blk < (dataSz/w_writeSize); blk++)
		{
			PBYTE pBlkDat = &nandData[blk*nandInfo.blockSize];
			while(workerSize < ((blk+1)*(nandInfo.blockSize)))
			{
				if(workerState == WSTM_IDLE) // ABORT!
				{
					writereg(SFCX_CONFIG, cfg);
					return FALSE;
				}
				Sleep(10);
			}
			// check for bad block, do NOT modify bad blocks!!!
			if(readBlock(g_blockBuf, blk) == 0)
			{
				// check if data needs to be written
				if(memcmp(g_blockBuf, pBlkDat, nandInfo.blockSize) != 0)
				{
					// check if block has data to write, or if it's only an erase
					if(BlockHasData(pBlkDat))
					{
						//DbgLog::GetInstance().log("Writing block %x at %x of %x\n", blk, blk*w_writeSize, dataSz/w_writeSize);
						writeBlock(pBlkDat, blk);
					}
					else
					{
						if(isBigBlock)
						{
							PPAGEDATA ppd = (PPAGEDATA)g_blockBuf;
							if((ppd[0].meta.bg.FsBlockType >= 0x2a)||(ppd[0].meta.bg.FsBlockType == 0))
								writeBlock(NULL, blk);
// 							else
// 								DbgLog::GetInstance().log("block 0x%x contains nandmu data! type: %x\n", blk, ppd[0].meta.bg.FsBlockType);
							//if((ppd[0].meta.bg.FsBlockType < 0x2a)&&(ppd[0].meta.bg.FsBlockType != 0))
							//	DbgLog::GetInstance().log("block 0x%x contains nandmu data! type: %x\n", blk, ppd[0].meta.bg.FsBlockType);
							//else
							//	writeBlock(NULL, blk);
						}
						else
						{
							//DbgLog::GetInstance().log("Erase only block %x at %x of %x\n", blk, blk*w_writeSize, dataSz/w_writeSize);
							writeBlock(NULL, blk);
						}
					}
				}
				//else
				//	DbgLog::GetInstance().log("skipping write block %x at %x of %x, data identical\n", blk, blk*w_writeSize, dataSz/w_writeSize);
			}
			else
				DbgLog::GetInstance().log("not writing to block %x, it is bad!\n", blk);
		}
		writereg(SFCX_CONFIG, cfg);
	}
// 	DbgLog::GetInstance().log("flash write complete\n");
	return TRUE;
}

BOOL Nand::ReadFullFlash(void)
{
// 	DbgLog::GetInstance().log("reading flash\n");
	if(IS_MMC_DEVICE)
	{
		DWORD rd = ReadFlash(0, nandData, nandInfo.dumpSize, 0x20000, &workerSize);
		if(rd != nandInfo.dumpSize)
		{
			DbgLog::GetInstance().log("trying to read mmc yielded 0x%x bytes instead of 0x%x!\n", rd, nandInfo.dumpSize);
			return FALSE;
		}
	}
	else
	{
		DWORD blk;
		DWORD cfg = readreg(SFCX_CONFIG);
		DWORD tcfg = (cfg&~(0x3C0|CONFIG_INT_EN|CONFIG_WP_EN));
		if(isBigBlock)
			tcfg = tcfg|((4-1)<<6); // change to 4 pages, bb 4 pages = 16 small pages
		else
			tcfg = tcfg|((16-1)<<6); // change to 16 pages
		writereg(SFCX_CONFIG, tcfg);
		for(blk = 0; blk < (dataSz/w_writeSize); blk++)
		{
			if(workerState == WSTM_IDLE) // ABORT!
			{
				writereg(SFCX_CONFIG, cfg);
				return FALSE;
			}
			//DbgLog::GetInstance().log("Reading block %x at %x of %x\n", blk, blk*w_writeSize, dataSz/w_writeSize);
			readBlock(&nandData[blk*nandInfo.blockSize], blk);
			workerSize = (blk+1)*nandInfo.blockSize;
			doLightSync(&workerSize);
		}
		writereg(SFCX_CONFIG, cfg);
	}
// 	DbgLog::GetInstance().log("flash read complete\n");
	return TRUE;
}

DWORD Nand::getCBBVer(DWORD off)
{
	BLDR_HEADER cbHdr;
	if(ReadFlash(off, (PBYTE)&cbHdr, sizeof(BLDR_HEADER), sizeof(BLDR_HEADER), NULL) != sizeof(BLDR_HEADER))
	{
		DbgLog::GetInstance().log("Unable to read CBB header!!\n");
		return 0;
	}
	return cbHdr.Build;
}

BOOL Nand::enumImageType(void)
{
	BLDR_HEADER cbHdr;
	w_IsJtag = FALSE;
	w_IsGlitch2 = FALSE;
	w_IsGlitch2m = FALSE;
	w_ConsoleType = 0;
	nandInfo.useFlags = 0;
	if(fhdr == NULL)
	{
		DbgLog::GetInstance().log("Flash header is NULL!!\n");
		return FALSE;
	}
	ZeroMemory(&cbHdr, sizeof(BLDR_HEADER));

	// read first CB header
	if(ReadFlash(fhdr->blHeader.Entry, (PBYTE)&cbHdr, sizeof(BLDR_HEADER), sizeof(BLDR_HEADER), NULL) != sizeof(BLDR_HEADER))
	{
		DbgLog::GetInstance().log("Unable to read CB header!!\n");
		return FALSE;
	}
#ifdef FULL_DEBUG_OUT
	DbgLog::GetInstance().log("Read CB header OK, V %d\n", cbHdr.Build);
#endif

	switch(cbHdr.Build)
	{
		case 1921:
		case 4558:
		case 5770:
		case 6723:
			nandInfo.useFlags = PATCH_MASK_JTAG;
			w_IsJtag = TRUE;
			break;
		case 4577:
		case 5772:
		case 6752:
		case 13121:
			nandInfo.useFlags = PATCH_MASK_GLITCH2;
			w_IsGlitch2 = TRUE;
			if(getCBBVer(fhdr->blHeader.Entry+((cbHdr.Size+0xF)&~0xF)) == 13182)
			{
				nandInfo.optFlag |= OFLAG_KEEP_BLS;
			}
			break;
		case 10375: // glitchish image on devkit trinity
		case 14352: // glitchish image on devkit corona
			w_IsGlitch2 = TRUE;
			w_IsGlitch2m = TRUE;
			nandInfo.useFlags = PATCH_MASK_GLITCH2MFG;
			break;
		case 9188:
			w_IsGlitch2 = TRUE;
			if(cbHdr.Flags & 1) // need to read cbb header to tell if it's trinity or corona...
			{
				DWORD off = fhdr->blHeader.Entry+((cbHdr.Size+0xF)&~0xF);
				nandInfo.useFlags = PATCH_MASK_GLITCH2MFG;
				w_IsGlitch2m = TRUE;
				if(ReadFlash(off, (PBYTE)&cbHdr, sizeof(BLDR_HEADER), sizeof(BLDR_HEADER), NULL) != sizeof(BLDR_HEADER))
				{
					DbgLog::GetInstance().log("Unable to read CBB header!!\n");
					return FALSE;
				}
#ifdef FULL_DEBUG_OUT
				DbgLog::GetInstance().log("Read CBB header OK, V %d\n", cbHdr.Build);
#endif
			}
			else
				nandInfo.useFlags = PATCH_MASK_GLITCH|PATCH_MASK_GLITCH2;
			break;
		default:
#ifdef FULL_DEBUG_OUT
			DbgLog::GetInstance().log("CB Type not JTAG or Glitch2, assuming Glitch1!\n");
#endif
			break;
	}
	if(((cbHdr.Build > 1000)&&(cbHdr.Build < 2000))||((cbHdr.Build > 7000)&&(cbHdr.Build < 8000)))
		w_ConsoleType = BL_TYPE_XENON;
	else if((cbHdr.Build > 4000)&&(cbHdr.Build < 5000))
		w_ConsoleType = BL_TYPE_ZEPHYR;
	else if((cbHdr.Build > 5000)&&(cbHdr.Build < 6000))
		w_ConsoleType = BL_TYPE_FALCON;
	else if((cbHdr.Build > 6000)&&(cbHdr.Build < 7000))
		w_ConsoleType = BL_TYPE_JASPER;
	else if((cbHdr.Build > 9000)&&(cbHdr.Build < 11000))
		w_ConsoleType = BL_TYPE_TRINITY;
	else if((cbHdr.Build > 13000)&&(cbHdr.Build < 15000))
		w_ConsoleType = BL_TYPE_CORONA;
	else if((cbHdr.Build > 16000)&&(cbHdr.Build < 17000))
		w_ConsoleType = BL_TYPE_WINCHESTER;
	// build the type mask to locate the patches in embedded
	if(w_IsJtag||w_IsGlitch2||w_IsGlitch2m)
		nandInfo.useFlags |= w_ConsoleType;
	else // all that is left is fat glitch
		nandInfo.useFlags = PATCH_MASK_GLITCH|w_ConsoleType;
	if(w_IsJtag == FALSE)
	{
		w_blockOff = fhdr->dwSysUpdateAddr;
		if(fhdr->dwPatchSlotSize == 0)
			w_blockOff += 0x10000;
		else
			w_blockOff += fhdr->dwPatchSlotSize;
		w_patchOff = 0;
		if(w_updateSize == 0x20000) 
		{
			if((w_blockOff%0x20000) != 0)// some xebuild images had big block with patch slot in the middle of a block
			{
				DWORD origOff = w_blockOff;
				w_blockOff = w_blockOff&~(0x20000-1); // align block read to the start of the block
				w_patchOff = origOff - w_blockOff;// start of patches in the block
			}
		}
		if(w_IsGlitch2m)
			w_patchOff += 0x60; // to keep the virtual fuses intact
	}
	DbgLog::GetInstance().log("CB%04d found, type: %s JTAG: %s GLITCH2: %s mfg: %s\n", cbHdr.Build, consoleModel[w_ConsoleType], w_IsJtag ? "yes":"no", w_IsGlitch2 ? "yes":"no", w_IsGlitch2m ? "yes":"no");
	if(updatePatchData())
	{
#ifdef FULL_DEBUG_OUT
		DbgLog::GetInstance().log("Patch info obtained OK\n");
#endif
		return TRUE;
	}
#ifdef FULL_DEBUG_OUT
	DbgLog::GetInstance().log("FAILED to obtain patch info\n");
#endif
	return FALSE;
}

BOOL Nand::updatePatchData(void)
{
	w_PatchInfo = FALSE;
	w_currPatchSize = 0;
	if(w_currPatchData == NULL)
		w_currPatchData = (BYTE*)VirtualAlloc(NULL, MAX_PATCH_SIZE, MEM_COMMIT, PAGE_READWRITE);
	if(w_currPatchData != NULL)
	{
		if(ReadFlash((w_blockOff+w_patchOff), w_currPatchData, MAX_PATCH_SIZE, MAX_PATCH_SIZE, NULL) == MAX_PATCH_SIZE)
		{
			w_currPatchSize = MAX_PATCH_SIZE;
			GetExtras();
			w_PatchInfo = TRUE;
			return TRUE;
		}
	}
#ifdef FULL_DEBUG_OUT
	else
		DbgLog::GetInstance().log("failed to allocate 0x4000 bytes to store patches!\n");
#endif
	return FALSE;
}

BOOL Nand::getPatchData(void* bout, int blen)
{
	if(w_PatchInfo)
	{
		if((bout != NULL)&&(blen != 0))
		{
			if(blen <= (int)w_currPatchSize)
				memcpy(bout, w_currPatchData, blen);
			else
				memcpy(bout, w_currPatchData, w_currPatchSize);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL Nand::setPatchData(void* bin, int blen)
{
	BOOL ret = FALSE;
	if((w_PatchInfo)&&(bin != NULL)&&(blen != 0))
	{
		PBYTE blockBuf = (BYTE*)VirtualAlloc(NULL, w_updateSize, MEM_COMMIT, PAGE_READWRITE);
		if(blockBuf != NULL)
		{
			ReadFlash(w_blockOff, blockBuf, w_updateSize, w_updateSize, NULL);
			memset(&blockBuf[w_patchOff], 0xFF, MAX_PATCH_SIZE);
			// inject the patches
			if((w_patchOff+blen+w_extraSize) < w_updateSize)
			{
				memcpy(&blockBuf[w_patchOff], bin, blen);
				if((w_extras!=NULL)&&(w_extraSize != 0))
				{
					DWORD off;
					off = w_patchOff+(blen-4);
#ifdef FULL_DEBUG_OUT
					DbgLog::GetInstance().log("adding extras 0x%x @ 0x%x\n", w_extraSize, off);
#endif
					memcpy(&blockBuf[off], w_extras, w_extraSize); // overwrites the last 0xFFFFFFFF
					off += w_extraSize;
					memset(&blockBuf[off], 0xFF, 4); // put one back, just in case
					off += 4;
					memcpy(&blockBuf[off], &w_extraSize, 4); // write in the extra size again
				}
				//write 'em back to flash
				WriteFlash(w_blockOff, blockBuf, w_updateSize, w_writeSize, NULL);
				updatePatchData();
				ret = TRUE;
			}
// 			else
// 				DbgLog::GetInstance().log("size 0x%x is bigger than 0x%x\n", w_updateSize, (w_patchOff+blen));

			VirtualFree(blockBuf, 0, MEM_RELEASE);
		}
	}
	return ret;
}

DWORD Nand::getU32(PBYTE data)
{
	DWORD ret = (data[0]&0xFF)<<24;
	ret |= (data[1]&0xFF)<<16;
	ret |= (data[2]&0xFF)<<8;
	ret |= (data[3]&0xFF);
	return ret;
}

BOOL Nand::GetExtras(void)
{
	int i = 0x10, cnt = 1, offset=0;
	int sz = 0;
	DWORD extras = 0;
	if(w_IsJtag)
	{
		// 		DbgLog::GetInstance().log("scanning for jtag\n");
		i = 0;
		cnt = 4;
	}
	sz = (MAX_PATCH_SIZE-(cnt*4))-i;
	// find the tail of oldp and check for any custom patches
	for(; i < sz; i+=4)
	{
		//DbgLog::GetInstance().log("%08x\n", getU32(&oldp[i]));
		if(getU32(&w_currPatchData[i]) == 0xFFFFFFFF)
		{
			// 			DbgLog::GetInstance().log("0x%08x at 0x%x\n",getU32(&w_currPatchData[i]), i);
			cnt--;
			if(cnt == 0)
			{
				offset = i;
				i+=4;
				extras = getU32(&w_currPatchData[i]);
				if(extras >= (DWORD)offset)
					extras = 0xFFFFFFFF;
				i = sz;
				//DbgLog::GetInstance().log("i: %08x i+1: %08x i+2: %x extras: %08x offset: %08x\n", getU32(&w_currPatchData[i]), getU32(&w_currPatchData[i+4]), getU32(&w_currPatchData[i+8]), extras, offset);
			}
		}
	}
	if((extras == 0xFFFFFFFF)||(extras == 0))
	{
		w_currPatchSize = offset;
		return FALSE;
	}
	if((extras%4) != 0)
	{
		w_currPatchSize = offset;
		return FALSE;
	}
	if(extras < (DWORD)offset)
	{
		nandInfo.optFlag = nandInfo.optFlag&~OFLAG_HAS_ADDONS;
		if(w_extras != NULL)
			VirtualFree(w_extras, 0, MEM_RELEASE);
		w_extraSize = 0;
#ifdef FULL_DEBUG_OUT
		DbgLog::GetInstance().log("0x%x bytes extra patches found in flash\n", extras);
		DbgLog::GetInstance().log("offset-extras: %08x\n", offset-extras);
#endif
		w_extras = (BYTE*)VirtualAlloc(NULL, extras, MEM_COMMIT, PAGE_READWRITE);
		if(w_extras != NULL)
		{
			memcpy(w_extras, &w_currPatchData[offset-extras], extras);
			memset(&w_currPatchData[offset-extras], 0xFF, 4);
			w_currPatchSize = offset-extras+4;
			w_extraSize = extras;
			nandInfo.optFlag |= OFLAG_HAS_ADDONS;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL Nand::readBlData(DWORD* currOff, char hchar, BYTE** pbuf, BYTE* headbuf)
{
	DWORD bread;
	if(headbuf != NULL)
	{
		PBLDR_HEADER pbl = (PBLDR_HEADER)headbuf;
		if((currOff[0]+0x30) >= dataSz)
		{
			DbgLog::GetInstance().log("error reading bl %c puts us outside of system area!\n", hchar);
			return FALSE;
		}
		bread = ReadFlash(currOff[0], headbuf, 0x30, 0x30, NULL);
		if(bread == 0x30)
		{
			if((pbl->Magic&0x43FF) != (0x4300|(hchar&0xFF)))
			{
				DbgLog::GetInstance().log("error bl header magic incorrect, got 0x%04x should be 0x%04x!\n", (pbl->Magic&0x43FF), (0x4300|(hchar&0xFF)));
				return FALSE;
			}
			currOff[0] += (pbl->Size+0xF)&~0xF;
		}
	}
	else
	{
		BYTE* buf;
		DWORD blsz;
		BLDR_HEADER blh;
		bread = ReadFlash(currOff[0], (PBYTE)&blh, 0x10, 0x10, NULL);
		if(bread != 0x10)
		{
			DbgLog::GetInstance().log("error reading bl %c header!\n", hchar);
			return FALSE;
		}
		if((blh.Magic&0x43FF) != (0x4300|(hchar&0xFF)))
		{
			DbgLog::GetInstance().log("error bl header magic incorrect, got 0x%04x should be 0x%04x!\n", (blh.Magic&0x43FF), (0x4300|(hchar&0xFF)));
			return FALSE;
		}
		blsz = (blh.Size+0xF)&~0xF;
		if((currOff[0]+blsz) >= dataSz)
		{
			DbgLog::GetInstance().log("error reading bl %c puts us outside of system area!\n", hchar);
			return FALSE;
		}
		buf = (BYTE*)VirtualAlloc(NULL, blsz, MEM_COMMIT, PAGE_READWRITE);
		if(buf == NULL)
		{
			DbgLog::GetInstance().log("error allocating buffer for BL data!\n");
			return FALSE;
		}
		memcpy(buf, &blh, 0x10);
		bread = ReadFlash(currOff[0]+0x10, &buf[0x10], blsz-0x10, blsz-0x10, NULL);
		if(bread != (blsz-0x10))
		{
			VirtualFree(buf, 0, MEM_RELEASE);
			DbgLog::GetInstance().log("error reading BL data! got: %x expected %x\n", bread, blsz-0x10);
			return FALSE;
		}
		*pbuf = buf;
		currOff[0] += blsz;
	}
	return TRUE;
}

BOOL Nand::getBlModData(DWORD* curroff)
{
	BYTE buf1[0x30];
	BYTE* buf2 = NULL;
	BYTE keybuf[0x10];
	BYTE rc4key[0x10];
	BYTE cpuKey[0x10];
	PBLDR_HEADER hbl;
	DWORD len;
	DbgLog::GetInstance().log("attempting to read BLMod data 0x%x in size\n", w_BlModSz);
	if(readBlData(curroff, 'B', NULL, buf1) == FALSE)
	{
		DbgLog::GetInstance().log("error reading CB/CBA data (mod)\n");
		return FALSE;
	}
	hbl = (PBLDR_HEADER)buf1;
	memcpy(nandInfo.CBANonce, &buf1[0x10], 0x10);
	// get key to decrypt CBB/CD
	memcpy(keybuf, nandInfo.Bl1Key, 0x10);
	XeCryptHmacSha(keybuf, 0x10, &buf1[0x10], 0x10, NULL, 0, NULL, 0, rc4key, 0x10);
// 	XeCryptRc4(rc4key, 0x10, &buf1[0x20], len);
	memcpy(cpuKey, nandInfo.CpuKey, 0x10);
	if(hbl->Flags&0x800) // has cbb
	{
		if(readBlData(curroff, 'B', &buf2, NULL) == FALSE)
		{
			DbgLog::GetInstance().log("**** Warning: error getting CBB data, aborting!\n");
			return FALSE;
		}
		memcpy(nandInfo.CBBNonce, &buf2[0x10], 0x10);
		// decrypt CBB and copy out the blmod data
		if(hbl->Flags&0x1)
			memset(cpuKey, 0, 0x10);
		if((hbl->Flags&0x1000) != 0)
		{
			unsigned char tdat[0x10];
			memcpy(tdat, buf1, 0x10);
			tdat[6] = 0;
			tdat[7] = 0;
			XeCryptHmacSha(rc4key, 0x10, &buf2[0x10], 0x10, cpuKey, 0x10 , tdat, 0x10, keybuf, 0x10);
		}
		else
			XeCryptHmacSha(rc4key, 0x10, &buf2[0x10], 0x10, cpuKey, 0x10 , NULL, 0, keybuf, 0x10);// cbaKey is the RC4 key used to crypt CBA, retkey is the key used to crypt CBB

		hbl = (PBLDR_HEADER)buf2;
		len = ((hbl->Size+0xF)&~0xF);
		XeCryptRc4(keybuf, 0x10, &buf2[0x20], len-0x20);
		memcpy(w_BlMod, &buf2[len-w_BlModSz], w_BlModSz);

		// still need to get the CD nonce
		if(readBlData(curroff, 'D', NULL, buf1) == FALSE)
		{
			VirtualFree(buf2, 0, MEM_RELEASE);
			DbgLog::GetInstance().log("**** Warning: error getting CD data, aborting!\n");
			return FALSE;
		}
		memcpy(nandInfo.CDNonce, &buf1[0x10], 0x10);
	}
	else
	{
		if(readBlData(curroff, 'D', &buf2, NULL) == FALSE)
		{
			VirtualFree(buf1, 0, MEM_RELEASE);
			DbgLog::GetInstance().log("**** Warning: error getting CD data, aborting!\n");
			return FALSE;
		}
		memcpy(nandInfo.CDNonce, &buf2[0x10], 0x10);
		// decrypt CD and copy the blmod data
		hbl = (PBLDR_HEADER)buf2;
		len = ((hbl->Size+0xF)&~0xF);
		XeCryptHmacSha(rc4key, 0x10, &buf2[0x10], 0x10, NULL, 0, NULL, 0, keybuf, 0x10);
		XeCryptRc4(keybuf, 0x10, &buf2[0x20], len-0x20);
		memcpy(w_BlMod, &buf2[len-w_BlModSz], w_BlModSz);
	}
	VirtualFree(buf2, 0, MEM_RELEASE);
	return TRUE;
}

BOOL Nand::getNonces(void)
{
	BYTE buf[0x30];
	PBLDR_HEADER hbl;
	DWORD curroff = 0;
	hbl = (PBLDR_HEADER)buf;
	w_BlModSz = 0;

	if(headerDat[0x4B] >= 1)
	{
		if((nandInfo.useFlags&(PATCH_MASK_GLITCH|PATCH_MASK_GLITCH2|PATCH_MASK_GLITCH2MFG)) != 0)
		{
			w_BlModSz = ((headerDat[0x49]&0xFF)<<8)|(headerDat[0x4A]&0xFF);
			if(w_BlModSz != 0)
			{
				w_BlMod = (BYTE*)VirtualAlloc(NULL, w_BlModSz, MEM_COMMIT, PAGE_READWRITE);
				if(w_BlMod == NULL)
				{
					DbgLog::GetInstance().log("error allocating BlMod buffer of size %x!\n", w_BlModSz);
					w_BlModSz = 0;
				}
				DbgLog::GetInstance().log("BLMod data 0x%x size 0x%x\n", w_BlMod, w_BlModSz);
			}
		}
	}

	// get first CB offset
	curroff = fhdr->blHeader.Entry;
	if(w_BlModSz != 0)
	{
		nandInfo.optFlag |= OFLAG_HAS_BLMOD;
		if(getBlModData(&curroff) == FALSE)
		{
			w_BlModSz = 0;
			VirtualFree(w_BlMod, 0, MEM_RELEASE);
			DbgLog::GetInstance().log("error getting BlMod data!\n");
			return FALSE;
		}
	}
	else
	{
		if(readBlData(&curroff, 'B', NULL, buf) == FALSE)
		{
			DbgLog::GetInstance().log("**** Warning: error getting CB/CBA data, aborting!\n");
			return FALSE;
		}
		memcpy(nandInfo.CBANonce, &buf[0x10], 0x10);
		if(hbl->Flags&0x800) // has cbb
		{
			if(readBlData(&curroff, 'B', NULL, buf) == FALSE)
			{
				DbgLog::GetInstance().log("**** Warning: error getting CBB data, aborting!\n");
				return FALSE;
			}
			memcpy(nandInfo.CBBNonce, &buf[0x10], 0x10);
		}

		// get CD nonce
		if(readBlData(&curroff, 'D', NULL, buf) == FALSE)
		{
			DbgLog::GetInstance().log("**** Warning: error getting CD data, aborting!\n");
			return FALSE;
		}
		memcpy(nandInfo.CDNonce, &buf[0x10], 0x10);
	}

	// get CE nonce
	if(readBlData(&curroff, 'E', NULL, buf) == FALSE)
	{
		DbgLog::GetInstance().log("**** Warning: error getting CE data, aborting!\n");
		return FALSE;
	}
	memcpy(nandInfo.CENonce, &buf[0x10], 0x10);

	// get CF nonce
	curroff = fhdr->dwSysUpdateAddr;
	if(readBlData(&curroff, 'F', NULL, buf) == FALSE)
	{
		DbgLog::GetInstance().log("**** Warning: error getting CF data, aborting!\n");
		return FALSE;
	}
	memcpy(nandInfo.CFNonce, &buf[0x20], 0x10);
	
	// get CG nonce
	if(readBlData(&curroff, 'G', NULL, buf) == FALSE)
	{
		DbgLog::GetInstance().log("**** Warning: error getting CG data, aborting!\n");
		return FALSE;
	}
	memcpy(nandInfo.CGNonce, &buf[0x10], 0x10);
	return TRUE;
}

// get 1bl key and RSA key
// grab fuses, vfuses and cpu key
BOOL Nand::readConsoleData(void)
{
	PBYTE buf = (PBYTE)XPhysicalAlloc(0x8000, MAXULONG_PTR, 0, MEM_LARGE_PAGES|PAGE_READWRITE|PAGE_NOCACHE);
	if(buf != NULL)
	{
		int i;
		DWORD tmp;
		PDWORD dwbuf = (PDWORD)buf;
		QWORD dest = 0x8000000000000000ULL | ((DWORD)MmGetPhysicalAddress(buf)&0xFFFFFFFF);
		QWORD src;
//#define HVPEEK_NONE		0 // can get nothing but kernel side things
//#define HVPEEK_BASIC	1 // can get CPU key and kernel side things
//#define HVPEEK_MEMCPY	2 // can get everything

		if(USE_PEEK_VER >= HVPEEK_MEMCPY)
		{
			// first get the 1bl
			src = 0x8000020000000000ULL; // 1BL address
			ZeroMemory(buf, 0x8000);
			HvxPeek(src, dest, 0x8000);
			tmp = *(DWORD*)(&buf[0xFC]);
			if(tmp < 0x8000)
			{
				if((*(DWORD*)(&buf[tmp]) == 0x20) && (*(DWORD*)(&buf[tmp+4]) == 0x00010001))
				{
					memcpy(nandInfo.BL1Rsa, &buf[tmp], 0x110);
					memset(&nandInfo.BL1Rsa[8], 0, 8);
					memcpy(nandInfo.Bl1Key, &buf[tmp+0x148], 0x10);
				}
			}

			// now get the physical fuses
			ZeroMemory(buf, 0x200*0xC);
			src = 0x8000020000020000ULL;
			HvxPeek(src, dest, (0x200*0xC));
			// copy fuses into struct
			for(i = 0; i < 0xC; i++)
				memcpy(&nandInfo.Fuses[i*8], &buf[i*0x200], 0x8);

			// now see if there is virtual fuses
			src = 0xFFA0ULL; // this is the new/current address of virtual fuses, at the end of the first page of hv
			ZeroMemory(buf, 0x60);
			HvxPeek(src, dest, 0x60);
			if((dwbuf[0] == 0xC0FFFFFF)&&(dwbuf[1] == 0xFFFFFFFF))
			{
// 	 			DbgLog::GetInstance().log("vfuses found at FFA0\n");
				memcpy(nandInfo.VFuses, buf, 0x60);
			}
			else
			{
// 	 			DbgLog::GetInstance().log("FFA0  buf0: %08x buf1: %08x\n", dwbuf[0], dwbuf[1]);
				src = 0x8000020000019B00ULL; // this is the old jtag address of virtual fuses, at the end of the first SoC SRAM
				ZeroMemory(buf, 0x60);
				HvxPeek(src, dest, 0x60);
				if((dwbuf[0] == 0xC0FFFFFF)&&(dwbuf[1] == 0xFFFFFFFF))
				{
// 					DbgLog::GetInstance().log("vfuses found at 0x80000200_00019B00!\n");
					memcpy(nandInfo.VFuses, buf, 0x60);
				}
#ifdef FULL_DEBUG_OUT
				else
				{
// 					DbgLog::GetInstance().log("19B00 buf0: %08x buf1: %08x\n", dwbuf[0], dwbuf[1]);
					DbgLog::GetInstance().log("vfuses NOT FOUND!\n");
				}
#endif
			}
		}
#ifdef FULL_DEBUG_OUT
		else
			DbgLog::GetInstance().log("skipped stuff that requires hv peek 2+\n");
#endif

		if(USE_PEEK_VER >= HVPEEK_BASIC)
		{
			// now get the CPU key that hv is using
			ZeroMemory(buf, 0x10);
			HvxPeek(0x20ULL, dest, 0x10ULL);
			memcpy(nandInfo.CpuKey, buf, 0x10);

			// get pairing info from HV (LE format!!)
			ZeroMemory(buf, 0x10);
			HvxPeek(0x14ULL, dest, 0x4ULL);
			memcpy(&nandInfo.pairing, buf, 4);

			// get the dvd key
			tmp = 0x10;
			XeKeysGetKey(XEKEY_DVD_KEY, nandInfo.DvdKey, &tmp);
			// get the PIRS key
			tmp = 0x110;
			XeKeysGetKey(XEKEY_CONSTANT_PIRS_KEY, nandInfo.PIRSRsa, &tmp);
			// get the MASTER key
			tmp = 0x110;
			XeKeysGetKey(XEKEY_CONSTANT_MASTER_KEY, nandInfo.MASTRsa, &tmp);

		}
#ifdef FULL_DEBUG_OUT
		else
			DbgLog::GetInstance().log("skipped stuff that requires hv peek 1+\n");
#endif

		XPhysicalFree(buf);
		// read flash header into buffer
		ReadFlash(0, headerDat, 512, 512, NULL);
		fhdr = (BLDR_FLASH*)headerDat;
		if(enumImageType())
		{

			// read smc binary into buffer
			if((fhdr->dwSmcBootAddr != 0)&&(fhdr->dwSmcBootSize != 0))
			{
				smcBin = (BYTE*)VirtualAlloc(NULL, fhdr->dwSmcBootSize, MEM_COMMIT, PAGE_READWRITE);
				if(smcBin != NULL)
				{
					tmp = ReadFlash(fhdr->dwSmcBootAddr, smcBin, fhdr->dwSmcBootSize, fhdr->dwSmcBootSize, NULL);
#ifdef FULL_DEBUG_OUT
					DbgLog::GetInstance().log("read %x bytes for smc binary\n", tmp);
#endif
				}
				else
					DbgLog::GetInstance().log("unable to allocate for smc binary data!\n");
			}
			if(fhdr->dwKeyVaultSize == 0)
				w_kvSize = 0x4000;
			else
				w_kvSize = fhdr->dwKeyVaultSize;
			if((fhdr->dwKeyVaultAddr != 0)&&(w_kvSize != 0))
			{
				kvBin = (BYTE*)VirtualAlloc(NULL, w_kvSize, MEM_COMMIT, PAGE_READWRITE);
				if(kvBin != NULL)
				{
					tmp = ReadFlash(fhdr->dwKeyVaultAddr, kvBin, w_kvSize, w_kvSize, NULL);
#ifdef FULL_DEBUG_OUT
					DbgLog::GetInstance().log("read %x bytes for kv binary\n", tmp);
#endif
				}
				else
					DbgLog::GetInstance().log("unable to allocate for keyvault binary data!\n");
			}
			//nandInfo.kernelVer = XamGetSystemVersion();
			nandInfo.kernelVer = ((XboxKrnlVersion->Major&0xF)<<28);
			nandInfo.kernelVer|= ((XboxKrnlVersion->Minor&0xF)<<24);
			nandInfo.kernelVer|= ((XboxKrnlVersion->Build&0xFFFF)<<8);
			if(XboxKrnlVersion->Qfe != 0)
				nandInfo.kernelVer|= 0x8;

			getNonces();
			if(nandInfo.useFlags&PATCH_MASK_JTAG)
			{
				freebootBin = (BYTE*)VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
				if(freebootBin)
				{
					ReadFlash(0x90000, freebootBin, 0x1000, 0x1000, NULL);
				}
			}
			if(XboxHardwareInfo->BldrMagic == 0x5e4e)
				nandInfo.optFlag |= OFLAG_IS_DEVKIT;
			// get flash size here!
			// 16M falcon flash fs size is 0xf80000
			// 256M jasper flash fs size is 0x3c00000
	// 		DbgLog::GetInstance().log("flash fs size is 0x%x\n", GetFlashFsSize());
			//tmp = *((DWORD*)0x0);
			//DbgLog::GetInstance().log("crash is 0x%x\n", tmp);
			return TRUE;
		}
	}
	else
		DbgLog::GetInstance().log("unable to allocate physical buffer for readConsoleData!\n");
	return FALSE;
}

void Nand::PopulateBbList(void)
{
	DWORD blk;
	DWORD cfg = readreg(SFCX_CONFIG);
	DWORD tcfg = (cfg&~(0x3C0|CONFIG_INT_EN|CONFIG_WP_EN));
// 	DbgLog::GetInstance().log("scanning for bad blocks\n");
	if(bblist.size())
		bblist.clear();
	tcfg = tcfg|((2-1)<<6); // change to 2 pages, bb 2 pages = 8 small pages
	writereg(SFCX_CONFIG, tcfg);
	for(blk = 0; blk < (dataSz/w_writeSize); blk++)
	{
		if(workerState != WSTM_BBLIST)
		{
			writereg(SFCX_CONFIG, cfg);
#ifdef FULL_DEBUG_OUT
			DbgLog::GetInstance().log("bad block scan aborted!\n");
#endif
			return;
		}
		if(readBlock(NULL, blk) != 0)
		{
			WORD bb = blk&0xFFFF;
			bblist.push_back(bb);
		}
	}
	writereg(SFCX_CONFIG, cfg);
#ifdef FULL_DEBUG_OUT
	DbgLog::GetInstance().log("%d bad blocks found\n", bblist.size());
#endif
}

WORD* Nand::getBbList(int* psz)
{
	int sz;
	waitWorkerBusy();
	sz = bblist.size();
	if(sz > 0)
	{
		int i;
		WORD* pw = new WORD[sz];
		for(i = 0; i < sz; i++)
			pw[i] = bblist.at(i);
		*psz = sz*sizeof(WORD);
		return pw;
	}
	*psz = 0;
	return NULL;
}

DWORD Nand::workerThread(void)
{
	workerState = WSTM_BBLIST;
	doLightSync(&workerState);
	while(workerState != WSTM_NORUN)
	{
		if(workerState == WSTM_READ)
		{
// 			DbgLog::GetInstance().log("worker read\n");
			ReadFullFlash();
// 			DbgLog::GetInstance().log("worker read done\n");
		}
		else if(workerState == WSTM_WRITE)
		{
// 			DbgLog::GetInstance().log("worker write\n");
			WriteFullFlash();
// 			DbgLog::GetInstance().log("worker write done\n");
		}
		else if(workerState == WSTM_BBLIST)
		{
			if(IS_MMC_DEVICE == FALSE)
				PopulateBbList();
		}
// 		else
// 			DbgLog::GetInstance().log("worker state unknown\n");
		workerState = WSTM_IDLE;
		doLightSync(&workerState);
// 		DbgLog::GetInstance().log("worker wait\n");
		WaitForSingleObject(workerEvent, INFINITE);
// 		DbgLog::GetInstance().log("worker signaled\n");
	}
	if(bblist.size())
		bblist.clear();
// 	DbgLog::GetInstance().log("worker exiting\n");
	workerState = WSTM_EXITED;
	doLightSync(&workerState);
	return 0;
}

BOOL Nand::nandStartup(void)
{
	if(classIsReady)
	{
		phyBuf = (PBYTE)XPhysicalAlloc(0x10000, MAXULONG_PTR, 0, MEM_LARGE_PAGES|PAGE_READWRITE); //.|PAGE_NOCACHE
		if(phyBuf != NULL)
		{
			phyAddr = ((DWORD)MmGetPhysicalAddress(phyBuf)&0xFFFFFFFF);
	// 				DbgLog::GetInstance().log("phybuf: %08x phyaddr: %08x\n", phyBuf, phyAddr);
			nandData = (BYTE*)VirtualAlloc(NULL, nandInfo.dumpSize, MEM_COMMIT|MEM_LARGE_PAGES, PAGE_READWRITE);
			g_blockBuf = (BYTE*)VirtualAlloc(NULL, nandInfo.blockSize, MEM_COMMIT|MEM_LARGE_PAGES, PAGE_READWRITE);
			if((nandData != NULL)&&(g_blockBuf != NULL))
			{
				workerEvent = CreateEvent(NULL, FALSE, FALSE, "nandWork");
				if(workerEvent != NULL)
				{
					DWORD thid;
					HANDLE hTh = CreateThread(NULL, 0x10000, (LPTHREAD_START_ROUTINE)NandWorkerThreadStart, (LPVOID)this, CREATE_SUSPENDED, &thid);
					if(hTh != NULL)
					{
						XSetThreadProcessor(hTh, 5);
						ResumeThread(hTh);
						CloseHandle(hTh);
						return TRUE;
					}
					else
						DbgLog::GetInstance().log("Failed to create NandWorkerThread, error %d\n", GetLastError());
				}
			}
		}
	}
	else
		DbgLog::GetInstance().log("couldn't start Nand, class is not ready!\n");
	nandShutdown();
	return FALSE;
}

void Nand::nandShutdown(void)
{
	// nandData = (BYTE*)VirtualAlloc(NULL, nandInfo.dumpSize, MEM_COMMIT|MEM_LARGE_PAGES, PAGE_READWRITE);
	// VirtualFree(nandData, 0, MEM_RELEASE);

	if(nandData != NULL)
		VirtualFree(nandData, 0, MEM_RELEASE);
	nandData = NULL;

	if(g_blockBuf != NULL)
		VirtualFree(g_blockBuf, 0, MEM_RELEASE);
	g_blockBuf = NULL;

	if(phyBuf != NULL)
		XPhysicalFree(phyBuf);
	phyBuf = NULL;

	if(workerState > WSTM_NORUN)
	{
		workerState = WSTM_NORUN;
		SetEvent(workerEvent);
		while(workerState != WSTM_EXITED)
			Sleep(25);
		workerState = WSTM_NORUN;
	}
	CloseHandle(workerEvent);
}
