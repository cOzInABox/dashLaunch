#include <xtl.h>
#include <string>
#include "xkelib.h"
#include "util.h"
#include "nandsvr.h"
#include "sysex.h"
#include "logging.h"

using namespace std;

#define CMD_LEN				4
#define FILE_SEND_SIZE		(PACKET_SIZE_SEND*30)

static BYTE g_fbuf[FILE_SEND_SIZE];

static char okreply[] = "OK";
static char noreply[] = "NO";
typedef struct _CMD_LIST{
	char* cmd;
	int cenum;
} CMD_LIST;

static const CMD_LIST cmdList[CMD_MAX] ={
	{"QUIT", CMD_QUIT},
	{"SHDN", CMD_SHUTDOWN},
	{"REEB", CMD_REBOOT},
	{"SMRB", CMD_SMCREBOOT},
	{"GTIN", CMD_GETINFO},
	{"GTBB", CMD_GETBBLIST},
	{"GTFL", CMD_GETFLASH},
	{"WRFL", CMD_WRITEFLASH},
	{"RBLK", CMD_READBLOCK},
	{"WBLK", CMD_WRITEBLOCK},
	{"ERBL", CMD_ERASEBLOCK},
	{"GTBL", CMD_GETBOOTLOADERS},
	{"WBPT", CMD_WRITEPATCH},
	{"MTPT", CMD_MOUNTPATH},
	{"UMPT", CMD_UNMOUNTPATH},
	{"GETF", CMD_GETFILE},
	{"SNDF", CMD_SENDFILE},
	{"FMEX", CMD_FORMATSYSEX},
	{"FMCM", CMD_FORMATCOMPAT},
	{"MKDR", CMD_MKDIR},
	{"HVPE", CMD_HVPEEK},
	{"HVPO", CMD_HVPOKE},
	{"PEEK", CMD_PEEK},
	{"POKE", CMD_POKE},
	{"GT1B", CMD_GET1BL},
	{"HVDM", CMD_HVDUMP},
	{"GTVR", CMD_GETVER},
};

// this starts the connection listener thread
static DWORD NandsvrLisThreadStart(void* Param)
{
	NandSvr* This = (NandSvr*) Param;
	return This->NandsvrLisThread();
}

// this starts the connected data thread
static DWORD NandsvrDatThreadStart(void* Param)
{
	NandSvr* This = (NandSvr*) Param;
	return This->NandsvrDatThread();
}

NandSvr::NandSvr()
{
	sClient = INVALID_SOCKET;
	sListen = INVALID_SOCKET;
	isConnected = FALSE;
	isRunning = FALSE;
}

NandSvr::~NandSvr()
{

}

// initialize socket stuffs
void NandSvr::sinit(void)
{
	if(sClient != INVALID_SOCKET)
		closesocket(sClient);
	if(sListen != INVALID_SOCKET)
		closesocket(sListen);
	sClient = INVALID_SOCKET;
	sListen = INVALID_SOCKET;
	isConnected = FALSE;
}

BOOL NandSvr::startup(BOOL* annc)
{
	udpIsAnnc = annc;
	if(Nand::Inst().nandStartup())
	{
		DWORD thid;
		HANDLE hTh;
		sinit();
		hTh = CreateThread(NULL, 0x10000, (LPTHREAD_START_ROUTINE)NandsvrLisThreadStart, (LPVOID)this, CREATE_SUSPENDED, &thid);
		if(hTh == NULL)
		{
			DbgLog::GetInstance().log("Failed to create NandsvrThread, error %d\n", GetLastError());
			return FALSE;
		}
		XSetThreadProcessor(hTh, 5);
		isRunning = TRUE;
		ResumeThread(hTh);
		CloseHandle(hTh);
		return TRUE;
	}
	DbgLog::GetInstance().log("failed to complete nand startup\n");
	return FALSE;
}

void NandSvr::shutdown(void)
{
	isRunning = FALSE;
	sinit();
	Nand::Inst().nandShutdown();
}

// this is the connection listener thread
DWORD NandSvr::NandsvrLisThread(void)
{
	SOCKADDR_IN saddr;
	int clients = 0;
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sListen == INVALID_SOCKET)
	{
		DbgLog::GetInstance().log("NandSvr failed to acquire socket, error %d\n", WSAGetLastError());
		return FALSE;
	}
	if(setSockSecurity(sListen) == FALSE)
		return FALSE;
	memset(&saddr, 0, sizeof(SOCKADDR_IN));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(NANDSVR_PORT);
	if(bind(sListen, (struct sockaddr *) &saddr, sizeof(SOCKADDR_IN)) != 0)
	{
		DbgLog::GetInstance().log("unable to bind listen socket, error %d\n", WSAGetLastError());
		return FALSE;
	}
	if(listen(sListen, SOMAXCONN) != 0)
	{
		DbgLog::GetInstance().log("unable to listen on socket, error %d\n", WSAGetLastError());
		return FALSE;
	}
	while(isRunning)
	{
		SOCKADDR_IN ac;
		SOCKET saccp;
		int acsz = sizeof(SOCKADDR_IN);
		if ((saccp = accept(sListen, (sockaddr*)&ac, &acsz)) != INVALID_SOCKET)
		{
			if(sClient == INVALID_SOCKET)
			{
				DWORD thid;
				HANDLE hTh;
				sClient = saccp;
				isConnected = TRUE;
				DbgLog::GetInstance().log("accepted client from %d.%d.%d.%d:%d\n", ac.sin_addr.s_net, ac.sin_addr.s_host, ac.sin_addr.s_lh, ac.sin_addr.s_impno, ac.sin_port);
				hTh = CreateThread(NULL, 0x10000, (LPTHREAD_START_ROUTINE)NandsvrDatThreadStart, (LPVOID)this, 0, &thid);
				CloseHandle(hTh);
				*udpIsAnnc = FALSE;
			}
			else
			{
				DbgLog::GetInstance().log("rejected client from %d.%d.%d.%d:%d\n", ac.sin_addr.s_net, ac.sin_addr.s_host, ac.sin_addr.s_lh, ac.sin_addr.s_impno, ac.sin_port);
				closesocket(saccp);
			}
		}
		else
			DbgLog::GetInstance().log("accept failed %d\n", WSAGetLastError());
	}
	return 0;
}

