#include "_hook_includes.h"
#include <xauth.h>
// LIVE Notifications
// XOnlineStartup
//XUserCheckPrivilege
// XamUserCheckPrivilege 530/0x212

//enum _XPRIVILEGE_TYPE {
//	XPRIVILEGE_SHARE_CONTENT_OUTSIDE_LIVE = 0xD3,
//	XPRIVILEGE_UNSAFE_PROGRAMMING = 0xD4,
//	XPRIVILEGE_CONTENT_AUTHOR = 0xDE, // The gamer is allowed to author content via the XNA Creators Club. 
//	XPRIVILEGE_VIDEO_COMMUNICATIONS_FRIENDS_ONLY = 0xEA,
//	XPRIVILEGE_VIDEO_COMMUNICATIONS = 0xEB,
// XPRIVILEGE_UNSAFE_CONTENT = 0xED,
//	XPRIVILEGE_TRADE_CONTENT = 0xEE,
//	XPRIVILEGE_PRESENCE_FRIENDS_ONLY = 0xF3,
//	XPRIVILEGE_PRESENCE = 0xF4,
//	XPRIVILEGE_PURCHASE_CONTENT = 0xF5,
//	XPRIVILEGE_USER_CREATED_CONTENT_FRIENDS_ONLY = 0xF6,
//	XPRIVILEGE_USER_CREATED_CONTENT = 0xF7,
//	XPRIVILEGE_PROFILE_VIEWING_FRIENDS_ONLY = 0xF8,
//	XPRIVILEGE_PROFILE_VIEWING = 0xF9,
//	XPRIVILEGE_COMMUNICATIONS_FRIENDS_ONLY = 0xFB,
//	XPRIVILEGE_COMMUNICATIONS = 0xFC,
//	XPRIVILEGE_MULTIPLAYER_SESSIONS = 0xFE,
// content restriction = 0xED
// network storage 0xD1
// gold upsell 0xFD
// XPRIVILEGE_PREMIUM_VIDEO = 0xE0
//	0xD6 0xDC
//};
#define XPRIVILEGE_UNSAFE_CONTENT	237
#define XPRIVILEGE_UNK_214			214 // XPRIVILEGE_PREMIUM_CONTENT
#define XPRIVILEGE_UNK_220			220 // XPRIVILEGE_SOCIAL_NETWORK_SHARING
#define XPRIVILEGE_PREMIUM_VIDEO	224 // access to Netflix app

//#define XUSER_GET_SIGNIN_INFO_ONLINE_XUID_ONLY		0x00000001
//#define XUSER_GET_SIGNIN_INFO_OFFLINE_XUID_ONLY		0x00000002

//enum _XAMUSER_SIGNIN_STATE {
//	eXamUserSigninState_NotSignedIn = 0x0,
//	eXamUserSigninState_SignedInLocally = 0x1,
//	eXamUserSigninState_SignedInToLive = 0x2,
//};
//
//enum _XUSER_SIGNIN_STATE {
//	eXUserSigninState_NotSignedIn = 0x0,
//	eXUserSigninState_SignedInLocally = 0x1,
//	eXUserSigninState_SignedInToLive = 0x2,
//};
//
//#define XUSER_INFO_FLAG_LIVE_ENABLED					0x00000001
//#define XUSER_INFO_FLAG_GUEST							0x00000002
//typedef struct _XUSER_SIGNIN_INFO { 
//	QWORD xuid; // 0x0 sz:0x8
//	DWORD dwInfoFlags; // 0x8 sz:0x4
//	enum _XUSER_SIGNIN_STATE UserSigninState; // 0xC sz:0x4
//	DWORD dwGuestNumber; // 0x10 sz:0x4
//	DWORD dwSponsorUserIndex; // 0x14 sz:0x4
//	char szUserName[0x10]; // 0x18 sz:0x10
//} XUSER_SIGNIN_INFO, *PXUSER_SIGNIN_INFO; // size 40
////C_ASSERT(sizeof(XUSER_SIGNIN_INFO) == 0x28);
//
////0x210/528
//DWORD XamUserGetSigninState(DWORD userIndex);
////551
//NTSTATUS XamUserGetSigninInfo(DWORD userIndex, DWORD flags, PXUSER_SIGNIN_INFO xSigningInfo);

// 	else if(type == IMAGEFLAGS)
// if (flags & 0x40000000) printf("\tRevocation Check Optional\n");
// if (flags & 0x80000000) printf("\tRevocation Check Required\n");
//PXEX_SECURITY_INFO SecurityInfo;//0x10

