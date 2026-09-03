#pragma once

#define ANNC_PORT	48
#define ANNC_DELAY	1000 // in milliseconds

class UdpAnnc
{
public:	
	UdpAnnc();
	~UdpAnnc();
	BOOL isAnnouncing; // this gets handled internally, by the client thread and this class
	BOOL startup(void);
	void shutdown(void);
	DWORD AnncThread(void);
	BOOL getAnnouncing(void){if(isRunning&&isAnnouncing)return TRUE; return FALSE;}
private:
	SOCKET sBcast;
	SOCKADDR_IN sBcastAddr;
	BOOL isRunning;
};