int NandSvr::SocketReceiveCommand(char* cmd)
{
	DWORD ret;
	int iBytes = 0;
	TIMEVAL tv;
	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(sClient, &fds);
	tv.tv_sec = COMMAND_TIMEOUT;
	tv.tv_usec = 0;
	while(iBytes < MAX_CMD_SZ)
	{
		ret = select(0, &fds, 0, 0, &tv);
		if((ret == SOCKET_ERROR) || (ret == 0))
		{
// 			DbgLog::GetInstance().log("timeout waiting for command, %d\n", WSAGetLastError());
			return -1; // Timeout
		}
		ret = recv(sClient, cmd, 1, 0);
// 		DbgLog::GetInstance().log("rec %x %c\n", *cmd, *cmd);
		if((ret == SOCKET_ERROR) || (ret == 0))
		{
// 			DbgLog::GetInstance().log("socket error receiving command, %d\n", WSAGetLastError());
			return -1; // Network error
		}
		if (*cmd == '\r')
			*cmd = 0;
		else if (*cmd == '\n')
		{
			*cmd = 0;
			return iBytes;
		}
		iBytes++;
		cmd++;
	}
	return 0;
}

int NandSvr::SocketReceiveData(void* data, DWORD dwBytesToRead, DWORD* dwBytesRec)
{
	int bRead = 0, ret;
	char *psz = (char*)data;
	TIMEVAL tv;
	fd_set fds;

	tv.tv_sec = DATA_TIMEOUT;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(sClient,&fds);
	ret = select(0, &fds, 0, 0, &tv);
	if((ret == SOCKET_ERROR) || (ret == 0))
	{
// 		DbgLog::GetInstance().log("timeout waiting for data, %d\n", WSAGetLastError());
		return -1; // Timeout
	}
	while(bRead < (int)dwBytesToRead)
	{
		ret = recv(sClient, &psz[bRead], (dwBytesToRead-bRead), 0);
// 		DbgLog::GetInstance().log("rec %x bytes\n", ret);
		if(ret == SOCKET_ERROR)
		{
// 			DbgLog::GetInstance().log("socket error receiving data, %d\n", WSAGetLastError());
			return -1; // Network error
		}
		bRead += ret;
		if(dwBytesRec)
		{
			*dwBytesRec = bRead;
			doLightSync(dwBytesRec);
		}
	}
// 	DbgLog::GetInstance().log("rec ttl %x bytes\n", bRead);
	return bRead;
}

int NandSvr::SocketReceiveDataFixedSz(void* data, DWORD dwBytesToRead, DWORD* dwBytesRec)
{
	DWORD dwTmp;
	if(SocketReceiveData(&dwTmp, 4, NULL) != 4)
		return 0;
	if(dwTmp != dwBytesToRead)
		return 0;
	return SocketReceiveData(data, dwBytesToRead, dwBytesRec);
}

BOOL NandSvr::SocketSendDataFixedSz(void* data, DWORD dwBytesToSend, DWORD* dwBytesAvail)
{
	int ret;
	DWORD bsent = 0, nsend = PACKET_SIZE_SEND;
	char* psz = (char*)data;
	ret = send(sClient, (char*)&dwBytesToSend, 4, 0);
	if(ret == SOCKET_ERROR)
	{
		DbgLog::GetInstance().log("socket error sending data size, %d\n", WSAGetLastError());
		return FALSE;
	}
	if((data != NULL)&&(dwBytesToSend != 0)) // so we can send a 0 reply for no data
	{
		while(bsent < dwBytesToSend)
		{
			if((dwBytesToSend-bsent) < PACKET_SIZE_SEND)
				nsend = dwBytesToSend-bsent;
			if(dwBytesAvail != NULL)
			{
				while(*dwBytesAvail < (bsent+nsend))
					Sleep(10);
			}
// 			DbgLog::GetInstance().log("sending 0x%x:0x%x of 0x%x sent\n", nsend, bsent, dwBytesToSend);
			ret = send(sClient, &psz[bsent], nsend, 0);
			if(ret == SOCKET_ERROR)
			{
				DbgLog::GetInstance().log("socket error sending data (1), %d\n", WSAGetLastError());
				return FALSE;
			}
			bsent += ret;
		}
	// 		DbgLog::GetInstance().log("sent %x bytes of %x\n", ret, sending);
	}
	return TRUE;
}

