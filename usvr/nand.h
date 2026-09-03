// everything relevant to updating the NAND
#pragma once
#include <vector>

#define MAX_PATCH_SIZE	0x4000

typedef enum {
	WSTM_EXITED = 0,
	WSTM_NORUN,
	WSTM_IDLE,
	WSTM_BBLIST,
	WSTM_WRITE,
	WSTM_READ,
};

#define OFLAG_IS_DEVKIT		0x1
#define OFLAG_HAS_BLMOD		0x2
#define OFLAG_HAS_ADDONS	0x4
#define OFLAG_HAS_BIGFFS	0x8
#define OFLAG_KEEP_BLS		0x10

typedef struct _NAND_INFO{
	DWORD structVer;	// initially will be 0 comprised of xxxxxxyy, yy is peek ver
	DWORD kernelVer;	// current kernel version
	DWORD optFlag;		// just to tell us some bitwise things
	DWORD useFlags;		// flags for patch info, from dash launch patch updater
	DWORD hwFlags;		// from xboxhardwareinfo
	DWORD dumpSize;		// file size of formatted dump for this NAND type
	DWORD blockSize;	// size of each block, including spare
	DWORD pairing;		// pairing data in little endian order 0xA92BED02 = pairing 0xED2BA9 LDV 02
	BYTE CpuKey[16];	// cpu key from hv space
	BYTE DvdKey[16];	// dvd key from kv
	BYTE Fuses[0x60];	// hardware fuses
	BYTE VFuses[0x60];	// all 0 if not present
	BYTE Bl1Key[16];	// RSA magic key from 1BL
	BYTE BL1Rsa[0x110]; // RSA key from 1BL
	BYTE PIRSRsa[0x110];// PIRS public key from hv
	BYTE MASTRsa[0x110];// master public key from hv
	BYTE CBANonce[0x10];
	BYTE CBBNonce[0x10];
	BYTE CDNonce[0x10];
	BYTE CENonce[0x10];
	BYTE CFNonce[0x10];
	BYTE CGNonce[0x10];
} NAND_INFO, *PNAND_INFO;

class Nand
{
public:	
	static Nand& Inst(){static Nand singleton; return singleton;}
	NAND_INFO nandInfo;
	BYTE* nandData;
	void waitWorkerBusy(void){while(workerState>WSTM_IDLE)Sleep(10);}
	DWORD getWorkerState(void){return workerState;}
	DWORD* setWorkerWrite(void){waitWorkerBusy(); workerSize = 0; workerState = WSTM_WRITE; doLightSync(&workerState); SetEvent(workerEvent); return &workerSize;}
	DWORD* setWorkerRead(void){waitWorkerBusy(); workerSize = 0; workerState = WSTM_READ; doLightSync(&workerState); SetEvent(workerEvent); return &workerSize;}
	void setWorkerBbList(void){waitWorkerBusy(); workerState = WSTM_BBLIST; doLightSync(&workerState); SetEvent(workerEvent);}
	void setWorkerIdle(void){workerState = WSTM_IDLE; doLightSync(&workerState);} // for aborting
	BYTE* getHeader(void){return headerDat;}
	BYTE* getKvBin(DWORD* sz){*sz = w_kvSize; return kvBin;}
	BYTE* getSmcBin(DWORD* sz){*sz = fhdr->dwSmcBootSize; return smcBin;}
	BYTE* getFreebootBin(DWORD* sz){if(freebootBin) *sz = 0x1000; return freebootBin;}
	BYTE* getAddonPatch(DWORD* sz) {*sz = w_extraSize; return w_extras;}
	DWORD getConType(void){return nandInfo.useFlags;}
	DWORD getBlockSize(void){return nandInfo.blockSize;}
	BYTE* getPatchDataBuf(DWORD* sz){*sz = w_currPatchSize; return w_currPatchData;}
	BYTE* getBlModBuf(DWORD* sz){*sz = w_BlModSz; return w_BlMod;}
	BOOL getPatchData(void* bout, int blen);
	BOOL setPatchData(void* bin, int blen);

	BOOL GetExtras(void);
	PBYTE readBlocks(DWORD block, int numBlocks, DWORD* osz);
	PBYTE getBootloaders(DWORD* osz);
	BOOL writeBlocks(PBYTE buf, DWORD block, int numBlocks);
	BOOL eraseBlocks(DWORD block, int numBlocks);
	BOOL nandStartup(void);
	void nandShutdown(void);
	DWORD workerThread(void);
	WORD* getBbList(int* sz);
	BYTE* phyBuf;
private:
	DWORD phyAddr;
	HANDLE workerEvent;
	DWORD workerState;// tells worker what it's supposed to be doing
	DWORD workerSize; // tracks worker progress for read and data stream progress for write
	void writereg(DWORD addr, DWORD data);
	DWORD readreg(DWORD addr);
	BOOL BlockHasData(PBYTE data);
	DWORD GetFlashFsSize(void);
	DWORD ReadFlash(DWORD offset, PBYTE buf, DWORD len, DWORD readSize, PDWORD pbRead);
	DWORD WriteFlash(DWORD offset, PBYTE buf, DWORD len, DWORD writeSize, PDWORD bAvail);
	BOOL ReadFullFlash(void);
	BOOL WriteFullFlash(void);
	DWORD readBlock(PBYTE data, DWORD block);
	DWORD eraseBlock(DWORD block);
	DWORD writeBlock(PBYTE data, DWORD block);
	void PopulateBbList(void);
	BOOL updatePatchData(void);
	DWORD getU32(PBYTE data);