typedef DWORD (*XAMUSERGETAGEGROUPFUN)(DWORD dwUserIndex, XUSER_AGE_GROUP *pAgeGroup,	PXOVERLAPPED pXOverlapped); // XUserGetSigninInfo
XAMUSERGETAGEGROUPFUN XamUserGetAgeGroupSave;

DWORD XamUserGetAgeGroupHook(DWORD dwUserIndex, XUSER_AGE_GROUP *pAgeGroup,	PXOVERLAPPED pXOverlapped)
{
	if(pXOverlapped == NULL)
	{
		DWORD ret = XamUserGetAgeGroupSave(dwUserIndex, pAgeGroup, pXOverlapped);
		if(pAgeGroup != NULL)
		{
#ifdef DEBUG_SIGNINSTATE_OUT
			DbgPrint("age group idx %x ovl %x ret %x group %d\n", dwUserIndex, pXOverlapped, ret, pAgeGroup[0]);
#endif
			*pAgeGroup = XAGEGROUP_ADULT;
			if(ret != ERROR_SUCCESS)
				ret = ERROR_SUCCESS;
		}
#ifdef DEBUG_SIGNINSTATE_OUT
		else
			DbgPrint("age group, pointer NULL! idx %x ovl %x ret %x\n", dwUserIndex, pXOverlapped, ret);
#endif
		return ret;
	}
#ifdef DEBUG_SIGNINSTATE_OUT
	DbgPrint("age group hook does not handle xoverlapped yet!\n");
#endif
	return XamUserGetAgeGroupSave(dwUserIndex, pAgeGroup, pXOverlapped);
}

typedef DWORD (*XAMUSERGETMEMTYPEXUIDFUN)(XUID inx); // XamUserGetMembershipTypeFromXUID
XAMUSERGETMEMTYPEXUIDFUN XamUserGetMembershipTypeFromXUIDSave;

DWORD XamUserGetMembershipTypeFromXUIDHook(XUID inx)
{
#ifdef DEBUG_SIGNINSTATE_OUT
	DWORD ret = XamUserGetMembershipTypeFromXUIDSave(inx);
	DbgPrint("type by xuid: %016I64x ret %d\n", inx, ret);
#endif
	return 6;
}

typedef DWORD (*XAMUSERGETMEMTYPEFUN)(DWORD idx); // XamUserGetMembershipType
XAMUSERGETMEMTYPEFUN XamUserGetMembershipTypeSave;
#define XAM_USER_GET_MEMB_TYPE_ORD		539 // 0x21B XamUserGetMembershipType
DWORD XamUserGetMembershipTypeHook(DWORD idx)
{
#ifdef DEBUG_SIGNINSTATE_OUT
	DWORD ret = XamUserGetMembershipTypeSave(idx);
	DbgPrint("type by idx: %x ret %d\n", idx, ret);
#endif
	return 6;
}


typedef NTSTATUS (*XAMUSERGETSIGNININFOFUN)(DWORD userIndex, DWORD flags, PXUSER_SIGNIN_INFO xSigningInfo); // XUserGetSigninInfo
typedef XAMUSER_SIGNIN_STATE (*XAMUSERGETSIGNINSTATEFUN)(DWORD userIndex); // XUserGetSigninState
typedef HRESULT (*XAUTHSTARTUPFUN)(XAUTH_SETTINGS *Settings); // XampXAuthStartup
typedef DWORD (*XAMUSERCHECKPRIVFUN)(DWORD dwUserIndex, XPRIVILEGE_TYPE PrivilegeType,PBOOL pfResult);

XAUTHSTARTUPFUN XampXAuthStartupSave;
HRESULT XampXAuthStartupHook(XAUTH_SETTINGS *Settings)
{
#ifdef DEBUG_SIGNINSTATE_OUT
	DbgPrint("xauth startup flags: %x\n", Settings->Flags);
#endif
	Settings->Flags |= XAUTH_FLAG_BYPASS_SECURITY;
	return XampXAuthStartupSave(Settings);
}

