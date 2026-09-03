#ifndef _XAMEXT_DEFINES_H
#define _XAMEXT_DEFINES_H

#include "kernel.h"
// DWORD GetCurrentProcessorNumber()
enum _XNCALLER_TYPE {
	XNCALLER_INVALID = 0x0,
	XNCALLER_TITLE = 0x1,
	XNCALLER_SYSAPP = 0x2,
	XNCALLER_XBDM = 0x3,
	XNCALLER_TEST = 0x4,
	NUM_XNCALLER_TYPES = 0x4,
};

#define XAMLOADERLAUNCHTITLEEX_ORD	421
typedef NTSTATUS (*XAMLOADERLAUNCHTITLEEX)(char const * szLaunchPath, char const * szMountPath, char const * szCmdLine, DWORD dwFlags);

#ifdef __cplusplus
extern "C" {
#endif
	DWORD XamContentOpenFile(
		IN      DWORD                       dwUserIndex,
		IN      LPCSTR                      pszRootName,
		IN      LPCSTR                      pszFileName,
		IN      DWORD                       dwContentFlags, // 0x4000043
		IN      DWORD                       dwFileCacheSize,
		OUT     PDWORD                      pdwLicenseMask      OPTIONAL,
		IN OUT  PXOVERLAPPED                pOverlapped         OPTIONAL
		);
	DWORD XamContentFlush(
		IN      LPCSTR                      pszRootName,
		IN      PXOVERLAPPED                pOverlapped         OPTIONAL
		);
	DWORD XamContentClose(
		IN      LPCSTR                      pszRootName,
		IN OUT  PXOVERLAPPED                pOverlapped         OPTIONAL
		);
// from xnet lib
	int NetDll_XNetStartup_Override(enum _XNCALLER_TYPE CallerType, XNetStartupParams *pxnsp);

	int NetDll_XNetStartupEx(enum _XNCALLER_TYPE, XNetStartupParams* xnsp, DWORD versionReq); // ordinal 80
	int NetDll_WSAStartupEx(enum _XNCALLER_TYPE, DWORD threadTypeunk, LPWSADATA wsad,  DWORD versionReq); // ordinal 36
	int NetDll_WSACleanup(enum _XNCALLER_TYPE);
	SOCKET NetDll_socket(enum _XNCALLER_TYPE, int af, int type, int protocol);
	int NetDll_setsockopt(enum _XNCALLER_TYPE, SOCKET s, int level, int optname, const char FAR * optval, int optlen);
	int NetDll_closesocket(enum _XNCALLER_TYPE, SOCKET s);
	int NetDll_bind(enum _XNCALLER_TYPE, SOCKET s, const struct sockaddr FAR * name, int namelen);
	int NetDll_send(enum _XNCALLER_TYPE, SOCKET s, const char FAR * buf, int len, int flags);
	int NetDll_recv(enum _XNCALLER_TYPE, SOCKET s, char FAR * buf, int len, int flags);
	int NetDll_listen(enum _XNCALLER_TYPE, SOCKET s, int backlog);
	SOCKET NetDll_accept(enum _XNCALLER_TYPE, SOCKET s, struct sockaddr FAR * addr, int FAR * addrlen);
	int NetDll_select(enum _XNCALLER_TYPE, int nfds, fd_set FAR * readfds, fd_set FAR * writefds, fd_set FAR *exceptfds, const struct timeval FAR * timeout);
	int NetDll_connect(enum _XNCALLER_TYPE, SOCKET s, const struct sockaddr FAR * name, int namelen);
	int NetDll_getpeername(enum _XNCALLER_TYPE, SOCKET s, struct sockaddr FAR * name, int FAR * namelen);
	int NetDll_getsockname(enum _XNCALLER_TYPE, SOCKET s, struct sockaddr FAR * name, int FAR * namelen);
	int NetDll_ioctlsocket(enum _XNCALLER_TYPE, SOCKET s, long cmd, u_long FAR * argp);
	DWORD NetDll_XNetGetEthernetLinkStatus(enum _XNCALLER_TYPE);
#ifdef __cplusplus
}
#endif

#define CUR_VER (((XboxKrnlVersion->Major&0xF)<<28) | ((XboxKrnlVersion->Minor)<<24) | ((XboxKrnlVersion->Build &0xFFFF)<<8) | ((XboxKrnlVersion->Qfe&0xFF)))

//#define WSAStartup(x,a)			NetDll_WSAStartupEx(XNCALLER_SYSAPP, 2, a, CUR_VER)
//#define XNetStartup(p)			NetDll_XNetStartupEx(XNCALLER_SYSAPP, p, CUR_VER)
//#define WSACleanup()				NetDll_WSACleanup(XNCALLER_SYSAPP)
//#define socket(a,t,p)				NetDll_socket(XNCALLER_SYSAPP, a, t ,p)
//#define setsockopt(s,lv,o,v,l)	NetDll_setsockopt(XNCALLER_SYSAPP, s, lv, o, v, l)
//#define closesocket(s)			NetDll_closesocket(XNCALLER_SYSAPP, s)

//#define bind(s,n,l)				NetDll_bind(XNCALLER_SYSAPP, s, n, l)
//#define send(s,b,l,f)				NetDll_send(XNCALLER_SYSAPP, s, b, l, f)
//#define recv(s,b,l,f)				NetDll_recv(XNCALLER_SYSAPP, s, b, l, f)
//#define listen(s,b)				NetDll_listen(XNCALLER_SYSAPP, s, b)
//#define accept(s,a,l)				NetDll_accept(XNCALLER_SYSAPP, s, a, l)
//#define select(n,r,w,e,t)			NetDll_select(XNCALLER_SYSAPP, n, r, w, e, t)
//#define connect(s,n,l)			NetDll_connect(XNCALLER_SYSAPP, s, n, l)
//#define getpeername(s,n,l)		NetDll_getpeername(XNCALLER_SYSAPP, s, n, l)
//#define getsockname(s,n,l)		NetDll_getsockname(XNCALLER_SYSAPP, s, n, l)
//#define ioctlsocket(s,c,a)		NetDll_ioctlsocket(XNCALLER_SYSAPP, s, c, a)

//#define XNetGetEthernetLinkStatus() NetDll_XNetGetEthernetLinkStatus(XNCALLER_SYSAPP)

#endif	//_XAMEXT_DEFINES_H
