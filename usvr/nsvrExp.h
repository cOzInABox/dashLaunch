#ifndef _NSVR_EXP_DEF
#define _NSVR_EXP_DEF

typedef BOOL (*USVR_RET_BOOL)(void);
typedef VOID (*USVR_RET_VOID)(void);
typedef DWORD (*USVR_RET_DWORD)(void);
typedef BOOL (*USVR_GETSET_PATCH)(void* buf, int blen);

// export ordinals
#define USVR_NAME_ORD		1
#define USVR_START_ORD		2
#define USVR_STOP_ORD		3
#define USVR_STAT_ORD		4
#define USVR_CONTYP_ORD		5
#define USVR_GETPATCH_ORD	6
#define USVR_SETPATCH_ORD	7

// status bits
#define NSVR_BIT_READY		(1)		// it's loaded and ready
#define NSVR_BIT_UDPBCAST	(1<<1)	// UDP is broadcasting
#define NSVR_BIT_RUNNING	(1<<2)	// main server thread is waiting for clients
#define NSVR_BIT_CONNECTED	(1<<3)	// a client is connected

//USVR_RET_BOOL usvrStartup = (USVR_RET_BOOL)NULL;
//USVR_RET_VOID usvrShutdown = (USVR_RET_VOID)NULL;
//USVR_RET_DWORD usvrStatus = (USVR_RET_DWORD)NULL;
//USVR_RET_DWORD usvrGetConType = (USVR_RET_DWORD)NULL;
//USVR_GETSET_PATCH usvrGetPatch = (USVR_GETSET_PATCH)NULL;
//USVR_GETSET_PATCH usvrSetPatch = (USVR_GETSET_PATCH)NULL;

//HANDLE hMod = INVALID_HANDLE_VALUE;
//XexGetModuleHandle(NULL, &hMod);
// usvrStartup = (USVR_RET_BOOL)GetProcAddress(hMod, (LPCSTR)USVR_START_ORD);
// usvrShutdown = (USVR_RET_VOID)GetProcAddress(hMod, (LPCSTR)USVR_STOP_ORD);
// usvrStatus = (USVR_RET_DWORD)GetProcAddress(hMod, (LPCSTR)USVR_STAT_ORD);
// usvrGetPatch = (USVR_GETSET_PATCH)GetProcAddress(hMod, (LPCSTR)USVR_GETPATCH_ORD);
// usvrSetPatch = (USVR_GETSET_PATCH)GetProcAddress(hMod, (LPCSTR)USVR_SETPATCH_ORD);


#endif // _NSVR_EXP_DEF