XAMUSERCHECKPRIVFUN XamUserCheckPrivilegeSave;
DWORD XamUserCheckPrivilegeHook(DWORD dwUserIndex, XPRIVILEGE_TYPE PrivilegeType, PBOOL pfResult)
{
	DWORD ret = XamUserCheckPrivilegeSave(dwUserIndex, PrivilegeType, pfResult);
#ifdef DEBUG_SIGNINSTATE_OUT
	DbgPrint("ret %x check priv %d idx: %x result: %x (patched)\n", ret, PrivilegeType, dwUserIndex, pfResult[0]);
#endif
	if(ret != 0)
	{
		switch(PrivilegeType)
		{
			case XPRIVILEGE_MULTIPLAYER_SESSIONS:
			case XPRIVILEGE_COMMUNICATIONS:
			case XPRIVILEGE_PROFILE_VIEWING:
			case XPRIVILEGE_USER_CREATED_CONTENT:
			case XPRIVILEGE_PURCHASE_CONTENT:
			case XPRIVILEGE_PRESENCE:
			case XPRIVILEGE_TRADE_CONTENT:
			case XPRIVILEGE_VIDEO_COMMUNICATIONS:
			case XPRIVILEGE_CONTENT_AUTHOR:
			case XPRIVILEGE_UNSAFE_PROGRAMMING:
			case XPRIVILEGE_SHARE_CONTENT_OUTSIDE_LIVE:
			case XPRIVILEGE_UNSAFE_CONTENT:
			case XPRIVILEGE_UNK_214: // XPRIVILEGE_PREMIUM_CONTENT
			case XPRIVILEGE_UNK_220: // XPRIVILEGE_SOCIAL_NETWORK_SHARING
			case XPRIVILEGE_PREMIUM_VIDEO:
				pfResult[0] = 1;
#ifdef DEBUG_SIGNINSTATE_OUT
				DbgPrint("check priv %d idx: %x result: %x (patched)\n", PrivilegeType, dwUserIndex, pfResult[0]);
#endif
				break;
			default:
#ifdef DEBUG_SIGNINSTATE_OUT
				DbgPrint("check priv %d idx: %x result: %x\n", PrivilegeType, dwUserIndex, pfResult[0]);
#endif
				break;
		}
		ret = ERROR_SUCCESS;
	}
#ifdef DEBUG_SIGNINSTATE_OUT
	else
		DbgPrint("check priv %d idx: %x failed, ret: %08x\n", PrivilegeType, dwUserIndex, ret);
#endif
	return ret;
}
 
//XAMUSERGETSIGNINSTATEFUN XamUserGetSigninStateSave;
//XAMUSER_SIGNIN_STATE XamUserGetSigninStateHook(DWORD userIndex)
//{
//	DWORD ret = XamUserGetSigninStateSave(userIndex);
//	DbgPrint("signinState idx:%x ret: %x\n", userIndex, ret);
//	if(ret == eXamUserSigninState_SignedInLocally)
//		ret = eXamUserSigninState_SignedInToLive;
//	return (XAMUSER_SIGNIN_STATE)ret;
//}
//
//XAMUSERGETSIGNININFOFUN XamUserGetSigninInfoSave;
//NTSTATUS XamUserGetSigninInfoHook(DWORD userIndex, DWORD flags, PXUSER_SIGNIN_INFO xSigningInfo)
//{
//	NTSTATUS ret;
//	ret = XamUserGetSigninInfoSave(userIndex, flags, xSigningInfo);
//	DbgPrint("signinInfo idx:%x flag: %08x ret: %x\n", userIndex, flags, ret);
//	if(xSigningInfo != NULL)
//	{
//		DbgPrint("xuid              : 0x%016I64X\n", xSigningInfo->xuid);
//		DbgPrint("dwInfoFlags       : 0x%08X\n", xSigningInfo->dwInfoFlags); // XUSER_INFO_FLAG_LIVE_ENABLED
//		DbgPrint("UserSigninState   : 0x%08X\n", xSigningInfo->UserSigninState);
//		DbgPrint("dwGuestNumber     : 0x%08X\n", xSigningInfo->dwGuestNumber);
//		DbgPrint("dwSponsorUserIndex: 0x%08X\n", xSigningInfo->dwSponsorUserIndex);
//		if(xSigningInfo->szUserName[0] != 0)
//			DbgPrint("szUserName        : %s\n", xSigningInfo->szUserName);
//		if(xSigningInfo->UserSigninState == eXUserSigninState_SignedInLocally)
//		{
//			xSigningInfo->UserSigninState = eXUserSigninState_SignedInToLive;
//			DbgPrint("UserSigninState   : 0x%08X (patched)\n", xSigningInfo->UserSigninState);
//		}
//	}
//	return ret;
//}
void __declspec(naked) XamUserGetSigninStateSaveVar(void)
{
	__asm{
		li r3, XAMSIGNINSTATE_VAL
		nop
		nop
		nop
		nop
		nop
		nop
		blr
	}
}