BOOL NandSvr::SocketSendData(void* data, DWORD dwBytesToSend)
{
	int ret;
	DWORD bsent = 0, nsend = PACKET_SIZE_SEND;
	char* psz = (char*)data;
	if((data != NULL)&&(dwBytesToSend != 0)) // so we can send a 0 reply for no data
	{
		while(bsent < dwBytesToSend)
		{
			if((dwBytesToSend-bsent) < PACKET_SIZE_SEND)
				nsend = dwBytesToSend-bsent;
			// 			DbgLog::GetInstance().log("sending 0x%x:0x%x of 0x%x sent\n", nsend, bsent, dwBytesToSend);
			ret = send(sClient, &psz[bsent], nsend, 0);
			if(ret == SOCKET_ERROR)
			{
				DbgLog::GetInstance().log("socket error sending data (1), %d\n", WSAGetLastError());
				return FALSE;
			}
			bsent += ret;
		}
		// 		DbgLog::GetInstance().log("sent %x bytes of %x\n", ret, sending);
	}
	return TRUE;
}

void NandSvr::endClient(void)
{
	isConnected = FALSE;
	*udpIsAnnc = TRUE;
	if(sClient != INVALID_SOCKET)
	{
		//if(shutdown(sClient, SD_BOTH)!= 0)
		//{
		//	DbgLog::GetInstance().log("shutdown client socket failed, err %d\n", WSAGetLastError());
		//}
// 		DbgLog::GetInstance().log("close socket now!\n");
		if(closesocket(sClient) != 0)
		{
			DbgLog::GetInstance().log("close client socket failed, err %d\n", WSAGetLastError());
		}
		sClient = INVALID_SOCKET;
	}
	isConnected = FALSE;
// 	DbgLog::GetInstance().log("client connection has ended!\n");
}

int NandSvr::enumCmd(char* cmd)
{
	int i;
	for(i = 0; i < CMD_MAX; i++)
	{
		if(strnicmp(cmd, cmdList[i].cmd, 4) == 0)
			return cmdList[i].cenum;
	}
	return -1;
}

BOOL NandSvr::doHvDump(void)
{
	BOOL ret = FALSE;
	PBYTE buf = (PBYTE)XPhysicalAlloc(0x40000, MAXULONG_PTR, 0, MEM_LARGE_PAGES|PAGE_READWRITE|PAGE_NOCACHE);
	// 4 x 64k Pages
	if(buf != NULL)
	{
		if((Nand::Inst().nandInfo.structVer&0xFF) >= HVPEEK_BASIC)
		{
			int i;
			QWORD dest = 0x8000000000000000ULL | ((DWORD)MmGetPhysicalAddress(buf)&0xFFFFFFFF);
			QWORD src = 0;
			ZeroMemory(buf, 0x40000);
			for(i = 0; i < 4; i++)
			{
				HvxPeek(src, dest, 0x10000);
				src = src + (0x200010000ULL);
				dest += 0x10000ULL;
			}
			if(SocketSendDataFixedSz(buf, 0x40000))
				ret = TRUE;
			else
				DbgLog::GetInstance().log("error sending hv data on socket!\n");
		}
		XPhysicalFree(buf);
	}
	else
		DbgLog::GetInstance().log("unable to allocate physical buffer for hvpeeks! error %d\n", GetLastError());
	if(ret == FALSE)
	{
		if(SocketSendDataFixedSz(NULL, 0))
			ret = TRUE;
// 		else
// 			DbgLog::GetInstance().log("error sending hv dummy data on socket!\n");
	}
	return ret;
}

BOOL NandSvr::do1blDump(void)
{
	BOOL ret = FALSE;
	PBYTE buf = (PBYTE)XPhysicalAlloc(0x8000, MAXULONG_PTR, 0, MEM_LARGE_PAGES|PAGE_READWRITE|PAGE_NOCACHE);
	// 4 x 64k Pages
	if(buf != NULL)
	{
		if((Nand::Inst().nandInfo.structVer&0xFF) > HVPEEK_BASIC)
		{
			QWORD dest = 0x8000000000000000ULL | ((DWORD)MmGetPhysicalAddress(buf)&0xFFFFFFFF);
			QWORD src = 0x8000020000000000ULL;
			ZeroMemory(buf, 0x8000);
			HvxPeek(src, dest, 0x8000);
			if(SocketSendDataFixedSz(buf, 0x8000))
				ret = TRUE;
// 			else
// 				DbgLog::GetInstance().log("error sending 1bl data on socket!\n");
		}
		XPhysicalFree(buf);
	}
	else
		DbgLog::GetInstance().log("unable to allocate physical buffer for hvpeeks! error %d\n", GetLastError());
	if(ret == FALSE)
	{
		if(SocketSendDataFixedSz(NULL, 0))
			ret = TRUE;
// 		else
// 			DbgLog::GetInstance().log("error sending hv dummy data on socket!\n");
	}
	return ret;
}

// like drive: \\device\\hello
BOOL NandSvr::doMount(char* cmd)
{
	const char* rep = noreply;
	// split cmd at space into 2 args, device and path
	string sdrv = cmd;
	int pos = sdrv.find_first_of(' ');
	if(pos != -1)
	{
		string sdev = sdrv.substr(pos+1);
		sdrv.erase(pos);
// 		DbgLog::GetInstance().log("mounting %s to %s\n", sdev.c_str(), sdrv.c_str());
		if(MountPath(sdrv.c_str(), sdev.c_str(), FALSE) == S_OK)
			rep = okreply;
	}
// 	DbgLog::GetInstance().log("sending reply %s\n", rep);
	if(!SocketSendDataFixedSz((void*)rep, 2))
		return FALSE;
	return TRUE;
}

