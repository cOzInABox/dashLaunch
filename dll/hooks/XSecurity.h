#ifndef _XSECURITY_H
#define _XSECURITY_H

NTSTATUS XSecurityOpenCloseHook(DWORD dwHardwareThread);
DWORD XSecurityVerifyHook(DWORD dwMilliseconds, LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
DWORD XSecurityGetFailureInfoHook(PXSECURITY_FAILURE_INFORMATION pFailureInformation);
void XSecurityHook(void);
void XSecurityUnhook(void);

#endif // _XSECURITY_H
