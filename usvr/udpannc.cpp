#include <xtl.h>
#include "xkelib.h"
#include "util.h"
#include "udpannc.h"
#include "logging.h"

#define CMD_MAGIC_BE		0x4E537672 // 'NSvr'
typedef struct _BCAST_MSG {
	DWORD magic;
	IN_ADDR xaddr;
}BCAST_MSG;

UdpAnnc::UdpAnnc()
{
	isRunning = FALSE;
	isAnnouncing = FALSE;
}

UdpAnnc::~UdpAnnc()
{
	if(isRunning)
		shutdown();
}

static DWORD AnncThreadStart(void* Param)
{
	UdpAnnc* This = (UdpAnnc*) Param;
	return This->AnncThread();
}

DWORD UdpAnnc::AnncThread(void)
{
	while(isRunning)
	{
		if(isAnnouncing)
		{
			if(XNetGetEthernetLinkStatus()&XNET_ETHERNET_LINK_ACTIVE)
			{
				BCAST_MSG smsg;
				XNADDR xna;
				DWORD dwRes = XNET_GET_XNADDR_PENDING;
				smsg.magic = CMD_MAGIC_BE;
				do{
					dwRes = XNetGetTitleXnAddr(&xna);
					Sleep(100);
				}while(dwRes == XNET_GET_XNADDR_PENDING); // todo: put this somewhere where it's affected by XNET connect/disconnect
				smsg.xaddr.S_un.S_addr = xna.ina.S_un.S_addr;
				//XNetInAddrToString( xna.ina, ip_address, 16 );
				//DbgLog::GetInstance().log("announce port %d\n", ANNC_PORT);
				sendto(sBcast, (const char*)&smsg, sizeof(BCAST_MSG), 0, (const sockaddr*)&sBcastAddr, sizeof(SOCKADDR_IN));
			}
		}
		Sleep(ANNC_DELAY);
	}
	return 0;
}

BOOL UdpAnnc::startup(void)
{
	DWORD thid;
	HANDLE hTh;
	BOOL btemp = TRUE;
	sBcast = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sBcast == INVALID_SOCKET)
	{
		DbgLog::GetInstance().log("UdpAnnc failed to acquire socket, error %d\n", WSAGetLastError());
		return FALSE;
	}
	if(setSockSecurity(sBcast) == FALSE)
		return FALSE;
	if(setsockopt(sBcast, SOL_SOCKET, SO_BROADCAST, (PCSTR)&btemp, sizeof(BOOL) ) != 0 )
	{
		DbgLog::GetInstance().log("UdpAnnc failed to set debug send socket to SO_BROADCAST, error %d\n", WSAGetLastError());
		return FALSE;
	}

	sBcastAddr.sin_family = AF_INET;
	sBcastAddr.sin_addr.s_addr = INADDR_BROADCAST;
	sBcastAddr.sin_port = htons(ANNC_PORT);

	hTh = CreateThread(NULL, 0x10000, (LPTHREAD_START_ROUTINE)AnncThreadStart, (LPVOID)this, CREATE_SUSPENDED, &thid);
	if(hTh == NULL)
	{
		DbgLog::GetInstance().log("Failed to create AnncThread, error %d\n", GetLastError());
		return FALSE;
	}
	isAnnouncing = TRUE;
	XSetThreadProcessor(hTh, 1);
	ResumeThread(hTh);
	isRunning = TRUE;
	return TRUE;
}

void UdpAnnc::shutdown(void)
{
	isRunning = FALSE;
	Sleep(ANNC_DELAY);
	closesocket(sBcast);
}