XAMUSERGETSIGNINSTATEFUN XamUserGetSigninStateSave = (XAMUSERGETSIGNINSTATEFUN)XamUserGetSigninStateSaveVar;
XAMUSER_SIGNIN_STATE XamUserGetSigninStateHook(DWORD userIndex)
{ 
	DWORD ret = XamUserGetSigninStateSave(userIndex);
//#ifdef DEBUG_SIGNINSTATE_OUT
//	DbgPrint("signinState idx:%x ret: %x\n", userIndex, ret);
//#endif
	if(ret == eXamUserSigninState_SignedInLocally)
		ret = eXamUserSigninState_SignedInToLive;
	return (XAMUSER_SIGNIN_STATE)ret;
}

void __declspec(naked) XamUserGetSigninInfoSaveVar(void)
{
	__asm{
		li r3, XAMSIGNINGINFO_VAL
		nop
		nop
		nop
		nop
		nop
		nop
		blr
	}
}

XAMUSERGETSIGNININFOFUN XamUserGetSigninInfoSave = (XAMUSERGETSIGNININFOFUN)XamUserGetSigninInfoSaveVar;
NTSTATUS XamUserGetSigninInfoHook(DWORD userIndex, DWORD flags, PXUSER_SIGNIN_INFO xSigningInfo)
{
	NTSTATUS ret;
	ret = XamUserGetSigninInfoSave(userIndex, flags, xSigningInfo);
#ifdef DEBUG_SIGNINSTATE_OUT
	DbgPrint("signinInfo idx:%x flag: %08x ret: %x\n", userIndex, flags, ret);
#endif
	if(xSigningInfo != NULL)
	{
#ifdef DEBUG_SIGNINSTATE_OUT
		DbgPrint("xuid              : 0x%016I64X\n", xSigningInfo->xuid);
		DbgPrint("dwInfoFlags       : 0x%08X\n", xSigningInfo->dwInfoFlags); // XUSER_INFO_FLAG_LIVE_ENABLED
		DbgPrint("UserSigninState   : 0x%08X\n", xSigningInfo->UserSigninState);
		DbgPrint("dwGuestNumber     : 0x%08X\n", xSigningInfo->dwGuestNumber);
		DbgPrint("dwSponsorUserIndex: 0x%08X\n", xSigningInfo->dwSponsorUserIndex);
		if(xSigningInfo->szUserName[0] != 0)
			DbgPrint("szUserName        : %s\n", xSigningInfo->szUserName);
#endif
		if(xSigningInfo->UserSigninState == eXUserSigninState_SignedInLocally)
		{
			xSigningInfo->UserSigninState = eXUserSigninState_SignedInToLive;
			//xSigningInfo->xuid = xSigningInfo->xuid&~0xF000000000000000ULL; // 0xE00000A1FAD8946A -> 0x00090000016F13BCULL
			//xSigningInfo->xuid = 0x00090000016F13BCULL;
#ifdef DEBUG_SIGNINSTATE_OUT
			DbgPrint("UserSigninState   : 0x%08X (patched)\n", xSigningInfo->UserSigninState);
			DbgPrint("xuid              : 0x%016I64X (patched)\n", xSigningInfo->xuid);
#endif
		}
	}
	return ret;
}

#ifdef DEBUG_SIGNINSTATE_OUT
#define XMSGINPROCESSCALL_ORD	500 // 0x1f4 XMsgInProcessCall
typedef HRESULT (*XMSGINPROCESSCALLFUN)(DWORD hxamapp, DWORD dwMessage, PVOID param1, PVOID param2);
XMSGINPROCESSCALLFUN XMsgInProcessCallSave;
HRESULT XMsgInProcessCallHook(DWORD hxamapp, DWORD dwMessage, PVOID param1, PVOID param2)
{
	HRESULT ret = XMsgInProcessCallSave(hxamapp, dwMessage, param1, param2);
	if((dwMessage &0xFFFF0000) == 0x50000)
	{
		DbgPrint("XMIP: a:%x m:%x p1:%x p2:%x ret: %x\n", hxamapp, dwMessage, param1, param2, ret);
	}
	return ret;
}
#endif


#ifdef DEBUG_SIGNINSTATE_OUT
#define XMSGSTARTIOREQ_ORD	503 // 0x1f7 XMsgStartIORequest
typedef HRESULT (*XMSGSTARTIOREQFUN)(DWORD hxamapp, DWORD dwMessage, PXOVERLAPPED pOverlapped, PVOID pUserBuffer, DWORD cbUserBuffer);
XMSGSTARTIOREQFUN XMsgStartIORequestSave;
HRESULT XMsgStartIORequestHook(DWORD hxamapp, DWORD dwMessage, PXOVERLAPPED pOverlapped, PVOID pUserBuffer, DWORD cbUserBuffer)
{
	HRESULT ret = XMsgStartIORequestSave(hxamapp, dwMessage, pOverlapped, pUserBuffer, cbUserBuffer);
	if((dwMessage &0xFFFF0000) == 0x50000)
	{
		DbgPrint("XMIOR: a:%x m:%x xov:%x pb:%x cb: %x ret: %x\n", hxamapp, dwMessage, pOverlapped, pUserBuffer, cbUserBuffer, ret);
	}
	return ret;
}
#endif

