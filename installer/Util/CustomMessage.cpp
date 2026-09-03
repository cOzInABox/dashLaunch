#include "CustomMessage.h"

void XuiMessageAnykey(XUIMessage *pMsg, XINPUT_KEYSTROKE* pData)
{
	XuiMessage(pMsg,XM_NOTIFY_ANYKEY);
	_XuiMessageExtra(pMsg,(XUIMessageData*) pData, sizeof(*pData));
}
