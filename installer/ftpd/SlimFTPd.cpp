/*
* Copyright (c) 2006, Matt Whitlock and WhitSoft Development
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* 
*   * Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*   * Neither the names of Matt Whitlock and WhitSoft Development nor the
*     names of their contributors may be used to endorse or promote products
*     derived from this software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
#include <xtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsockx.h>
#include <sal.h>
#include "xkelib.h"
#include <algorithm>
#include "permdb.h"
#include "userdb.h"
#include "vfs.h"
#include "SlimFTPd.h"
#include "tree.h"
#include "dlDrives.h"
#include "XboxUtil.h"
#include "logging.h"

using namespace std;

#define StrToInt atoi

typedef struct hostent {
	char FAR *    h_name;
	char FAR  FAR **h_aliases;
	short         h_addrtype;
	short         h_length;
	char FAR  FAR **h_addr_list;
}HOSTENT, *PHOSTENT, FAR *LPHOSTENT;

typedef struct Ftpd
{
	string UserName[64];
	int UserCount;
}FTPD;

FTPD ftp;

#define SERVERID							"DLiFTPD 0.1"
#define PACKET_SIZE_RCV						1024*64
#define PACKET_SIZE_SEND					1452

#define IP_ADDRESS_TYPE_LAN					1
#define IP_ADDRESS_TYPE_WAN					2
#define IP_ADDRESS_TYPE_LOCAL				3
#define SOCKET_FILE_IO_DIRECTION_SEND		1
#define SOCKET_FILE_IO_DIRECTION_RECEIVE	2

// Configuration functions
void LogConfError(const char *, DWORD, const char *);
bool ConfSetMountPoint(const char *pszUser, const char *pszVirtual, const char *pszLocal, DWORD dwLine);
bool ConfSetPermission(DWORD dwMode, const char *pszUser, const char *pszVirtual, const char *pszPerms, DWORD dwLine);

// Network functions
bool PatchSocket(SOCKET sock);
bool WINAPI ListenThread(LPVOID);
bool WINAPI ConnectionThread(SOCKET);
bool SocketSendString(SOCKET, const char *);
DWORD SocketReceiveString(SOCKET, char *, DWORD);
DWORD SocketReceiveData(SOCKET, char *, DWORD);
SOCKET EstablishDataConnection(SOCKADDR_IN *, SOCKET *);
void LookupHost(IN_ADDR ia, char *pszHostName, size_t stHostName);
bool DoSocketFileIO(SOCKET sCmd, SOCKET sData, HANDLE hFile, DWORD dwDirection, DWORD *pdwAbortFlag);

// Miscellaneous support functions
DWORD FileReadLine(HANDLE, char *, DWORD);
DWORD SplitTokens(char *);
const char * GetToken(const char *, DWORD);
DWORD GetIPAddressType(IN_ADDR ia);
bool CanUserLogin(const char *pszUser, IN_ADDR iaPeer);

// Global Variables
HINSTANCE hInst;
DWORD dwMaxConnections = 20, dwCommandTimeout = 300, dwConnectTimeout = 15;
bool bLookupHosts = true;
DWORD dwActiveConnections = 0;
SOCKET sListen;
SOCKADDR_IN saiListen;
UserDB *pUsers;

BOOL InitFtpd(USHORT port)
{
	if (StartFtpd(port))
	{
#ifdef LOG_EXTRA_OUT
 		lDbgPrint("FTPd Startup Ok port %d\n", port);
#endif
		return TRUE;
	} 
#ifdef LOG_EXTRA_OUT
 	else
 	{
 		lDbgPrint("An error occurred while starting FTPd.\n");
 	}
#endif
	return FALSE;
}

const char* userPassX = "xbox";
BOOL StartFtpd(USHORT port)
{
	int ret;
	DWORD dw;

	// Allocate user database
	pUsers = new UserDB;

	// Log some startup info
#ifdef LOG_EXTRA_OUT
	lDbgPrint(SERVERID);
	lDbgPrint("\nFTPd is starting.\n");
#endif

	// Init listen socket to defaults
	ZeroMemory(&saiListen,sizeof(SOCKADDR_IN));
	saiListen.sin_family=AF_INET;
	saiListen.sin_addr.S_un.S_addr=INADDR_ANY;
	saiListen.sin_port=htons(port);

	// Exec config script
	ftp.UserCount = 0;
	saiListen.sin_addr.S_un.S_addr=INADDR_ANY;
	bLookupHosts = false;
	saiListen.sin_port=htons(port);
	dwMaxConnections=5;
	dwCommandTimeout=300;
	dwConnectTimeout=15;

	// add user xbox
	pUsers->Add(userPassX);
	pUsers->SetPassword(userPassX,userPassX);
	ftp.UserName[ftp.UserCount] = userPassX;
	ftp.UserCount++;
	FtpdSetDevices();

	// Create and bind the listen socket
	sListen=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sListen == INVALID_SOCKET)
	{
// 		lDbgPrint("sListen returned with error %d\n", WSAGetLastError());
		return FALSE;
	}
	PatchSocket( sListen );
	if(bind(sListen,(SOCKADDR *)&saiListen,sizeof(SOCKADDR_IN))) {
// 		lDbgPrint("Unable to bind socket. Error %d\n", WSAGetLastError());
		closesocket(sListen);
		return FALSE;
	}
	ret = listen(sListen, SOMAXCONN);
	if(ret != NO_ERROR)
	{
// 		lDbgPrint("listen returned error %d\n", WSAGetLastError());
		closesocket(sListen);
		return FALSE;
	}

	HANDLE g_hThread;
	g_hThread = CreateThread( 0, 0, (LPTHREAD_START_ROUTINE)ListenThread, 0, CREATE_SUSPENDED, &dw );
	XSetThreadProcessor( g_hThread, 5 );
	ResumeThread( g_hThread );
	CloseHandle( g_hThread );

	return TRUE;
}

void StopFtpd()
{
	closesocket(sListen);
	// Log the stop of the service
#ifdef LOG_EXTRA_OUT
	lDbgPrint("FTPd has stopped.\n");
#endif
	// Deallocate the user database
	delete pUsers;
}

void LogConfError(const char *pszError, DWORD dwLine, const char *pszArg)
{
#ifdef LOG_EXTRA_OUT
	char sz[1024];
	sprintf_s(sz, (string("Error on line %u: ") + pszError).c_str(), dwLine, pszArg);
	lDbgPrint(sz);
#endif
}

void FtpdAddStaticDevice(const char* user, char* device, char* mount)
{
	string devPath("/");
	string devMount(mount);
	devMount += ":";
	devPath += device;
	ConfSetMountPoint( user, devPath.c_str(), devMount.c_str(), 0 );
	ConfSetPermission( 1, user, devPath.c_str(), "All", 0 );
	ConfSetPermission( 1, user, "/", "Read", 0 );
	ConfSetPermission( 1, user, "/", "List", 0 );

}

void FtpdSetDevices()
{
	int i, j;
	VFS *pvfs;

	for (i = 0; i < ftp.UserCount; i++ )
	{
		pvfs=pUsers->GetVFS(ftp.UserName[i].c_str());
		if (pvfs)pvfs->MountClear();

		for (j = 0; j < (int)dlDrives::GetInstance().GetNumDrives(); j++ )
		{
			PCHAR name = dlDrives::GetInstance().MountDrive(j);
			if(name)
			{
				string MountPoint = name;
				if(MountPoint.size() != 0)
				{
					string MountPath = "/" + MountPoint;
					int pos = MountPath.find_last_of(':');
					if(pos != -1)
						MountPath.erase(pos);
					//Mounting /Hdd1 for user xbox
 //					lDbgPrint("Mounting %s for user %s at %s\n", MountPoint.c_str(), ftp.UserName[i].c_str(), MountPath.c_str());
					ConfSetMountPoint( ftp.UserName[i].c_str(), MountPath.c_str(), MountPoint.c_str(), 0 );
					ConfSetPermission( 1, ftp.UserName[i].c_str(), MountPath.c_str(), "All", 0 );
// 					ConfSetMountPoint( ftp.UserName[i].c_str(), "/xcon", "xcon:", 0 );
// 					ConfSetPermission( 1, ftp.UserName[i].c_str(), "/xcon", "All", 0 );
				}
			}
		}
		FtpdAddStaticDevice(ftp.UserName[i].c_str(), "xcon", "xcon");
		FtpdAddStaticDevice(ftp.UserName[i].c_str(), "SysEx", "xsep");
		FtpdAddStaticDevice(ftp.UserName[i].c_str(), "Flash", "media");
	}
}

bool ConfSetMountPoint(const char *pszUser, const char *pszVirtual, const char *pszLocal, DWORD dwLine)
{
	VFS *pvfs;
	string strVirtual, strLocal;

	VFS::CleanVirtualPath(pszVirtual, strVirtual);

	if (strVirtual.at(0) != '/') {
		LogConfError("Mount directive cannot parse invalid virtual path \"%s\". Virtual paths must begin with a slash.", dwLine, strVirtual.c_str());
		return false;
	}
	if (pszLocal) {
		strLocal = pszLocal;
		replace(strLocal.begin(), strLocal.end(), '/', '\\');
		if (*strLocal.rbegin() == '\\') {
			strLocal = strLocal.substr(0, strLocal.length() - 1);
		}
#ifndef _XBOX
		if (GetFileAttributes(strLocal.c_str()) == -1) {
			LogConfError("Mount directive cannot find local path \"%s\".", dwLine, strLocal.c_str());
			return false;
		}
#endif
	}
	pvfs=pUsers->GetVFS(pszUser);
	if (pvfs) pvfs->Mount(pszVirtual, pszLocal);
	return true;
}

bool ConfSetPermission(DWORD dwMode, const char *pszUser, const char *pszVirtual, const char *pszPerms, DWORD dwLine)
{
	PermDB *pperms;

	string strVirtual;
	VFS::CleanVirtualPath(pszVirtual, strVirtual);

	if (strVirtual.at(0) != '/') {
		if (dwMode) {
			LogConfError("Allow directive cannot parse invalid virtual path \"%s\". Virtual paths must begin with a slash.", dwLine, strVirtual.c_str());
		} else {
			LogConfError("Deny directive cannot parse invalid virtual path \"%s\". Virtual paths must begin with a slash.", dwLine, strVirtual.c_str());
		}
		return false;
	}

	pperms=pUsers->GetPermDB(pszUser);
	if (!pperms) return false;

	while (*pszPerms) {
		if (!_stricmp(pszPerms,"Read")) {
			pperms->SetPerm(strVirtual.c_str(), PERM_READ, dwMode);
		} else if (!_stricmp(pszPerms,"Write")) {
			pperms->SetPerm(strVirtual.c_str(), PERM_WRITE, dwMode);
		} else if (!_stricmp(pszPerms,"List")) {
			pperms->SetPerm(strVirtual.c_str(), PERM_LIST, dwMode);
		} else if (!_stricmp(pszPerms,"Admin")) {
			pperms->SetPerm(strVirtual.c_str(), PERM_ADMIN, dwMode);
		} else if (!_stricmp(pszPerms,"All")) {
			pperms->SetPerm(strVirtual.c_str(), PERM_READ, dwMode);
			pperms->SetPerm(strVirtual.c_str(), PERM_WRITE, dwMode);
			pperms->SetPerm(strVirtual.c_str(), PERM_LIST, dwMode);
			pperms->SetPerm(strVirtual.c_str(), PERM_ADMIN, dwMode);
		} else {
			if (dwMode) {
				LogConfError("Allow directive does not recognize argument \"%s\".",dwLine,pszPerms);
			} else {
				LogConfError("Deny directive does not recognize argument \"%s\".",dwLine,pszPerms);
			}
			return false;
		}
		pszPerms=GetToken(pszPerms,2);
	}
	return true;
}

bool WINAPI ListenThread(LPVOID lParam)
{
	SOCKET sIncoming;
	DWORD dw;

// 	lDbgPrint("Waiting for incoming connections...\n");

	// Accept incoming connections and pass them to connection threads
	while ((sIncoming=accept(sListen,0,0))!=INVALID_SOCKET) {
		HANDLE g_hThread;
		g_hThread = CreateThread( 0, 0x20000, (LPTHREAD_START_ROUTINE)ConnectionThread, (void *)sIncoming, CREATE_SUSPENDED, &dw );
		XSetThreadProcessor( g_hThread, 4 );
		SetThreadPriority( g_hThread, THREAD_PRIORITY_TIME_CRITICAL );
		ResumeThread( g_hThread );
	}
	closesocket(sListen);
	return false;
}

// parses the path to something xbox can handle internally
bool parsePath(const char *vpath, char * dest, DWORD maxlen)
{
	const char* psz;
	DWORD len, i, j, k;
	psz = (strchr(vpath, '/'))+1;
	len = strlen(psz);
	if(len>(maxlen-1))
		return FALSE;
	memset(dest,0,maxlen);
	for(i=0,j=0,k=0; i<len; i++,j++)
	{
		if(psz[i] == '/'){
			if(k == 0){
				dest[j]=':';
				dest[j+1]='\\';
				j++;
				k = 1;
			}
			else
				dest[j]='\\';
		}
		else
			dest[j]=psz[i];
	}
	return TRUE;
}

void execLaunch(const char* vpath, DWORD typ)
{
	char epath[MAX_PATH];
	char mntDrv[] = "X";
	char defPath[] = "X:\\default.xex";
	HANDLE hfile;

	if(parsePath(vpath, epath, MAX_PATH))
	{
		if(typ != 'XEX2') // LIVE con needs mounting first
		{
			if(XboxUtil::GetInstance().MountConExec(mntDrv, epath) >= 0) // mount the con
			{
				// check for default.xex inside, if not there unmount con and return
				hfile = CreateFile(defPath,GENERIC_READ,0,0,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,0);
				if(hfile == INVALID_HANDLE_VALUE)
				{
					XboxUtil::GetInstance().UnmountCon(mntDrv);
					return;
				}
				//if there, put the new path into epath and let 'er launch
				CloseHandle(hfile);
			}
			else return;
		}
		StopFtpd();
		if(typ == 1)
			XLaunchNewImage(defPath,0);
		else
			XLaunchNewImage(epath,0);
	}
}

BOOL conMount(const char* vpath, DWORD typ)
{
	char epath[MAX_PATH];
	char mntDrv[] = "xcon";

	if(typ != 0)
	{
		if(parsePath(vpath, epath, MAX_PATH))
		{
			DWORD dwret;
			XboxUtil::GetInstance().UnmountCon(mntDrv);
// 			lDbgPrint("mounting %s to %s\n", epath, mntDrv);
			dwret = XboxUtil::GetInstance().MountConExec(mntDrv, epath);
// 			lDbgPrint("mount con returned %08x\n", dwret);
			if(dwret == 0) // mount the con
			{
// 				lDbgPrint("con mounted OK\n");
				return TRUE;
			}
		}
	}
	return FALSE;
}

bool WINAPI ConnectionThread(SOCKET sCmd)
{
	SOCKET sData=0, sPasv=0;
	SOCKADDR_IN saiCmd, saiCmdPeer, saiData, saiPasv;
	char szPeerName[64], szOutput[1024], szCmd[512], *pszParam;
	string strUser, strCurrentVirtual, strNewVirtual, strRnFr;
	DWORD dw, dwRestOffset=0;
	bool isLoggedIn = false;
	HANDLE hFile;
	SYSTEMTIME st;
	FILETIME ft;
	VFS *pVFS = NULL;
	PermDB *pPerms = NULL;
	VFS::listing_type listing;
	UINT_PTR i;

	ZeroMemory(&saiData, sizeof(SOCKADDR_IN));

	// Get peer address
	dw=sizeof(SOCKADDR_IN);
	getpeername(sCmd, (SOCKADDR *)&saiCmdPeer, (int *)&dw);
	LookupHost(saiCmdPeer.sin_addr, szPeerName, 64);

	// Log incoming connection
	//sprintf_s(szOutput, "[%u] Incoming connection from %s:%u.", sCmd, szPeerName, ntohs(saiCmdPeer.sin_port));
	//lDbgPrint(szOutput);

	// Send greeting
	sprintf_s(szOutput, "220-%s\r\n220-You are connecting from %s:%u.\r\n220 Proceed with login.\r\n", SERVERID, szPeerName, ntohs(saiCmdPeer.sin_port));
	SocketSendString(sCmd, szOutput);

	// Get host address
	dw=sizeof(SOCKADDR_IN);
	getsockname(sCmd, (SOCKADDR *)&saiCmd, (int *)&dw);

	// Command processing loop
	for (;;) {

		dw=SocketReceiveString(sCmd,szCmd,511);

		if (dw==-1) {
			// Connection dropped or timed out
			SocketSendString(sCmd,"421 Connection timed out.\r\n");
			break;
		} else if (dw>511) {
			SocketSendString(sCmd,"500 Command line too long.\r\n");
			continue;
		}

		pszParam = strchr(szCmd, ' ');
		if (pszParam != NULL) *(pszParam++) = 0;
		else pszParam = szCmd+strlen(szCmd);

		if (!_stricmp(szCmd, "USER")) {
			if (!*pszParam) {
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments.\r\n");
				continue;
			} else if (isLoggedIn) {
				SocketSendString(sCmd, "503 Already logged in. Use REIN to change users.\r\n");
				continue;
			} else {
				strUser = pszParam;
				if (pUsers->CheckPassword(strUser.c_str(), "")) {
					strcpy_s(szCmd, "PASS");
					szCmd[5] = 0;
				} else {
					sprintf_s(szOutput, "331 Need password for user \"%s\".\r\n", strUser.c_str());
					SocketSendString(sCmd, szOutput);
					continue;
				}
			}
		}

		if (!_stricmp(szCmd, "PASS")) {
			if (strUser.empty()) {
				SocketSendString(sCmd, "503 Bad sequence of commands. Send USER first.\r\n");
			} else if (isLoggedIn) {
				SocketSendString(sCmd, "503 Already logged in. Use REIN to change users.\r\n");
			} else {
				if (pUsers->CheckPassword(strUser.c_str(), pszParam)) {
					if (CanUserLogin(strUser.c_str(), saiCmdPeer.sin_addr)) {
						isLoggedIn = true;
						dwActiveConnections++;
						strCurrentVirtual = "/";
						sprintf_s(szOutput, "230 User \"%s\" logged in.\r\n", strUser.c_str());
						SocketSendString(sCmd, szOutput);
#ifdef LOG_EXTRA_OUT
						sprintf_s(szOutput, "[%u] User \"%s\" logged in.", sCmd, strUser.c_str());
						lDbgPrint(szOutput);
#endif
						pVFS = pUsers->GetVFS(strUser.c_str());
						pPerms = pUsers->GetPermDB(strUser.c_str());
					} else {
						SocketSendString(sCmd, "421 Your login was refused due to a server connection limit.\r\n");
#ifdef LOG_EXTRA_OUT
						sprintf_s(szOutput, "[%u] Login for user \"%s\" refused due to connection limit.", sCmd, strUser.c_str());
						lDbgPrint(szOutput);
#endif
						break;
					}
				} else {
					SocketSendString(sCmd,"530 Incorrect password.\r\n");
				}
			}
		}

		else if (!_stricmp(szCmd, "REIN")) {
			if (isLoggedIn) {
				isLoggedIn = false;
				dwActiveConnections--;
				sprintf_s(szOutput, "220-User \"%s\" logged out.\r\n", strUser.c_str());
				SocketSendString(sCmd, szOutput);
#ifdef LOG_EXTRA_OUT
				sprintf_s(szOutput, "[%u] User \"%s\" logged out.", sCmd, strUser.c_str());
				lDbgPrint(szOutput);
#endif
				strUser.clear();
			}
			SocketSendString(sCmd, "220 REIN command successful.\r\n");
		}

		else if (!_stricmp(szCmd, "HELP")) {
			SocketSendString(sCmd, "214 For help, please seek a sufficient medical practitioner.\r\n");
		}

		else if (!_stricmp(szCmd, "FEAT")) {
			SocketSendString(sCmd, "211-Extensions supported:\r\n UTF8\r\n SIZE\r\n REST STREAM\r\n MDTM\r\n TVFS\r\n EXEC\r\n SHDN\r\n211 END\r\n");
			// SocketSendString(sCmd, "211-Extensions supported:\r\n SIZE\r\n REST STREAM\r\n MDTM\r\n TVFS\r\n MNTC\r\n EXEC\r\n211 END\r\n");
		}

		else if (!_stricmp(szCmd, "SYST")) {
			sprintf_s(szOutput, "215 WIN32 Type: L8 Version: %s\r\n", SERVERID);
			SocketSendString(sCmd, szOutput);
		}

		else if (!_stricmp(szCmd, "QUIT")) {
			if (isLoggedIn) {
				isLoggedIn = false;
				dwActiveConnections--;
				sprintf_s(szOutput, "221-User \"%s\" logged out.\r\n", strUser.c_str());
				SocketSendString(sCmd, szOutput);
#ifdef LOG_EXTRA_OUT
				sprintf_s(szOutput, "[%u] User \"%s\" logged out.", sCmd, strUser.c_str());
				lDbgPrint(szOutput);
#endif
			}
			SocketSendString(sCmd, "221 Goodbye!\r\n");
			break;
		}

		else if (!_stricmp(szCmd, "NOOP")) {
			SocketSendString(sCmd, "200 NOOP command successful.\r\n");
		}

		else if (!_stricmp(szCmd, "PWD") || !_stricmp(szCmd, "XPWD")) {
			if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				sprintf_s(szOutput, "257 \"%s\" is current directory.\r\n", strCurrentVirtual.c_str());
				SocketSendString(sCmd, szOutput);
			}
		}

		else if (!_stricmp(szCmd, "CWD") || !_stricmp(szCmd, "XCWD")) {
			if (!*pszParam) {
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments.\r\n");
			} else if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if (pVFS->IsFolder(strNewVirtual.c_str())) {
					strCurrentVirtual = strNewVirtual;
					sprintf_s(szOutput, "250 \"%s\" is now current directory.\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				} else {
					sprintf_s(szOutput, "550 \"%s\": Path not found.\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}

		else if (!_stricmp(szCmd, "CDUP") || !_stricmp(szCmd, "XCUP")) {
			if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), "..", strNewVirtual);
				strCurrentVirtual = strNewVirtual;
				sprintf_s(szOutput,"250 \"%s\" is now current directory.\r\n", strCurrentVirtual.c_str());
				SocketSendString(sCmd, szOutput);
			}
		}

		else if (!_stricmp(szCmd,"TYPE")) {
			if (!*pszParam) {
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments.\r\n");
			} else if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				SocketSendString(sCmd, "200 TYPE command successful.\r\n");
			}
		}

		else if (!_stricmp(szCmd, "REST")) {
			dw = StrToInt(pszParam);
			if (!*pszParam || (!dw) && (*pszParam!='0')) {
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments.\r\n");
			} else if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				dwRestOffset = dw;
				sprintf_s(szOutput, "350 Ready to resume transfer at %u bytes.\r\n", dwRestOffset);
				SocketSendString(sCmd, szOutput);
			}
		}

		else if (!_stricmp(szCmd, "PORT")) {
			if (!*pszParam) {
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments.\r\n");
			} else if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				ZeroMemory(&saiData, sizeof(SOCKADDR_IN));
				saiData.sin_family = AF_INET;
				for (dw = 0; dw < 6; dw++) {
					if (dw < 4) ((unsigned char *)&saiData.sin_addr)[dw] = (unsigned char)StrToInt(pszParam);
					else ((unsigned char *)&saiData.sin_port)[dw-4] = (unsigned char)StrToInt(pszParam);
					pszParam = strchr(pszParam, ',');
					if (!(pszParam)) break;
					pszParam++;
				}
				if (dw == 5) {
					if (sPasv) {
						closesocket(sPasv);
						sPasv = 0;
					}
					SocketSendString(sCmd, "200 PORT command successful.\r\n");
				} else {
					SocketSendString(sCmd, "501 Syntax error in parameters or arguments.\r\n");
					ZeroMemory(&saiData, sizeof(SOCKADDR_IN));
				}
			}
		}

		else if (!_stricmp(szCmd, "PASV")) {
			if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				if (sPasv) closesocket(sPasv);
				ZeroMemory(&saiPasv, sizeof(SOCKADDR_IN));
				saiPasv.sin_family = AF_INET;
				saiPasv.sin_addr.S_un.S_addr = INADDR_ANY;
				saiPasv.sin_port = 0;
				sPasv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				PatchSocket( sPasv );
				bind(sPasv, (SOCKADDR *)&saiPasv, sizeof(SOCKADDR_IN));
				listen(sPasv, 1);
				dw = sizeof(SOCKADDR_IN);
				getsockname(sPasv, (SOCKADDR *)&saiPasv, (int *)&dw);
				sprintf_s(szOutput, "227 Entering Passive Mode (%u,%u,%u,%u,%u,%u)\r\n", saiCmd.sin_addr.S_un.S_un_b.s_b1, saiCmd.sin_addr.S_un.S_un_b.s_b2, saiCmd.sin_addr.S_un.S_un_b.s_b3, saiCmd.sin_addr.S_un.S_un_b.s_b4, ((unsigned char *)&saiPasv.sin_port)[0], ((unsigned char *)&saiPasv.sin_port)[1]);
				SocketSendString(sCmd, szOutput);
			}
		}

		else if (!_stricmp(szCmd, "LIST") || !_stricmp(szCmd, "NLST")) {
			if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				if (*pszParam == '-')
				{
					pszParam = strchr(pszParam, ' ');
					if (pszParam) pszParam++;
				}
				if (pszParam && *pszParam) {
					pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				}
				else {
					strNewVirtual = strCurrentVirtual;
				}
				if (pPerms->GetPerm(strNewVirtual.c_str(), PERM_LIST) == 1) {
					if (pVFS->GetDirectoryListing(strNewVirtual.c_str(), strcmp(szCmd, "LIST"), listing)) {
						sprintf_s(szOutput, "150 Opening %s mode data connection for listing of \"%s\".\r\n", sPasv ? "passive" : "active", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
						sData = EstablishDataConnection(&saiData, &sPasv);
						if (sData) {
							for (VFS::listing_type::const_iterator it = listing.begin(); it != listing.end(); ++it) {
								SocketSendString(sData, it->second.c_str());
							}
							listing.clear();
							closesocket(sData);
							sprintf_s(szOutput, "226 %s command successful.\r\n", _stricmp(szCmd, "NLST") ? "LIST" : "NLST");
							SocketSendString(sCmd, szOutput);
						} else {
							listing.clear();
							SocketSendString(sCmd, "425 Can't open data connection.\r\n");
						}
					} else {
						sprintf_s(szOutput, "550 \"%s\": Path not found.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					}
				} else {
					sprintf_s(szOutput, "550 \"%s\": List permission denied.\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}

		else if (!_stricmp(szCmd, "STAT")) {
			if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				if (*pszParam == '-'){
					pszParam = strchr(pszParam, ' ');
					if (pszParam) pszParam++;
				}
				if (pszParam && *pszParam) {
					pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				}
				else {
					strNewVirtual = strCurrentVirtual;
				}
				if (pPerms->GetPerm(strNewVirtual.c_str(), PERM_LIST) == 1) {
					if (pVFS->GetDirectoryListing(strNewVirtual.c_str(), 1, listing)) {
						sprintf_s(szOutput, "212-Sending directory listing of \"%s\".\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd,szOutput);
						for (VFS::listing_type::const_iterator it = listing.begin(); it != listing.end(); ++it) {
							SocketSendString(sCmd, it->second.c_str());
						}
						listing.clear();
						SocketSendString(sCmd, "212 STAT command successful.\r\n");
					} else {
						sprintf_s(szOutput, "550 \"%s\": Path not found.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					}
				} else {
					sprintf_s(szOutput ,"550 \"%s\": List permission denied.\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}

		else if (!_stricmp(szCmd, "RETR")) {
			if (!*pszParam) {
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments.\r\n");
			} else if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if (pPerms->GetPerm(strNewVirtual.c_str(), PERM_READ) == 1) {
					hFile = pVFS->CreateFile(strNewVirtual.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING);
					if (hFile == INVALID_HANDLE_VALUE) {
						sprintf_s(szOutput, "550 \"%s\": Unable to open file.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					} else {
						if (dwRestOffset) {
							SetFilePointer(hFile, dwRestOffset, 0, FILE_BEGIN);
							dwRestOffset = 0;
						}
						sprintf_s(szOutput, "150 Opening %s mode data connection for \"%s\".\r\n", sPasv ? "passive" : "active", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
						sData = EstablishDataConnection(&saiData, &sPasv);
						if (sData) {
#ifdef LOG_EXTRA_OUT
							sprintf_s(szOutput, "[%u] User \"%s\" began downloading \"%s\".", sCmd, strUser.c_str(), strNewVirtual.c_str());
							lDbgPrint(szOutput);
#endif
							if (DoSocketFileIO(sCmd, sData, hFile, SOCKET_FILE_IO_DIRECTION_SEND, &dw)) {
								sprintf_s(szOutput, "226 \"%s\" transferred successfully.\r\n", strNewVirtual.c_str());
								SocketSendString(sCmd, szOutput);
#ifdef LOG_EXTRA_OUT
								sprintf_s(szOutput, "[%u] Download completed.", sCmd);
								lDbgPrint(szOutput);
#endif
							} else {
								SocketSendString(sCmd, "426 Connection closed; transfer aborted.\r\n");
								if (dw) SocketSendString(sCmd, "226 ABOR command successful.\r\n");
#ifdef LOG_EXTRA_OUT
								sprintf_s(szOutput, "[%u] Download aborted.", sCmd);
								lDbgPrint(szOutput);
#endif
							}
							closesocket(sData);
						} else {
							SocketSendString(sCmd,"425 Can't open data connection.\r\n");
						}
						CloseHandle(hFile);
					}
				} else {
					sprintf_s(szOutput, "550 \"%s\": Read permission denied.\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}

		else if (!_stricmp(szCmd, "STOR") || !_stricmp(szCmd, "APPE")) {
			if (!*pszParam) {
				SocketSendString(sCmd,"501 Syntax error in parameters or arguments.\r\n");
			} else if (!isLoggedIn) {
				SocketSendString(sCmd,"530 Not logged in.\r\n");
			} else {
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if (pPerms->GetPerm(strNewVirtual.c_str(), PERM_WRITE) == 1) {
					hFile = pVFS->CreateFile(strNewVirtual.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_ALWAYS);
					if (hFile == INVALID_HANDLE_VALUE) {
						sprintf_s(szOutput, "550 \"%s\": Unable to open file.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					} else {
						if (_stricmp(szCmd, "APPE") == 0) {
							SetFilePointer(hFile, 0, 0, FILE_END);
						}
						else {
							SetFilePointer(hFile, dwRestOffset, 0, FILE_BEGIN);
							SetEndOfFile(hFile);
						}
						dwRestOffset = 0;
						sprintf_s(szOutput, "150 Opening %s mode data connection for \"%s\".\r\n", sPasv ? "passive" : "active", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
						sData = EstablishDataConnection(&saiData, &sPasv);
						if (sData) {
#ifdef LOG_EXTRA_OUT
							sprintf_s(szOutput, "[%u] User \"%s\" began uploading \"%s\".", sCmd, strUser.c_str(), strNewVirtual.c_str());
							lDbgPrint(szOutput);
#endif
							if (DoSocketFileIO(sCmd, sData, hFile, SOCKET_FILE_IO_DIRECTION_RECEIVE, 0)) {
								sprintf_s(szOutput, "226 \"%s\" transferred successfully.\r\n", strNewVirtual.c_str());
								SocketSendString(sCmd, szOutput);
#ifdef LOG_EXTRA_OUT
								sprintf_s(szOutput, "[%u] Upload completed.", sCmd);
								lDbgPrint(szOutput);
#endif
							} else {
								SocketSendString(sCmd, "426 Connection closed; transfer aborted.\r\n");
#ifdef LOG_EXTRA_OUT
								sprintf_s(szOutput, "[%u] Upload aborted.", sCmd);
								lDbgPrint(szOutput);
#endif
							}
							closesocket(sData);
						} else {
							SocketSendString(sCmd,"425 Can't open data connection.\r\n");
						}
						CloseHandle(hFile);
					}
				} else {
					sprintf_s(szOutput, "550 \"%s\": Write permission denied.\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}

		else if (!_stricmp(szCmd, "ABOR")) {
			if (sPasv) {
				closesocket(sPasv);
				sPasv = 0;
			}
			dwRestOffset = 0;
			SocketSendString(sCmd,"200 ABOR command successful.\r\n");
		}

		else if (!_stricmp(szCmd, "SIZE")) {
			if (!*pszParam) {
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments.\r\n");
			} else if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if (pPerms->GetPerm(strNewVirtual.c_str(), PERM_READ) == 1) {
					hFile = pVFS->CreateFile(strNewVirtual.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING);
					if (hFile == INVALID_HANDLE_VALUE) {
						sprintf_s(szOutput, "550 \"%s\": File not found.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					} else {
						sprintf_s(szOutput, "213 %u\r\n", GetFileSize(hFile, 0));
						SocketSendString(sCmd, szOutput);
						CloseHandle(hFile);
					}
				} else {
					sprintf_s(szOutput, "550 \"%s\": Read permission denied.\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}

		else if (!_stricmp(szCmd, "MDTM")) {
			if (!*pszParam) {
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments.\r\n");
			} else if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				for (i = 0; i < 14; i++) {
					if ((pszParam[i] < '0') || (pszParam[i] > '9')) {
						break;
					}
				}
				if ((i == 14) && (pszParam[14] == ' ')) {
					strncpy_s(szOutput, pszParam, 4);
					szOutput[4] = 0;
					st.wYear = (WORD)StrToInt(szOutput);
					strncpy_s(szOutput, pszParam + 4, 2);
					szOutput[2] = 0;
					st.wMonth = (WORD)StrToInt(szOutput);
					strncpy_s(szOutput, pszParam + 6, 2);
					st.wDay = (WORD)StrToInt(szOutput);
					strncpy_s(szOutput, pszParam + 8, 2);
					st.wHour = (WORD)StrToInt(szOutput);
					strncpy_s(szOutput, pszParam + 10, 2);
					st.wMinute = (WORD)StrToInt(szOutput);
					strncpy_s(szOutput, pszParam + 12, 2);
					st.wSecond = (WORD)StrToInt(szOutput);
					pszParam += 15;
					dw = 1;
				} else {
					dw = 0;
				}
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if (dw) {
					if (pPerms->GetPerm(strNewVirtual.c_str(), PERM_WRITE) == 1) {
						hFile = pVFS->CreateFile(strNewVirtual.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING);
						if (hFile == INVALID_HANDLE_VALUE) {
							sprintf_s(szOutput, "550 \"%s\": File not found.\r\n", strNewVirtual.c_str());
							SocketSendString(sCmd, szOutput);
						} else {
							SystemTimeToFileTime(&st, &ft);
							SetFileTime(hFile, 0, 0, &ft);
							CloseHandle(hFile);
							SocketSendString(sCmd, "250 MDTM command successful.\r\n");
						}
					} else {
						sprintf_s(szOutput, "550 \"%s\": Write permission denied.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					}
				} else {
					if (pPerms->GetPerm(strNewVirtual.c_str(), PERM_READ) == 1) {
						hFile = pVFS->CreateFile(strNewVirtual.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING);
						if (hFile == INVALID_HANDLE_VALUE) {
							sprintf_s(szOutput, "550 \"%s\": File not found.\r\n", strNewVirtual.c_str());
							SocketSendString(sCmd, szOutput);
						} else {
							GetFileTime(hFile, 0, 0, &ft);
							CloseHandle(hFile);
							FileTimeToSystemTime(&ft, &st);
							sprintf_s(szOutput, "213 %04u%02u%02u%02u%02u%02u\r\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
							SocketSendString(sCmd, szOutput);
						}
					} else {
						sprintf_s(szOutput, "550 \"%s\": Read permission denied.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					}
				}
			}
		}

		else if (!_stricmp(szCmd, "DELE")) {
			if (!*pszParam) {
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments.\r\n");
			} else if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if (pPerms->GetPerm(strNewVirtual.c_str(), PERM_ADMIN) == 1) {
					if (pVFS->FileExists(strNewVirtual.c_str())) {
						if (pVFS->DeleteFile(strNewVirtual.c_str())) {
							sprintf_s(szOutput, "250 \"%s\" deleted successfully.\r\n", strNewVirtual.c_str());
							SocketSendString(sCmd, szOutput);
#ifdef LOG_EXTRA_OUT
							sprintf_s(szOutput, "[%u] User \"%s\" deleted \"%s\".", sCmd, strUser.c_str(), strNewVirtual.c_str());
							lDbgPrint(szOutput);
#endif
						} else {
							sprintf_s(szOutput, "550 \"%s\": Unable to delete file.\r\n", strNewVirtual.c_str());
							SocketSendString(sCmd, szOutput);
						}
					} else {
						sprintf_s(szOutput, "550 \"%s\": File not found.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					}
				} else {
					sprintf_s(szOutput, "550 \"%s\": Admin permission denied.\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd,szOutput);
				}
			}
		}

		else if (!_stricmp(szCmd, "RNFR")) {
			if (!*pszParam) {
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments.\r\n");
			} else if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if (pPerms->GetPerm(strNewVirtual.c_str(), PERM_ADMIN) == 1) {
					if (pVFS->FileExists(strNewVirtual.c_str())) {
						strRnFr = strNewVirtual;
						sprintf_s(szOutput, "350 \"%s\": File exists; proceed with RNTO.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					} else {
						sprintf_s(szOutput, "550 \"%s\": File not found.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					}
				} else {
					sprintf_s(szOutput, "550 \"%s\": Admin permission denied.\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}

		else if (!_stricmp(szCmd, "RNTO")) {
			if (!*pszParam) {
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments.\r\n");
			} else if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else if (strRnFr.length() == 0) {
				SocketSendString(sCmd, "503 Bad sequence of commands. Send RNFR first.\r\n");
			} else {
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if (pPerms->GetPerm(strNewVirtual.c_str(), PERM_ADMIN) == 1) {
					if (pVFS->MoveFile(strRnFr.c_str(), strNewVirtual.c_str())) {
						SocketSendString(sCmd, "250 RNTO command successful.\r\n");
#ifdef LOG_EXTRA_OUT
						sprintf_s(szOutput, "[%u] User \"%s\" renamed \"%s\" to \"%s\".", sCmd, strUser.c_str(), strRnFr.c_str(), strNewVirtual.c_str());
						lDbgPrint(szOutput);
#endif
						strRnFr.clear();
					} else {
						sprintf_s(szOutput, "553 \"%s\": Unable to rename file.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					}
				} else {
					SocketSendString(sCmd, "550 Admin permission denied.\r\n");
				}
			}
		}

		else if (!_stricmp(szCmd, "MKD") || !_stricmp(szCmd, "XMKD")) {
			if (!*pszParam) {
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments.\r\n");
			} else if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if (pPerms->GetPerm(strNewVirtual.c_str(), PERM_WRITE) == 1) {
					if (pVFS->CreateDirectory(strNewVirtual.c_str())) {
						sprintf_s(szOutput, "250 \"%s\" created successfully.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
#ifdef LOG_EXTRA_OUT
						sprintf_s(szOutput, "[%u] User \"%s\" created directory \"%s\".", sCmd, strUser.c_str(), strNewVirtual.c_str());
						lDbgPrint(szOutput);
#endif
					} else {
						sprintf_s(szOutput, "550 \"%s\": Unable to create directory.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					}
				} else {
					sprintf_s(szOutput, "550 \"%s\": Write permission denied.\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}

		else if (!_stricmp(szCmd, "RMD") || !_stricmp(szCmd, "XRMD")) {
			if (!*pszParam) {
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments.\r\n");
			} else if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if (pPerms->GetPerm(strNewVirtual.c_str(), PERM_ADMIN) == 1) {
					if (pVFS->RemoveDirectory(strNewVirtual.c_str())) {
						sprintf_s(szOutput, "250 \"%s\" removed successfully.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
#ifdef LOG_EXTRA_OUT
						sprintf_s(szOutput, "[%u] User \"%s\" removed directory \"%s\".", sCmd, strUser.c_str(), strNewVirtual.c_str());
						lDbgPrint(szOutput);
#endif
					} else {
						sprintf_s(szOutput, "550 \"%s\": Unable to remove directory.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					}
				} else {
					sprintf_s(szOutput, "550 \"%s\": Admin permission denied.\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}
		else if (!_stricmp(szCmd, "EXEC")) {
			if (!*pszParam) {
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments.\r\n");
			} else if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if (pPerms->GetPerm(strNewVirtual.c_str(), PERM_READ) == 1) {
					hFile = pVFS->CreateFile(strNewVirtual.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING);
					if (hFile == INVALID_HANDLE_VALUE) {
						sprintf_s(szOutput, "550 \"%s\": Unable to open file.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					} else {
						char tst[5]={0};
						DWORD trd;
						DWORD typ = 0;
						BOOL trf = ReadFile(hFile, tst, 4, &trd ,NULL); // reads 4 bytes of the file to determine type
						CloseHandle(hFile);
						if(trf)	{
							typ = (tst[0]<<24)|(tst[1]<<16)|(tst[2]<<8)|(tst[3]);
							sprintf_s(szOutput, "250 \"%s\" attempting %c%c%c launch (%08x).\r\n", strNewVirtual.c_str(), tst[0], tst[1], tst[2], typ);
							SocketSendString(sCmd, szOutput);
#ifdef LOG_EXTRA_OUT
							lDbgPrint(szOutput);
#endif

							execLaunch(strNewVirtual.c_str(), typ);
							// if this returns, then something went wrong
							sprintf_s(szOutput, "550 \"%s\": Launch path errors, could not start.\r\n", strNewVirtual.c_str());
							SocketSendString(sCmd, szOutput);
						}
					}
				} else {
					sprintf_s(szOutput, "550 \"%s\": Read permission denied.\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}
		else if (!_stricmp(szCmd, "MNTC")) {
			if (!*pszParam) {
				SocketSendString(sCmd, "501 Syntax error in parameters or arguments.\r\n");
			} else if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				pVFS->ResolveRelative(strCurrentVirtual.c_str(), pszParam, strNewVirtual);
				if (pPerms->GetPerm(strNewVirtual.c_str(), PERM_READ) == 1) {
					hFile = pVFS->CreateFile(strNewVirtual.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING);
					if (hFile == INVALID_HANDLE_VALUE) {
						sprintf_s(szOutput, "550 \"%s\": Unable to open file.\r\n", strNewVirtual.c_str());
						SocketSendString(sCmd, szOutput);
					} else {
						char tst[5]={0};
						DWORD trd;
						DWORD typ = 0;
						BOOL trf = ReadFile(hFile, tst, 4, &trd ,NULL); // reads 4 bytes of the file to determine type
						CloseHandle(hFile);
						if(trf)	{
							tst[4] = NULL;
							if(!_stricmp(tst, "CON ")) {
								sprintf_s(szOutput, "250 \"%s\" attempting CON mount.\r\n", strNewVirtual.c_str());
								typ = 1;
							}
							else if(!_stricmp(tst, "LIVE")){
								sprintf_s(szOutput, "250 \"%s\" attempting LIVE mount.\r\n", strNewVirtual.c_str());
								typ = 2;
							}
							else if(!_stricmp(tst, "PIRS")){
								sprintf_s(szOutput, "250 \"%s\" attempting PIRS mount.\r\n", strNewVirtual.c_str());
								typ = 3;
							}
							SocketSendString(sCmd, szOutput);
#ifdef LOG_EXTRA_OUT
							lDbgPrint(szOutput);
#endif

							if(!conMount(strNewVirtual.c_str(), typ))
							{
								// if this returns, then something went wrong
								sprintf_s(szOutput, "550 \"%s\": Mount path errors, could not mount.\r\n", strNewVirtual.c_str());
								SocketSendString(sCmd, szOutput);
							}
							else
							{
								sprintf_s(szOutput, "250 \"%s\" container mounted OK.\r\n", strNewVirtual.c_str());
								SocketSendString(sCmd, szOutput);
							}

						}
					}
				} else {
					sprintf_s(szOutput, "550 \"%s\": Read permission denied.\r\n", strNewVirtual.c_str());
					SocketSendString(sCmd, szOutput);
				}
			}
		}
		else {
			sprintf_s(szOutput,"500 Syntax error, command \"%s\" unrecognized.\r\n",szCmd);
			SocketSendString(sCmd,szOutput);
		}

	}

	if (sPasv) closesocket(sPasv);
	closesocket(sCmd);

	if (isLoggedIn) {
		dwActiveConnections--;
	}

#ifdef LOG_EXTRA_OUT
	sprintf_s(szOutput,"[%u] Connection closed.",sCmd);
	lDbgPrint(szOutput);
#endif

	return false;
}

bool SocketSendString(SOCKET s, const char *psz)
{
	if (send(s,psz,(INT)strlen(psz),0)==SOCKET_ERROR) return false;
	else return true;
}

DWORD SocketReceiveString(SOCKET s, char *psz, DWORD dwMaxChars)
{
	DWORD dw, dwBytes;
	TIMEVAL tv;
	fd_set fds;

	tv.tv_sec=dwCommandTimeout;
	tv.tv_usec=0;
	for (dwBytes=0;;dwBytes++) {
		FD_ZERO(&fds);
		FD_SET(s,&fds);
		dw=select(0,&fds,0,0,&tv);
		if (dw==SOCKET_ERROR || dw==0) return 0xFFFFFFFF; // Timeout
		dw=recv(s,psz,1,0);
		if (dw==SOCKET_ERROR || dw==0) return 0xFFFFFFFF; // Network error
		if (*psz=='\r') *psz=0;
		else if (*psz=='\n') {
			*psz=0;
			return dwBytes;
		}
		if (dwBytes<dwMaxChars) psz++;
	}
}

DWORD SocketReceiveData(SOCKET s, char *psz, DWORD dwBytesToRead)
{
	DWORD dw;
	TIMEVAL tv;
	fd_set fds;

	tv.tv_sec=dwConnectTimeout;
	tv.tv_usec=0;
	FD_ZERO(&fds);
	FD_SET(s,&fds);
	dw=select(0,&fds,0,0,&tv);
	if (dw==SOCKET_ERROR || dw==0) return 0xFFFFFFFF; // Timeout
	dw=recv(s,psz,dwBytesToRead,0);

	//	lDbgPrint("Bytes Received: %ld\n", dw);

	if (dw==SOCKET_ERROR) return 0xFFFFFFFF; // Network error
	return dw;
}

SOCKET EstablishDataConnection(SOCKADDR_IN *psaiData, SOCKET *psPasv)
{
	SOCKET sData;
	DWORD dw;
	TIMEVAL tv;
	fd_set fds;

	if (psPasv && *psPasv) {
		tv.tv_sec=dwConnectTimeout;
		tv.tv_usec=0;
		FD_ZERO(&fds);
		FD_SET(*psPasv,&fds);
		dw=select(0,&fds,0,0,&tv);
		if (dw && dw!=SOCKET_ERROR) {
			dw=sizeof(SOCKADDR_IN);
			sData=accept(*psPasv,(SOCKADDR *)psaiData,(int *)&dw);
		} else {
			sData=0;
		}
		closesocket(*psPasv);
		*psPasv=0;
		return sData;
	} else {
		sData=socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
		PatchSocket( sData );
		if (connect(sData,(SOCKADDR *)psaiData,sizeof(SOCKADDR_IN))) {
			closesocket(sData);
			return false;
		} else {
			return sData;
		}
	}
}

void LookupHost( IN_ADDR ia, char *pszHostName, size_t stHostName )
	// Performs a reverse DNS lookup on ia. If no host name could be resolved, or
	// if LookupHosts is Off, pszHostName will contain a string representation of
	// the given IP address.
{
	sprintf_s( pszHostName, stHostName, "%d.%d.%d.%d",ia.S_un.S_un_b.s_b1, ia.S_un.S_un_b.s_b2, ia.S_un.S_un_b.s_b3, ia.S_un.S_un_b.s_b4 );
}

bool DoSocketFileIO(SOCKET sCmd, SOCKET sData, HANDLE hFile, DWORD dwDirection, DWORD *pdwAbortFlag)
{
	char szBuffer_rcv[PACKET_SIZE_RCV];
	char szBuffer_send[PACKET_SIZE_SEND];
	DWORD dw;

	if (pdwAbortFlag) *pdwAbortFlag = 0;
	switch (dwDirection) {
	case SOCKET_FILE_IO_DIRECTION_SEND:
		for (;;) {
			if (!ReadFile(hFile, szBuffer_send, PACKET_SIZE_SEND, &dw, 0)) return false;
			if (!dw) return true;
			if (send(sData, szBuffer_send, dw, 0) == SOCKET_ERROR) return false;
			ioctlsocket(sCmd, FIONREAD, &dw);
			if (dw) {
				SocketReceiveString(sCmd, szBuffer_send, 511);
				if (!_stricmp(szBuffer_send, "ABOR")) {
					*pdwAbortFlag = 1;
					return false;
				} else {
					SocketSendString(sCmd, "500 Only command allowed at this time is ABOR.\r\n");
				}
			}
		}
		break;
	case SOCKET_FILE_IO_DIRECTION_RECEIVE:
		for (;;) {
			dw = SocketReceiveData(sData, szBuffer_rcv, PACKET_SIZE_RCV);
			if (dw == -1) return false;
			if (dw == 0) return true;
			if (!WriteFile(hFile, szBuffer_rcv, dw, &dw, 0)) return false;
		}
		break;
	default:
		return false;
	}
}

DWORD FileReadLine(HANDLE hFile, char *pszBuf, DWORD dwBufLen)
{
	// Reads a line from an open text file into a character buffer, discarding the
	// trailing CR/LF, up to dwBufLen bytes. Any additional bytes are discarded.
	// Returns the number of characters in the line, excluding the CR/LF, or -1 if
	// the end of the file was reached. May be greater than dwBufLen to indicate
	// that bytes were discarded. Note that a return value of 0 does not
	// necessarily indicate an error; it could mean a blank line was read.

	DWORD dw, dwBytesRead, dwCount;

	for (dwCount=0;;) {
		dw=ReadFile(hFile,pszBuf,1,&dwBytesRead,0);
		if (!dw || (dw && !dwBytesRead && !dwCount)) return 0xFFFFFFFF;
		if (!dwBytesRead || *pszBuf=='\n') break;
		if (*pszBuf!='\r') {
			dwCount++;
			if (dwCount<dwBufLen) pszBuf++;
		}
	}
	*pszBuf=0;

	return dwCount;
}

DWORD SplitTokens(char *pszIn)
{
	// Processes a string into a null-separated list of its tokens. A quoted
	// substring will be treated as a single token. Backslashes may be used to
	// escape special characters within quoted substrings.
	// Returns the number of tokens split from the string.

	DWORD dwCount=0;
	char *pszOut;

	pszOut=pszIn;
	while (*pszIn==' ' || *pszIn=='\t') pszIn++;
	while (*pszIn) {
		dwCount++;
		if (*pszIn=='"') {
			pszIn++;
			for (;;pszOut++,pszIn++) {
				if (*pszIn=='\\') {
					pszIn++;
					if (*pszIn=='n') *pszIn='\n';
					else if (*pszIn=='r') *pszIn='\r';
					else if (*pszIn=='t') *pszIn='\t';
					*pszOut=*pszIn;
					continue;
				} else if (!*pszIn || *pszIn=='"') {
					break;
				} else {
					*pszOut=*pszIn;
				}
			}
			pszIn++;
		} else {
			do {
				*pszOut=*pszIn;
				pszOut++;
				pszIn++;
			} while (*pszIn && *pszIn!=' ' && *pszIn!='\t');
		}
		while (*pszIn==' ' || *pszIn=='\t') pszIn++;
		*pszOut=0;
		pszOut++;
	}
	*pszOut=0;
	return dwCount;
}

const char *GetToken(const char *pszTokens, DWORD dwToken) {
	// Returns a pointer to the one-based dwToken'th null-separated token in
	// pszTokens.

	DWORD dw;

	for (dw=1;dw<dwToken;dw++,pszTokens++) {
		pszTokens+=strlen(pszTokens);
	}
	return pszTokens;
}

DWORD GetIPAddressType(IN_ADDR ia)
	// Returns one of the predefined IP address types, according to ia.
{
	if (((ia.S_un.S_un_b.s_b1 == 192) && (ia.S_un.S_un_b.s_b2 == 168)) || ((ia.S_un.S_un_b.s_b1 == 169) && (ia.S_un.S_un_b.s_b2 == 254)) || (ia.S_un.S_un_b.s_b1 == 10)) {
		return IP_ADDRESS_TYPE_LAN;
	} else if ((ia.S_un.S_un_b.s_b1 == 127) && (ia.S_un.S_un_b.s_b2 == 0) && (ia.S_un.S_un_b.s_b3 == 0) && (ia.S_un.S_un_b.s_b4 == 1)) {
		return IP_ADDRESS_TYPE_LOCAL;
	} else {
		return IP_ADDRESS_TYPE_WAN;
	}
}

inline bool CanUserLogin(const char *pszUser, IN_ADDR iaPeer)
{
	return (dwActiveConnections < dwMaxConnections);
}

bool PatchSocket( SOCKET sock )
{
#ifndef _DEBUG
	BOOL bBroadcast = TRUE;
	//if( setsockopt(sock, SOL_SOCKET, SO_PRIVATE, (PCSTR)&bBroadcast, sizeof(BOOL) ) != 0 )//PATCHED!
	//{
 //		lDbgPrint( "Failed to set sockopt SO_PRIVATE, error\n");
	//	return FALSE;
	//}

	if( setsockopt(sock, SOL_SOCKET, SO_MARKINSECURE, (PCSTR)&bBroadcast, sizeof(BOOL) ) != 0 )//PATCHED!
	{
 		lDbgPrint( "Failed to set sockopt SO_MARKINSECURE, error\n");
		return FALSE;
	}
	//if( setsockopt(sock, SOL_SOCKET, SO_GRANTINSECURE, (PCSTR)&bBroadcast, sizeof(BOOL) ) != 0 )//PATCHED!
	//{
 //		lDbgPrint( "Failed to set sockopt SO_GRANTINSECURE, error\n");
	//	return FALSE;
	//}
#endif
	return TRUE;
}
