#pragma once

#include "udpannc.h"
#include "nandsvr.h"

class Svr
{
public:	
	static Svr& GetInstance(){static Svr singleton; return singleton;}
	BOOL startup(void);
	void shutdown(void);
	DWORD getStatus(void);
private:
	BOOL status;
	class NandSvr nandsvr;
	class UdpAnnc udpannc;

	Svr();
	~Svr() {}
	Svr(const Svr&);                 // Prevent copy-construction
	Svr& operator=(const Svr&);      // Prevent assignment
};
