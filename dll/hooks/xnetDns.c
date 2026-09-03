#include "_hook_includes.h"

typedef DWORD (*XNETDNSFUN)(DWORD callerType, char const* pszHost, HANDLE hEvent, XNDNS** ppXNDns);
#define XNETDNS_ORD						0x43  // 67  NetDll_XNetDnsLookup

// fsd uses catalog.xboxlive.com
#define DNS_BLOCK_WEAKLIST	16
BlockRule wRules[] = {
	{"xemacs", "xboxlive.com"},
	{"xeas", "xboxlive.com"},
	{"xetgs", "xboxlive.com"},
	{"xexds", "xboxlive.com"},
	{"piflc", "xboxlive.com"},
	{"siflc", "xboxlive.com"},
	{"msac", "xboxlive.com"},
	{"xlink", "xboxlive.com"},
	{"xuacs", "xboxlive.com"},
	{"sts", "xboxlive.com"},
	{"xam", "xboxlive.com"},
	{"notice", "xbox.com"},
	{"macs", "xbox.com"},
	{"rad", "msn.com"},
	{NULL, "passport.net"},
	{"localhost", NULL},
};

//ssl.bing.com
//http://download.xbox.com/content/xna/assets/{1}_World/xboxboxart.jpg
//http://download.xbox.com/content/images/{1}/{2}/{3}
//rad.msn.com

#define DNS_BLOCK_STRONGLIST	8
BlockRule sRules[] = {
	{NULL, "xboxlive.com"},
	{NULL, "xbox.com"},
	{NULL, "nsatc.net"},
	{NULL, "microsoft.com"},
	{NULL, "passport.net"},
	{NULL, "bing.net"},
	{NULL, "msn.com"},
	{"localhost", NULL},
};

DnsBlocks DnsRules = {
	DNS_BLOCK_STRONGLIST, sRules
};


BOOL isDnsBlocked(const char* dnsReq)
{
	int i;
#ifdef DEBUG_XNETDNS_OUT_RULES
	DbgPrint("num rules: %d\n",DnsRules.numRules);
#endif
	for(i = 0; i < DnsRules.numRules; i++)
	{
		BOOL startBlock = FALSE;
		BOOL endBlock = FALSE;
		int len;
#ifdef DEBUG_XNETDNS_OUT_RULES
		DbgPrint("\n%d checking %x - %x\n", i, bRules[i].startsWith, bRules[i].endsWith);
#endif
		// compare startswith
		if(DnsRules.bRules[i].startsWith != NULL)
		{
			len = strlen(DnsRules.bRules[i].startsWith);
#ifdef DEBUG_XNETDNS_OUT_RULES
			DbgPrint("%d check rule sw: '%s'\n", i, bRules[i].startsWith);
#endif
			if(strnicmp(DnsRules.bRules[i].startsWith, dnsReq, len) == 0)
			{
#ifdef DEBUG_XNETDNS_OUT_RULES
				DbgPrint("%d sw block: '%s'\n", i, bRules[i].startsWith);
#endif
				startBlock = TRUE;
			}
		}
		else
			startBlock = TRUE;

		// compare endswith
		if(DnsRules.bRules[i].endsWith != NULL)
		{
			int reqLen = strlen(dnsReq);
#ifdef DEBUG_XNETDNS_OUT_RULES
			DbgPrint("%d check rule ew: '%s'\n", i, bRules[i].endsWith);
#endif
			len = strlen(DnsRules.bRules[i].endsWith);
			if(reqLen >= len)
			{
				if(strnicmp(DnsRules.bRules[i].endsWith, &dnsReq[reqLen-len], len) == 0)
				{
#ifdef DEBUG_XNETDNS_OUT_RULES
					DbgPrint("%d ew block: '%s' of '%s'\n", i, bRules[i].endsWith, &dnsReq[reqLen-len]);
#endif
					endBlock = TRUE;
				}
			}
		}
		else
			endBlock = TRUE;

		if((startBlock == TRUE)&&(endBlock == TRUE))
			return TRUE;
	}
	return FALSE;
}

void __declspec(naked) xnetDnsSaveVar(void)
{
	__asm{
		li r3, XNETDNS_VAL
		nop
		nop
		nop
		nop
		nop
		nop
		blr
	}
}
XNDNS dummyDnsReply;
XNETDNSFUN xnetDnsSave = (XNETDNSFUN)xnetDnsSaveVar;
DWORD xnetDnsHook(DWORD callerType, char const* pszHost, HANDLE hEvent, XNDNS** ppXNDns)
{
#ifdef DEBUG_XNETDNS_OUT
	DWORD ret = 0;
#endif
	if((pszHost != NULL) && (getOpt(OPT_LIVE_BLOCK)))
	{
		if(isDnsBlocked(pszHost))
		{
#ifdef DEBUG_XNETDNS_OUT
			dbgPrintFake("xndns: '%s' (req redir) type %d hEvent %08x\n", pszHost, callerType, hEvent);
#endif
			dummyDnsReply.iStatus = WSAHOST_NOT_FOUND;
			dummyDnsReply.cina = 0;
			dummyDnsReply.aina[0].S_un.S_addr = INADDR_LOOPBACK;
			ppXNDns[0] = &dummyDnsReply;

			//dummyDnsReply.iStatus = 0;
			//dummyDnsReply.cina = 1;
			//dummyDnsReply.aina[0].S_un.S_addr = INADDR_LOOPBACK;
			//ppXNDns[0] = &dummyDnsReply;
			if(hEvent != NULL)
				WSASetEvent(hEvent);
			return S_OK;
		}
	}
#ifdef DEBUG_XNETDNS_OUT
	ret = xnetDnsSave(callerType, pszHost, hEvent, ppXNDns);
	dbgPrintFake("xndns: '%s' (req allow) type %d hEvent %08x return: %08x\n", pszHost, callerType, hEvent, ret);
	return ret;
#else
	return xnetDnsSave(callerType, pszHost, hEvent, ppXNDns);
#endif
}

void XNetDnsSetRules(BOOL useStrong)
{
	if(useStrong)
	{
#ifdef DEBUG_XNETDNS_OUT
		DbgPrint("using strong live block rules\n");
#endif
		DnsRules.numRules = DNS_BLOCK_STRONGLIST;
		DnsRules.bRules = sRules;
	}
	else
	{
#ifdef DEBUG_XNETDNS_OUT
		DbgPrint("using weak live block rules\n");
#endif
		DnsRules.numRules = DNS_BLOCK_WEAKLIST;
		DnsRules.bRules = wRules;
	}
}

static DWORD xnetDnsOld[4];
static BOOL isXnetHooked = FALSE;
//static DWORD setLaunchOld = 0;
void XNetDnsHook(void)
{
	if(!isXnetHooked)
	{
		hookFunctionStartOrd(MODULE_XAM, XNETDNS_ORD, (PDWORD)xnetDnsSaveVar, xnetDnsOld, (DWORD)xnetDnsHook);
		isXnetHooked = TRUE;
	}
}

void XNetDnsUnhook(void)
{
	if(isXnetHooked)
	{
		unhookFunctionStartOrd(MODULE_XAM, XNETDNS_ORD, xnetDnsOld);
		isXnetHooked = FALSE;
	}
}