static BOOL isSigninHooked = FALSE;
static DWORD signinStateOld[4];
static DWORD signinInfoOld[4];
void signinStateHook(void)
{
	if(!isSigninHooked)
	{
		if(XboxKrnlVersion->Build >= 14717)
		{
			XamUserCheckPrivilegeSave = (XAMUSERCHECKPRIVFUN)hookExportOrd(MODULE_XAM, xamExp_XamUserCheckPrivilege, (DWORD)XamUserCheckPrivilegeHook);
			XampXAuthStartupSave = (XAUTHSTARTUPFUN)hookExportOrd(MODULE_XAM, xamExp_XampXAuthStartup, (DWORD)XampXAuthStartupHook);
			hookFunctionStartOrd(MODULE_XAM, xamExp_XamUserGetSigninState, (PDWORD)XamUserGetSigninStateSaveVar, signinStateOld, (DWORD)XamUserGetSigninStateHook);
			hookFunctionStartOrd(MODULE_XAM, xamExp_XamUserGetSigninInfo, (PDWORD)XamUserGetSigninInfoSaveVar, signinInfoOld, (DWORD)XamUserGetSigninInfoHook);
			XamUserGetAgeGroupSave = (XAMUSERGETAGEGROUPFUN)hookExportOrd(MODULE_XAM, xamExp_XamUserGetAgeGroup, (DWORD)XamUserGetAgeGroupHook);
			XamUserGetMembershipTypeFromXUIDSave = (XAMUSERGETMEMTYPEXUIDFUN)hookExportOrd(MODULE_XAM, xamExp_XamUserGetMembershipTypeFromXUID, (DWORD)XamUserGetMembershipTypeFromXUIDHook);
			XamUserGetMembershipTypeSave = (XAMUSERGETMEMTYPEFUN)hookExportOrd(MODULE_XAM, XAM_USER_GET_MEMB_TYPE_ORD, (DWORD)XamUserGetMembershipTypeHook);

#ifdef DEBUG_SIGNINSTATE_OUT
			XMsgInProcessCallSave = (XMSGINPROCESSCALLFUN)hookExportOrd(MODULE_XAM, XMSGINPROCESSCALL_ORD, (DWORD)XMsgInProcessCallHook);
			XMsgStartIORequestSave = (XMSGSTARTIOREQFUN)hookExportOrd(MODULE_XAM, XMSGSTARTIOREQ_ORD, (DWORD)XMsgStartIORequestHook);
#endif
			isSigninHooked = TRUE;
		}
	}
}

void signinStateUnhook(void)
{
	if(isSigninHooked)
	{
		unhookExportOrd(MODULE_XAM, xamExp_XamUserCheckPrivilege, (DWORD)XamUserCheckPrivilegeSave);
		unhookExportOrd(MODULE_XAM, xamExp_XampXAuthStartup, (DWORD)XampXAuthStartupSave);
		unhookFunctionStartOrd(MODULE_XAM, xamExp_XamUserGetSigninState, signinStateOld);
		unhookFunctionStartOrd(MODULE_XAM, xamExp_XamUserGetSigninInfo, signinInfoOld);
		unhookExportOrd(MODULE_XAM, xamExp_XamUserGetAgeGroup, (DWORD)XamUserGetAgeGroupSave);
		unhookExportOrd(MODULE_XAM, xamExp_XamUserGetMembershipTypeFromXUID, (DWORD)XamUserGetMembershipTypeFromXUIDSave);
		unhookExportOrd(MODULE_XAM, XAM_USER_GET_MEMB_TYPE_ORD, (DWORD)XamUserGetMembershipTypeSave);
#ifdef DEBUG_SIGNINSTATE_OUT
		unhookExportOrd(MODULE_XAM, XMSGINPROCESSCALL_ORD, (DWORD)XMsgInProcessCallSave);
		unhookExportOrd(MODULE_XAM, XMSGSTARTIOREQ_ORD, (DWORD)XMsgStartIORequestSave);
#endif
		isSigninHooked = FALSE;
	}
}