BOOL NandSvr::doUnmount(char* cmd)
{
	const char* rep = noreply;
	HRESULT hr = deleteLink(cmd, FALSE);
	// take cmd and unmount it
	if(hr == S_OK)
		rep = okreply;
	else
		printf("unmount failed, HRES %08x\n", hr);
	if(!SocketSendDataFixedSz((void*)rep, 2))
		return FALSE;
	return TRUE;
}

BOOL NandSvr::doSendBbList(void)
{
	BOOL ret = FALSE;
	int sz;
	PWORD dat = Nand::Inst().getBbList(&sz);
	if(SocketSendDataFixedSz(dat, sz))
		ret = TRUE;
	if(dat)
		delete[] dat;
	return ret;
}

BOOL NandSvr::doSendFileFragsSz(DWORD sz)
{
	if(SocketSendData(&sz, 4))
		return TRUE;
	return FALSE;
}

BOOL NandSvr::doSendFileFrags(char* cmd)
{
	BOOL ret = TRUE;
	HANDLE hFile = CreateFile(cmd, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwSize = 0;
		dwSize = GetFileSize(hFile, NULL);
		ret = doSendFileFragsSz(dwSize);
		if((ret)&&(dwSize != 0))
		{
			DWORD dwRead, dwSent = 0, dwSsz = FILE_SEND_SIZE;
			while(dwSent < dwSize)
			{
				if((dwSize - dwSent) < FILE_SEND_SIZE)
					dwSsz = dwSize - dwSent;
				ReadFile(hFile, g_fbuf, dwSsz, &dwRead, NULL);
				if(SocketSendData(g_fbuf, dwSsz) == FALSE)
				{
					ret = FALSE;
					dwSent = dwSize;
				}
				dwSent+=dwRead;
			}
		}
		CloseHandle(hFile);
	}
	else
		ret = doSendFileFragsSz(0xFFFFFFFF); // file doesn't exist
	return ret;
}

// need to get flash header, kv and smc.bin too
BOOL NandSvr::doSendFile(char* cmd)
{
	PBYTE fdat = NULL;
	DWORD dwSize = 0;
	BOOL ret = FALSE;
// 	DbgLog::GetInstance().log("reading file %s\n", cmd);
	if(strnicmp(cmd, "kv_enc", 6) == 0)
	{
		fdat = Nand::Inst().getKvBin(&dwSize);
// 		DbgLog::GetInstance().log("sending kv_enc, p:%08x sz:%08x", fdat, dwSize);
		if(SocketSendDataFixedSz(fdat, dwSize))
			ret = TRUE;
	}
	else if(strnicmp(cmd, "smc_enc", 7) == 0)
	{
		fdat = Nand::Inst().getSmcBin(&dwSize);
// 		DbgLog::GetInstance().log("sending smc_enc, p:%08x sz:%08x", fdat, dwSize);
		if(SocketSendDataFixedSz(fdat, dwSize))
			ret = TRUE;
	}
	else if(strnicmp(cmd, "flash_hdr", 9) == 0)
	{
// 		DbgLog::GetInstance().log("sending flash_hdr, p:%08x sz:%08x", Nand::Inst().getHeader(), 512);
		if(SocketSendDataFixedSz(Nand::Inst().getHeader(), 512))
			ret = TRUE;
	}
	else if(strnicmp(cmd, "freeboot_bin", 12) == 0)
	{
		fdat = Nand::Inst().getFreebootBin(&dwSize);
// 		DbgLog::GetInstance().log("sending freeboot_bin, p:%08x sz:%08x", fdat, dwSize);
		if(SocketSendDataFixedSz(fdat, dwSize))
			ret = TRUE;
	}
	else if(strnicmp(cmd, "patches", 7) == 0)
	{
		fdat = Nand::Inst().getPatchDataBuf(&dwSize);
// 		DbgLog::GetInstance().log("sending patches, p:%08x sz:%08x", fdat, dwSize);
		if(SocketSendDataFixedSz(fdat, dwSize))
			ret = TRUE;
	}
	else if(strnicmp(cmd, "addons", 6) == 0)
	{
		fdat = Nand::Inst().getAddonPatch(&dwSize);
// 		DbgLog::GetInstance().log("sending addons, p:%08x sz:%08x", fdat, dwSize);
		if(SocketSendDataFixedSz(fdat, dwSize))
			ret = TRUE;
	}
	else if(strnicmp(cmd, "blmod", 5) == 0)
	{
		fdat = Nand::Inst().getBlModBuf(&dwSize);
// 		DbgLog::GetInstance().log("sending blmod, p:%08x sz:%08x", fdat, dwSize);
		if(SocketSendDataFixedSz(fdat, dwSize))
			ret = TRUE;
	}
	else
	{
		ret = doSendFileFrags(cmd);
	}
	return ret;
}

