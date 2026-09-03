#include <xtl.h>
#include "xkelib.h"
#include "util.h"
#include "svr.h"
#include "nsvrExp.h"


Svr::Svr()
{
	status = FALSE;
}

DWORD Svr::getStatus(void)
{
	DWORD ret = 0;
	if(status)
	{
		ret |= NSVR_BIT_READY;
		if(udpannc.getAnnouncing())
			ret |= NSVR_BIT_UDPBCAST;
		if(nandsvr.getRunning())
			ret |= NSVR_BIT_RUNNING;
		if(nandsvr.getConnected())
			ret |= NSVR_BIT_CONNECTED;
	}
	return ret;
}

BOOL Svr::startup(void)
{
	if(nandsvr.startup(&udpannc.isAnnouncing))
	{
		if(udpannc.startup())
			status = TRUE;
	}
	return status;
}

void Svr::shutdown(void)
{
	udpannc.shutdown();
	nandsvr.shutdown();
	status = FALSE;
}
