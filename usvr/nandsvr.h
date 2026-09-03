#pragma once

#include "nand.h"

#define MAX_CMD_SZ			1024
#define NANDSVR_PORT		49
#define COMMAND_TIMEOUT		300
#define DATA_TIMEOUT		15
#define CMD_MAGIC_BE		0x4E537672 // 'NSvr', use 0x7276534E on little endian
#define CMD_MAGIC_LE		0x7276534E // little endian 'NSvr'
#define PACKET_SIZE			64*1024 //8192
#define PACKET_SIZE_SEND	1452

// TODO: add flash erase! allow second arg that permits unsafe erase
typedef enum {
	CMD_QUIT = 0,		// QUIT disconnect
	CMD_SHUTDOWN,		// SHDN shutdown xbox
	CMD_REBOOT,			// REEB reboot xbox
	CMD_SMCREBOOT,		// SMRB reset xbox via SMC reset
	CMD_GETINFO,		// GTIN get console info, including things like cpu key
	CMD_GETBBLIST,		// GTBB gets a list of bad blocks
	CMD_GETFLASH,		// GTFL retrieve flash dump of size specified by INFO struct
	CMD_WRITEFLASH,		// WRFL send flash image and write it of size specified by INFO struct, should be followed by a reboot or shutdown
	CMD_READBLOCK,		// RBLK read a block of flash
	CMD_WRITEBLOCK,		// WBLK write a block of flash
	CMD_ERASEBLOCK,		// ERBL erase a block, even if it's marked as bad
	CMD_GETBOOTLOADERS, // GTBL gets the bootloaders in RAW format
	CMD_WRITEPATCH,		// WBPT write patches to flash
	CMD_MOUNTPATH,		// MTPT mounts a device to a path
	CMD_UNMOUNTPATH,	// UMPT unmount a path
	CMD_GETFILE,		// GETF gets a file from the console
	CMD_SENDFILE,		// SNDF sends a file to the console
	CMD_FORMATSYSEX,	// FMEX format sysex hdd partition
	CMD_FORMATCOMPAT,	// FMCM format xbox compatibility partition
	CMD_MKDIR,			// MKDR	make directory
	CMD_HVPEEK,			// HVPE
	CMD_HVPOKE,			// HVPO
	CMD_PEEK,			// PEEK
	CMD_POKE,			// POKE
	CMD_GET1BL,			// GT1B peek the 1bl
	CMD_HVDUMP,			// HVDM dumps all hv space into 1 flat file
	CMD_GETVER,			// GTVR
	CMD_MAX
};

class NandSvr
{
public:	
	NandSvr();
	~NandSvr();

	BOOL startup(BOOL* annc);
	void shutdown(void);
	BOOL getConnected(void){return isConnected;}
	BOOL getRunning(void){return isRunning;}
	DWORD getStatus(void);
	DWORD NandsvrLisThread(void);
	DWORD NandsvrDatThread(void);
private:
	void endClient(void);
	BOOL doHvDump(void);
	BOOL do1blDump(void);
	BOOL doMount(char* cmd);
	BOOL doUnmount(char* cmd);
	BOOL doSendBbList(void);
	BOOL doSendFileFragsSz(DWORD sz);
	BOOL doSendFileFrags(char* cmd);
	BOOL doSendFile(char* cmd);
	BOOL doRecFile(char* cmd);
	BOOL doExFormat(void);
	BOOL doCompatFormat(void);
	BOOL doMakeDir(char* path);
	BOOL doWriteFlash(void);
	BOOL doReadFlash(void);
	BOOL parseReadWriteBlock(char* cmd, DWORD* blk, DWORD* blnum, int cmdlen);
	BOOL doReadBlock(char* arg, int cmdlen);
	BOOL doWriteBlock(char* arg, int cmdlen);
	BOOL doEraseBlock(char* arg, int cmdlen);
	BOOL doGetBootloaders();
	BOOL doWritePatch(void);
	BOOL doSendVer(void);
	BOOL parseHvPePo(char* cmd, QWORD* addr, DWORD* len, int cmdlen);
	BOOL doHvPeek(char* cmd, int cmdlen);
	BOOL doHvPoke(char* cmd, int cmdlen);
	BOOL parsePePo(char* cmd, DWORD* addr, DWORD* len, int cmdlen);
	BOOL doPeek(char* cmd, int cmdlen);
	BOOL doPoke(char* cmd, int cmdlen);

	int enumCmd(char* cmd);
	int SocketReceiveCommand(char* cmd);
	int SocketReceiveData(void* data, DWORD dwBytesToRead, DWORD* dwBytesRec = NULL);
	int SocketReceiveDataFixedSz(void* data, DWORD dwBytesToRead, DWORD* dwBytesRec = NULL);
	BOOL SocketSendDataFixedSz(void* data, DWORD dwBytesToSend, DWORD* dwBytesAvail = NULL);
	BOOL SocketSendData(void* data, DWORD dwBytesToSend);
	void sinit(void);
	SOCKET sListen;
	SOCKET sClient;
	BOOL isConnected;
	BOOL isRunning;
	BOOL* udpIsAnnc;
};
