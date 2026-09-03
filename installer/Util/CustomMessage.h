#pragma once
#include <xtl.h>
#include <xui.h>
#include <xuiapp.h>

#define XM_NOTIFY_ANYKEY  XM_USER



// Sig: HRESULT OnAnyKey(DWORD key, BOOL& bHandled)
#define XUI_ON_XM_ANYKEY(MemberFunc)\
    if (pMessage->dwMessage == XM_NOTIFY_ANYKEY)\
    {\
		XINPUT_KEYSTROKE* pkey = (XINPUT_KEYSTROKE *) pMessage->pvData;\
        return MemberFunc(pkey->VirtualKey, pMessage->bHandled);\
    }

void XuiMessageAnykey(XUIMessage *pMsg, XINPUT_KEYSTROKE* pData);