	BOOL enumerateNand(void);
	BOOL readBlData(DWORD* currOff, char hchar, BYTE** buf, BYTE* headbuf);
	BOOL getBlModData(DWORD* curroff);
	BOOL getNonces(void);
	BOOL readConsoleData(void);
	DWORD getCBBVer(DWORD off);
	BOOL enumImageType(void);

	BYTE headerDat[512];
	BYTE* g_blockBuf;
	BYTE* smcBin;
	BYTE* kvBin;
	DWORD w_kvSize;
	BYTE* freebootBin;
	BYTE* w_currPatchData;
	DWORD w_currPatchSize;
	BYTE* w_extras;
	DWORD w_extraSize;
	BYTE* w_BlMod;
	DWORD w_BlModSz;
	BLDR_FLASH* fhdr;
	std::vector <WORD> bblist;
	DWORD configSave;
	DWORD dataSz;
	DWORD spareSz;
	DWORD pagesPerBlock;
	DWORD w_writeSize; // happens to be block size and write alignment all in one, also logical block size
	DWORD w_updateSize; // two blocks
	DWORD w_blockOff; // start of the two blocks
	DWORD w_patchOff; // start of patches in the two blocks
	DWORD w_PatchInfo;

	BOOL w_IsJtag;
	BOOL w_IsGlitch2;
	BOOL w_IsGlitch2m;
	DWORD w_ConsoleType;

	BOOL isBigBlock;
	BOOL isBigBlockCont;

	BOOL classIsReady;
	BOOL ClassStartup(void);
	void ClassShutdown(void);
	Nand();
	~Nand();
	Nand(const Nand&);                 // Prevent copy-construction
	Nand& operator=(const Nand&);      // Prevent assignment
};

#pragma pack(push, 1)
typedef struct _METADATA_SMALLBLOCK{
	unsigned char BlockID1; // lba/id = (((BlockID0<<8)&0xF)+(BlockID1&0xFF))
	unsigned char FsUnused0 : 4;
	unsigned char BlockID0 : 4; 
	unsigned char FsSequence0; // oddly these aren't reversed
	unsigned char FsSequence1;
	unsigned char FsSequence2;
	unsigned char BadBlock;
	unsigned char FsSequence3;
	unsigned char FsSize1; // (((FsSize0<<8)&0xFF)+(FsSize1&0xFF)) = cert size
	unsigned char FsSize0;
	unsigned char FsPageCount; // free pages left in block (ie: if 3 pages are used by cert then this would be 29:0x1d)
	unsigned char FsUnused1[2];
	unsigned char ECC3 : 2;
	unsigned char FsBlockType : 6;
	unsigned char ECC2; // 26 bit ECD
	unsigned char ECC1;
	unsigned char ECC0;
} SMALLBLOCK, *PSMALLBLOCK;

typedef struct _METADATA_BIGONSMALL{
	unsigned char FsSequence0;
	unsigned char BlockID1; // lba/id = (((BlockID0<<8)&0xF)+(BlockID1&0xFF))
	unsigned char FsUnused0 : 4;
	unsigned char BlockID0 : 4; 
	unsigned char FsSequence1;
	unsigned char FsSequence2;
	unsigned char BadBlock;
	unsigned char FsSequence3;
	unsigned char FsSize1; // (((FsSize0<<8)&0xFF)+(FsSize1&0xFF)) = cert size
	unsigned char FsSize0;
	unsigned char FsPageCount; // free pages left in block (ie: if 3 pages are used by cert then this would be 29:0x1d)
	unsigned char FsUnused1[2];
	unsigned char ECC3 : 2;
	unsigned char FsBlockType : 6;
	unsigned char ECC2; // 26 bit ECD
	unsigned char ECC1;
	unsigned char ECC0;
} BIGONSMALL, *PBIGONSMALL;

//off: 2CDC000 [2b80000] block: 15C LBA: 15c : ff 5c 01 00 00 00 00 06 20 04 00 00 aa 21 14 1c :t 2a :root: 1FFE : 
//off: 0BDC000 [0b80000] block: 05C LBA: 05c : ff 5c 00 00 00 01 00 00 08 3f 00 00 b1 87 b5 b5 :t 31 :root: 005D : MobileB.dat ver: 10000 size: 0x800
typedef struct _METADATA_BIGBLOCK{
	unsigned char BadBlock;
	unsigned char BlockID1; // lba/id = (((BlockID0<<8)&0xF)+(BlockID1&0xFF))
	unsigned char FsUnused0 : 4;
	unsigned char BlockID0 : 4;
	unsigned char FsSequence2; // oddly, compared to before these are reversed...?
	unsigned char FsSequence1;
	unsigned char FsSequence0;
	unsigned char FsUnused1;
	unsigned char FsSize1; // FS: 06 (system reserve block number) else ((FsSize0<<16)+(FsSize1<<8)) = cert size
	unsigned char FsSize0; // FS: 20 (size of flash filesys in smallblocks >>5)
	unsigned char FsPageCount; // FS: 04 (system config reserve) free pages left in block (multiples of 4 pages, ie if 3f then 3f*4 pages are free after)
	unsigned char FsUnused2[0x2];
	unsigned char ECC3 : 2;
	unsigned char FsBlockType : 6; // FS: 2a bitmap: 2c (both use FS: vals for size), mobiles
	unsigned char ECC2; // 26 bit ECD
	unsigned char ECC1;
	unsigned char ECC0;
} BIGBLOCK, *PBIGBLOCK;

typedef struct _METADATA{
	union{
		SMALLBLOCK sm;
		BIGBLOCK bg;
		BIGONSMALL bos;
	};
} METADATA, *PMETADATA;

typedef struct _PAGEDATA{
	unsigned char user[512];
	METADATA meta;
} PAGEDATA, *PPAGEDATA;

#pragma pack(pop)