BOOL NandSvr::doRecFile(char* cmd)
{
	DWORD dwSize = 0;
	const char* rep = noreply;
	BOOL ret = FALSE;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	if(IsFileExist(cmd))
		DeleteFileA(cmd);
	hFile = CreateFile(cmd, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(SocketReceiveData(&dwSize, 4, NULL) != -1)
	{
		DWORD dwRec = 0, dwRsz = FILE_SEND_SIZE, dwBrec;
// 		DbgLog::GetInstance().log("receiving file %s size 0x%x\n", cmd, dwSize);
		while(dwRec < dwSize)
		{
			if((dwSize - dwRec) < FILE_SEND_SIZE)
				dwRsz = (dwSize - dwRec);
			dwBrec = SocketReceiveData(g_fbuf, dwRsz, NULL);
			if(dwBrec == -1) // socket error
			{
				DbgLog::GetInstance().log("socket error receiving file data!\n");
				if(hFile != INVALID_HANDLE_VALUE)
					CloseHandle(hFile);
				return FALSE;
			}
			if(hFile != INVALID_HANDLE_VALUE)
			{
				DWORD dwWrote;
				WriteFile(hFile, g_fbuf, dwBrec, &dwWrote, NULL);
			}
			dwRec+=dwBrec;
		}
		rep = okreply;
		if(hFile != INVALID_HANDLE_VALUE)
			CloseHandle(hFile);
		if(SocketSendDataFixedSz((void*)rep, 2))
			ret = TRUE;
	}
	else
		DbgLog::GetInstance().log("socket error receiving file size!\n");
	return ret;
}

BOOL NandSvr::doExFormat(void)
{
	const char* rep = noreply;
	BOOL ret = FALSE;
	if(doFormatPartitions())
		rep = okreply;
	if(SocketSendDataFixedSz((void*)rep, 2))
		ret = TRUE;
	return ret;
}

BOOL NandSvr::doCompatFormat(void)
{
	const char* rep = noreply;
	BOOL ret = FALSE;
	if(doFormatCompatPartition())
		rep = okreply;
	if(SocketSendDataFixedSz((void*)rep, 2))
		ret = TRUE;
	return ret;
}

BOOL NandSvr::doMakeDir(char* path)
{
	BOOL ret = FALSE;
	const char* rep = noreply;
	if(CreateDirectory(path, NULL) != 0)
		rep = okreply;
	else
		DbgLog::GetInstance().log("error %d creating directory %s\n", GetLastError(), path);
	if(SocketSendDataFixedSz((void*)rep, 2))
		ret = TRUE;
	return ret;
}

BOOL NandSvr::doWriteFlash(void)
{
	BOOL ret = FALSE;
	DWORD dwSize = 0;
	if(SocketReceiveData(&dwSize, 4, NULL) != -1)
	{
		if(dwSize == Nand::Inst().nandInfo.dumpSize)
		{
			DWORD* wrk = Nand::Inst().setWorkerWrite();
// 			DbgLog::GetInstance().log("receiving 0x%x bytes flash data...\n", dwSize);
			if(SocketReceiveData(Nand::Inst().nandData, dwSize, wrk) != -1)
			{
				Nand::Inst().waitWorkerBusy();
				if(SocketSendDataFixedSz((void*)okreply, 2))
					ret = TRUE;
			}
			else
				DbgLog::GetInstance().log("socket error receiving file body!\n");
		}
		else
			DbgLog::GetInstance().log("error, sent size of 0x%x does not match real size of 0x%x!\n", dwSize, Nand::Inst().nandInfo.dumpSize);
	}
	else
		DbgLog::GetInstance().log("socket error receiving file size!\n");

	return ret;
}

BOOL NandSvr::doReadFlash(void)
{
	DWORD* wrk = Nand::Inst().setWorkerRead();
	return SocketSendDataFixedSz(Nand::Inst().nandData, Nand::Inst().nandInfo.dumpSize, wrk);
}

BOOL NandSvr::parseReadWriteBlock(char* cmd, DWORD* blk, DWORD* blnum, int cmdlen)
{
	// max peek size will be 0x10000
	if((cmdlen == 8)||(cmdlen == 4))
	{
		unsigned char tch;
		int i = 0;
		// first 4 bytes are block
		for(; i < 4; i++)
		{
			tch = myatox(cmd[i]);
			if(tch == 0xFF)
				return FALSE;
			*blk = *blk<<4;
			*blk |= tch&0xF;
		}
		// next 4 bytes are len
		for(; i < 8; i++)
		{
			tch = myatox(cmd[i]);
			if(tch == 0xFF)
				return FALSE;
			*blnum = *blnum<<4;
			*blnum |= tch&0xF;
		}
		return TRUE;
	}
// 	else
// 		DbgLog::GetInstance().log("command len %d is not 4 or 8\n", cmdlen);
	return FALSE;
}
// readblock args BLOCK NUMBLOCKS xxxxyyyy block, numblocks
BOOL NandSvr::doReadBlock(char* arg, int cmdlen)
{
	BOOL ret = FALSE;
	DWORD block = 0, blnum = 0;
	if(parseReadWriteBlock(arg, &block, &blnum, cmdlen))
	{
		DWORD osz;
		PBYTE buf;
// 		DbgLog::GetInstance().log("reading %x blocks at %x\n", blnum, block);
		buf = Nand::Inst().readBlocks(block, blnum, &osz);
		if(SocketSendDataFixedSz(buf, osz))
			ret = TRUE;
		if(buf != NULL)
			VirtualFree(buf,0, MEM_RELEASE );
	}
	return ret;
}

// writeblock args BLOCK NUMBLOCKS
BOOL NandSvr::doWriteBlock(char* arg, int cmdlen)
{
	BOOL ret = FALSE;
	DWORD block = 0, blnum = 0;
	if(parseReadWriteBlock(arg, &block, &blnum, cmdlen))
	{
		DWORD sz;
		if(SocketReceiveData(&sz, 4, NULL) == 4)
		{
			char* resp = noreply;
			PBYTE buf = (PBYTE)VirtualAlloc(0, sz, MEM_COMMIT, PAGE_READWRITE);
			if(buf != NULL)
			{
// 				DbgLog::GetInstance().log("receiving 0x%x bytes data\n", sz);
				if(SocketReceiveData(buf, sz, NULL))
				{
					if(sz == Nand::Inst().getBlockSize()*blnum)
					{
// 						DbgLog::GetInstance().log("writing %x blocks at %x\n", blnum, block);
						if(Nand::Inst().writeBlocks(buf, block, blnum))
							resp = okreply;
					}
				}
				VirtualFree(buf, 0, MEM_RELEASE);
			}
			if(SocketSendDataFixedSz(resp, 2))
				ret = TRUE;
		}
	}
	return ret;
}

BOOL NandSvr::doEraseBlock(char* arg, int cmdlen)
{
	BOOL ret = FALSE;
	DWORD block = 0, blnum = 0;
	if(parseReadWriteBlock(arg, &block, &blnum, cmdlen))
	{
		char* resp = noreply;
		if(Nand::Inst().eraseBlocks(block, blnum))
			resp = okreply;
		if(SocketSendDataFixedSz(resp, 2))
			ret = TRUE;
	}
	return ret;
}

BOOL NandSvr::doGetBootloaders(void)
{
	BOOL ret = FALSE;
	DWORD osz;
	PBYTE buf;
	buf = Nand::Inst().getBootloaders(&osz);
	if(SocketSendDataFixedSz(buf, osz))
		ret = TRUE;
	if(buf != NULL)
		VirtualFree(buf,0, MEM_RELEASE );
	return ret;
}

BOOL NandSvr::doWritePatch(void)
{
	BOOL ret = FALSE;
	DWORD sz;
	if(SocketReceiveData(&sz, 4, NULL) == 4)
	{
		char* resp = noreply;
		PBYTE buf = (PBYTE)VirtualAlloc(0, sz, MEM_COMMIT, PAGE_READWRITE);
		if(buf != NULL)
		{
			if(SocketReceiveData(buf, sz, NULL))
			{
				if(Nand::Inst().setPatchData(buf, sz))
					resp = okreply;
			}
			VirtualFree(buf, 0, MEM_RELEASE);
		}
		if(SocketSendDataFixedSz(resp, 2))
			ret = TRUE;
	}
	return ret;
}

BOOL NandSvr::doSendVer(void)
{
	return SocketSendDataFixedSz(&Nand::Inst().nandInfo.structVer, 4, NULL);
}

BOOL NandSvr::parseHvPePo(char* cmd, QWORD* addr, DWORD* len, int cmdlen)
{
	// max peek size will be 0x10000
	if(cmdlen == 24)
	{
		unsigned char tch;
		int i = 0;
		// first 8 bytes are address
		for(; i < 16; i++)
		{
			tch = myatox(cmd[i]);
			if(tch == 0xFF)
				return FALSE;
			*addr = *addr<<4;
			*addr |= tch&0xF;
		}
		// next 4 bytes are len
		for(; i < 24; i++)
		{
			tch = myatox(cmd[i]);
			if(tch == 0xFF)
				return FALSE;
			*len = *len<<4;
			*len |= tch&0xF;
		}
		if(*len <= 0x10000)
			return TRUE;
	}
// 	else
// 		DbgLog::GetInstance().log("command len %d is not 24\n", cmdlen);
	return FALSE;
}

BOOL NandSvr::doHvPeek(char* cmd, int cmdlen) // 8 byte address, 4 byte len
{
	BOOL ret = FALSE;
	QWORD src = 0;
	DWORD len = 0;
	if(parseHvPePo(cmd, &src, &len, cmdlen))
	{
		PBYTE buf = (PBYTE)XPhysicalAlloc(len, MAXULONG_PTR, 0, MEM_LARGE_PAGES|PAGE_READWRITE|PAGE_NOCACHE);
		if(buf != NULL)
		{
			if((Nand::Inst().nandInfo.structVer&0xFF) >= HVPEEK_BASIC)
			{
				QWORD dest = 0x8000000000000000ULL | ((DWORD)MmGetPhysicalAddress(buf)&0xFFFFFFFF);
				ZeroMemory(buf, len);
// 				DbgLog::GetInstance().log("hvpeek: %016I64x len: %08x ", src, len);
				HvxPeek(src, dest, len);
// 				DbgLog::GetInstance().log("done\n");
				if(SocketSendDataFixedSz(buf, len))
					ret = TRUE;
// 				else
// 					DbgLog::GetInstance().log("error sending hv data on socket!\n");
			}
			XPhysicalFree(buf);
		}
// 		else
// 			DbgLog::GetInstance().log("unable to allocate physical buffer for hvpeek! error %d\n", GetLastError());
	}
// 	else
// 		DbgLog::GetInstance().log("parse command failed\n");
	if(ret == FALSE)
	{
		if(SocketSendDataFixedSz(NULL, 0))
			ret = TRUE;
// 		else
// 			DbgLog::GetInstance().log("error sending hvpeek dummy data on socket!\n");
	}
	return ret;
}

BOOL NandSvr::doHvPoke(char* cmd, int cmdlen) // 8 byte address, 4 byte len, receive data first
{
	BOOL ret = FALSE;
	QWORD src = 0;
	DWORD len = 0;
	if(parseHvPePo(cmd, &src, &len, cmdlen))
	{
		PBYTE buf = (PBYTE)XPhysicalAlloc(len, MAXULONG_PTR, 0, MEM_LARGE_PAGES|PAGE_READWRITE|PAGE_NOCACHE);
		if(buf != NULL)
		{
			if((Nand::Inst().nandInfo.structVer&0xFF) >= HVPEEK_BASIC)
			{
				QWORD dest = 0x8000000000000000ULL | ((DWORD)MmGetPhysicalAddress(buf)&0xFFFFFFFF);
				ZeroMemory(buf, len);
				// get the data
				if(SocketReceiveDataFixedSz(buf, len, NULL) == len)
				{
// 					DbgLog::GetInstance().log("hvpoke: %016I64x len: %08x ", src, len);
					HvxPeek(dest, src, len);
// 					DbgLog::GetInstance().log("done\n");
					// send OK reply
					if(SocketSendDataFixedSz(okreply, 2))
						ret = TRUE;
// 					else
// 						DbgLog::GetInstance().log("error sending ok reply on socket!\n");
				}
			}
			XPhysicalFree(buf);
		}
// 		else
// 			DbgLog::GetInstance().log("unable to allocate physical buffer for hvpoke! error %d\n", GetLastError());
	}
	if(ret == FALSE)
	{
		// send NO reply
		if(SocketSendDataFixedSz(noreply, 2))
			ret = TRUE;
// 		else
// 			DbgLog::GetInstance().log("error sending hvpoke no reply on socket!\n");
	}
	return ret;
}

BOOL NandSvr::parsePePo(char* cmd, DWORD* addr, DWORD* len, int cmdlen)
{
	if(cmdlen == 16)
	{
		unsigned char tch;
		int i = 0;
		// first 8 bytes are address
		for(; i < 8; i++)
		{
			tch = myatox(cmd[i]);
			if(tch == 0xFF)
				return FALSE;
			*addr = *addr<<4;
			*addr |= tch&0xF;
		}
		// next 4 bytes are len
		for(; i < 16; i++)
		{
			tch = myatox(cmd[i]);
			if(tch == 0xFF)
				return FALSE;
			*len = *len<<4;
			*len |= tch&0xF;
		}
		if(*len <= 0x10000)
			return TRUE;
	}
	return FALSE;
}

BOOL NandSvr::doPeek(char* cmd, int cmdlen) // 4 byte address, 4 byte len
{
	BOOL ret = FALSE;
	DWORD src = 0;
	DWORD len = 0;
	if(parsePePo(cmd, &src, &len, cmdlen))
	{
		PBYTE buf = (BYTE*)VirtualAlloc(NULL, len, MEM_COMMIT|MEM_LARGE_PAGES, PAGE_READWRITE);
		if(buf != NULL)
		{
			ZeroMemory(buf, len);
// 			DbgLog::GetInstance().log("peek: %08x len: %08x ", src, len);
			memcpy(buf, (VOID*)src, len);
// 			DbgLog::GetInstance().log("done\n");
			if(SocketSendDataFixedSz(buf, len))
				ret = TRUE;
// 			else
// 				DbgLog::GetInstance().log("error sending peek data on socket!\n");
			VirtualFree(buf, 0, MEM_RELEASE);
		}
// 		else
// 			DbgLog::GetInstance().log("unable to allocate physical buffer for peek! error %d\n", GetLastError());
	}
	if(ret == FALSE)
	{
		if(SocketSendDataFixedSz(NULL, 0))
			ret = TRUE;
// 		else
// 			DbgLog::GetInstance().log("error sending peek dummy data on socket!\n");
	}
	return ret;
}

BOOL NandSvr::doPoke(char* cmd, int cmdlen) // 4 byte address, 4 byte len, receive data first
{
	BOOL ret = FALSE;
	DWORD dest = 0;
	DWORD len = 0;
	if(parsePePo(cmd, &dest, &len, cmdlen))
	{
		PBYTE buf = (BYTE*)VirtualAlloc(NULL, len, MEM_COMMIT|MEM_LARGE_PAGES, PAGE_READWRITE);
		if(buf != NULL)
		{
			ZeroMemory(buf, len);
			// get the data
			if(SocketReceiveDataFixedSz(buf, len, NULL) == len)
			{
// 				DbgLog::GetInstance().log("poke: %08x len: %08x ", dest, len);
				memcpy((VOID*)dest, buf, len);
// 				DbgLog::GetInstance().log("done\n");
				HvxPeek(dest, dest, len);
				// send OK reply
				if(SocketSendDataFixedSz(okreply, 2))
					ret = TRUE;
// 				else
// 					DbgLog::GetInstance().log("error sending ok reply on socket!\n");
			}
			VirtualFree(buf, 0, MEM_RELEASE);
		}
// 		else
// 			DbgLog::GetInstance().log("unable to allocate physical buffer for poke! error %d\n", GetLastError());
	}
	if(ret == FALSE)
	{
		// send NO reply
		if(SocketSendDataFixedSz(noreply, 2))
			ret = TRUE;
// 		else
// 			DbgLog::GetInstance().log("error sending poke no reply on socket!\n");
	}
	return ret;
}

// this is the connected data thread, handles commands and dispatches data in reply
DWORD NandSvr::NandsvrDatThread(void)
{
	char cmd[MAX_CMD_SZ];
	while(isConnected)
	{
		int len;
// 		DbgLog::GetInstance().log("waiting for command\n");
		memset(cmd, 0, MAX_CMD_SZ);
		len = SocketReceiveCommand(cmd);
		if((len != -1) && (len != 0))
		{
// 			DbgLog::GetInstance().log("cmd:%d:%s\n", len, cmd);
			int icmd = enumCmd(cmd);
			switch(icmd)
			{
				case CMD_QUIT:
// 					DbgLog::GetInstance().log("client issued QUIT command\n");
					endClient();
					break;
				case CMD_SHUTDOWN:
// 					DbgLog::GetInstance().log("client issued SHUTDOWN command\n");
					endClient();
					HalReturnToFirmware(HalPowerDownRoutine);
					break;
				case CMD_REBOOT:
// 					DbgLog::GetInstance().log("client issued REBOOT command\n");
					endClient();
					HalReturnToFirmware(HalRebootRoutine);
					break;
				case CMD_SMCREBOOT:
// 					DbgLog::GetInstance().log("client issued SMC REBOOT command\n");
					endClient();
					HalReturnToFirmware(HalResetSMCRoutine);
					break;
				case CMD_GETINFO:
					if(SocketSendDataFixedSz(&Nand::Inst().nandInfo, sizeof(NAND_INFO)) == FALSE)
						endClient();
					break;
				case CMD_GETBBLIST:
					if(doSendBbList() == FALSE)
						endClient();
					break;
				case CMD_GETFLASH:
					if(doReadFlash() == FALSE)
					{
						Nand::Inst().setWorkerIdle();
						endClient();
					}
					break;
				case CMD_WRITEFLASH:
					if(doWriteFlash() == FALSE)
					{
						Nand::Inst().setWorkerIdle();
						endClient();
					}
					break;
				case CMD_READBLOCK:
					if(doReadBlock(&cmd[CMD_LEN], len-CMD_LEN) == FALSE)
						endClient();
					break;
				case CMD_WRITEBLOCK:
					if(doWriteBlock(&cmd[CMD_LEN], len-CMD_LEN) == FALSE)
						endClient();
					break;
				case CMD_ERASEBLOCK:
					if(doEraseBlock(&cmd[CMD_LEN], len-CMD_LEN) == FALSE)
						endClient();
					break;
				case CMD_GETBOOTLOADERS:
					if(doGetBootloaders() == FALSE)
						endClient();
					break;
				case CMD_WRITEPATCH:
					if(doWritePatch() == FALSE)
						endClient();
					break;
				case CMD_MOUNTPATH:
					if(doMount(&cmd[CMD_LEN]) == FALSE)
						endClient();
					break;
				case CMD_UNMOUNTPATH:
					if(doUnmount(&cmd[CMD_LEN]) == FALSE)
						endClient();
					break;
				case CMD_GETFILE:
					if(doSendFile(&cmd[CMD_LEN]) == FALSE)
						endClient();
					break;
				case CMD_SENDFILE:
					if(doRecFile(&cmd[CMD_LEN]) == FALSE)
						endClient();
					break;
				case CMD_FORMATSYSEX:
					if(doExFormat() == FALSE)
						endClient();
					break;
				case CMD_FORMATCOMPAT:
					if(doCompatFormat() == FALSE)
						endClient();
					break;
				case CMD_MKDIR:
					if(doMakeDir(&cmd[CMD_LEN]) == FALSE)
						endClient();
					break;
				case CMD_HVPEEK: // 8 byte address, 4 byte len
					if(doHvPeek(&cmd[CMD_LEN], len-CMD_LEN) == FALSE)
						endClient();
					break;
				case CMD_HVPOKE: // 8 byte address, 4 byte len
					if(doHvPoke(&cmd[CMD_LEN], len-CMD_LEN) == FALSE)
						endClient();
					break;
				case CMD_PEEK: // 4 byte address, 4 byte len
					if(doPeek(&cmd[CMD_LEN], len-CMD_LEN) == FALSE)
						endClient();
					break;
				case CMD_POKE: // 4 byte address, 4 byte len
					if(doPoke(&cmd[CMD_LEN], len-CMD_LEN) == FALSE)
						endClient();
					break;
				case CMD_GET1BL:
					if(do1blDump() == FALSE)
						endClient();
					break;
				case CMD_HVDUMP:
					if(doHvDump() == FALSE)
						endClient();
					break;
				case CMD_GETVER:
					if(doSendVer() == FALSE)
						endClient();
					break;
				default:
// 					DbgLog::GetInstance().log("unhandled command %d:%s\n", icmd, cmd);
					break;
			}
		}
		else
		{
// 			DbgLog::GetInstance().log("Receive command failed or timed out! Ending client!\n");
			isConnected = FALSE;
		}
	}
	endClient();
	return 0;
}

