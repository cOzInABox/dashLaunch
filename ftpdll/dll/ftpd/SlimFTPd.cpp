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
#include <algorithm>
#include "permdb.h"
#include "userdb.h"
#include "vfs.h"
#include "tree.cpp"
#include "../mounts.h"
#include "../xamext.h"
#include "../util.h"
#include "../resolveFunct.h"

extern MOUNTS mnt[];
extern int nummnt;

#pragma warning(disable:4127)
using namespace std;

#define StrToInt atoi

typedef struct hostent {
  char FAR *    h_name;
  char FAR  FAR **h_aliases;
  short         h_addrtype;
  short         h_length;
  char FAR  FAR **h_addr_list;
}HOSTENT, *PHOSTENT, FAR *LPHOSTENT;

// Crappy user buffers for device add

typedef struct Ftpd
{
	string UserName[64];
	int UserCount;
}FTPD;

FTPD ftp;

#define SERVERID "xFTPDll 0.1 powered by SlimFTPd"
#define PACKET_SIZE_SEND					1452
#define PACKET_SIZE_RECV					(1024*64)
#define IP_ADDRESS_TYPE_LAN					1
#define IP_ADDRESS_TYPE_WAN					2
#define IP_ADDRESS_TYPE_LOCAL				3
#define SOCKET_FILE_IO_DIRECTION_SEND		1
#define SOCKET_FILE_IO_DIRECTION_RECEIVE	2

// Service functions
bool Startup();
void Cleanup();
void FtpdSetDevices();
bool PatchSocket(SOCKET sock);

// Configuration functions {
bool ConfSetMountPoint(const char *pszUser, const char *pszVirtual, const char *pszLocal, DWORD dwLine);
bool ConfSetPermission(DWORD dwMode, const char *pszUser, const char *pszVirtual, const char *pszPerms, DWORD dwLine);

// Network functions
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
const char * GetToken(const char *, DWORD);
DWORD GetIPAddressType(IN_ADDR ia);
bool CanUserLogin(const char *pszUser, IN_ADDR iaPeer);


// Global Variables 
HINSTANCE hInst;
bool isWinNT, isService;
DWORD dwMaxConnections = 20, dwCommandTimeout = 300, dwConnectTimeout = 15;
bool bLookupHosts = true;
DWORD dwActiveConnections = 0;
SOCKET sListen;
SOCKADDR_IN saiListen;
UserDB *pUsers;

BOOL CntrlFtpd(int mode) //1 is for startup, 0 is for shutdown
{
	if(mode == 0)
	{
		Cleanup();
		return TRUE;
	}
	else if (mode == 1)
	{
		NetDll_WSACleanup(XNCALLER_SYSAPP);
		isService = false;

		XNetStartupParams xnsp;
		memset(&xnsp, 0, sizeof(xnsp));
		xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);
		xnsp.cfgFlags = 0x80|XNET_STARTUP_BYPASS_SECURITY;
		xnsp.cfgSockDefaultRecvBufsizeInK = 128; // default = 16
		xnsp.cfgSockDefaultSendBufsizeInK = 128; // default = 16
		xnsp.cfgQosSrvMaxSimultaneousResponses = 16;

		//INT iResult = XNetStartup(&xnsp);
		INT iResult = NetDll_XNetStartupEx(XNCALLER_SYSAPP, &xnsp, CUR_VER);
		if( iResult != NO_ERROR )
		{
			DbgPrint("XNETSTARTUP ERROR\n");
		}
		else if(Startup())
		{
			DbgPrint("Startup Ok\n");
			return TRUE;
		} 
		else
		{
			DbgPrint("An error occurred while starting SlimFTPd.\n");
		}
	}
	return FALSE;
}

const char* userPassX = "xbox";

bool Startup()
{
	WSADATA wsad;
	DWORD dw;

	// Allocate user database
	pUsers = new UserDB;

	// Log some startup info
	//DbgPrint("-------------------------------------------------------------------------------");
	//DbgPrint(SERVERID);
	//DbgPrint("FTPd is starting.");

	// Init listen socket to defaults
	ZeroMemory(&saiListen,sizeof(SOCKADDR_IN));
	saiListen.sin_family=AF_INET;
	saiListen.sin_addr.S_un.S_addr=INADDR_ANY;
	saiListen.sin_port=htons(21);

	// Start Winsock
	NetDll_WSAStartupEx(XNCALLER_SYSAPP, 2, &wsad, CUR_VER);

	// Exec config script
	ftp.UserCount = 0;
	saiListen.sin_addr.S_un.S_addr=INADDR_ANY; //ConfSetBindInterface( NULL, 0 );
	bLookupHosts = false; //ConfSetLookupHosts( NULL, 0 );
	saiListen.sin_port=htons(7564); //ConfSetBindPort(GetToken(psz,2),dwLine);
	dwMaxConnections=20; // ConfSetMaxConnections(GetToken(psz,2),dwLine)
	dwCommandTimeout=300; // ConfSetCommandTimeout(GetToken(psz,2),dwLine)
	dwConnectTimeout=15; //ConfSetConnectTimeout(GetToken(psz,2),dwLine)

	// add user xbox
	pUsers->Add(userPassX); // ConfAddUser(GetToken(psz,2),dwLine)
	pUsers->SetPassword(userPassX,userPassX); // ConfSetUserPassword(strUser.c_str(), GetToken(psz, 2), dwLine))
	ftp.UserName[ftp.UserCount] = userPassX;
	ftp.UserCount++;
	//FtpdSetDevices();

	// add user flash
	//pUsers->Add(userPassF); // ConfAddUser(GetToken(psz,2),dwLine)
	//pUsers->SetPassword(userPassF,userPassF); // ConfSetUserPassword(strUser.c_str(), GetToken(psz, 2), dwLine))
	//ftp.UserName[ftp.UserCount] = userPassF;
	//ftp.UserCount++;

	// Create and bind the listen socket
	sListen=NetDll_socket(XNCALLER_SYSAPP, AF_INET, SOCK_STREAM, IPPROTO_TCP);
	PatchSocket( sListen );
	if (NetDll_bind(XNCALLER_SYSAPP, sListen,(SOCKADDR *)&saiListen,sizeof(SOCKADDR_IN))) {
		DbgPrint("Unable to bind socket. Specified port may already be in use.");
		NetDll_closesocket(XNCALLER_SYSAPP, sListen);
		return false;
	}
	NetDll_listen(XNCALLER_SYSAPP, sListen,SOMAXCONN);

	// Launch the listen thread
	HANDLE g_hThread;
	//  thread flags 0x400 = hide from debugger
	//g_hThread = CreateThread( 0, 0, (LPTHREAD_START_ROUTINE)ListenThread, 0, CREATE_SUSPENDED, &dw );
	ExCreateThread(&g_hThread, 0, &dw, (VOID*) XapiThreadStartup , (LPTHREAD_START_ROUTINE)ListenThread, 0, 0x18000426);// 0x18000426 // 0x040004A6 (natelx suggested)
	XSetThreadProcessor( g_hThread, 4 );

	//ExCreateThread(&g_hThread, 0, &dw, (VOID*) XapiThreadStartup , (LPTHREAD_START_ROUTINE)ListenThread, 0, 0x2);
	//XSetThreadProcessor( g_hThread, 4 );
	ResumeThread( g_hThread );

	return true;
}

void Cleanup()
{
	// Cleanup Winsock
	NetDll_WSACleanup(XNCALLER_SYSAPP);

	// Log the stop of the service
	DbgPrint("SlimFTPd has stopped.");

	// Deallocate the user database
	delete pUsers;

	// Shut down the logger thread
}

void FtpdSetDevices()
{
	int i, j;

	for ( i = 0; i < ftp.UserCount; i++ )
	{
		for ( j = 0; j < nummnt; j++ )
		{
			if(mnt[j].isMounted)
			{
				char MountPoint[512];
				memset( MountPoint, 0, 512 );

				sprintf_s( MountPoint, "/%s", mnt[j].name);
				MountPoint[strlen(MountPoint)-1] = '\0';

				//DbgPrint("Mounting %s for user %s\n", MountPoint, ftp.UserName[i].c_str() );

				ConfSetMountPoint( ftp.UserName[i].c_str(), MountPoint, mnt[j].name, 0 );
				ConfSetPermission( 1, ftp.UserName[i].c_str(), MountPoint, "All", 0 );
			}
		}

		ConfSetPermission( 1, ftp.UserName[i].c_str(), "/", "Read", 0 );
		ConfSetPermission( 1, ftp.UserName[i].c_str(), "/", "List", 0 );
	}
}

bool ConfSetMountPoint(const char *pszUser, const char *pszVirtual, const char *pszLocal, DWORD dwLine)
{
	VFS *pvfs;
	string strVirtual, strLocal;

	VFS::CleanVirtualPath(pszVirtual, strVirtual);

	if (strVirtual.at(0) != '/') {
		//DbgPrint("Mount directive cannot parse invalid virtual path \"%s\". Virtual paths must begin with a slash.", dwLine, strVirtual.c_str());
		return false;
	}
	if (pszLocal) {
		strLocal = pszLocal;
		replace(strLocal.begin(), strLocal.end(), '/', '\\');
		if (*strLocal.rbegin() == '\\') {
			strLocal = strLocal.substr(0, strLocal.length() - 1);
		}
		//if (GetFileAttributes(strLocal.c_str()) == -1) {
		//	//DbgPrint("Mount directive cannot find local path \"%s\".", dwLine, strLocal.c_str());
		//	return false;
		//}
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
		//if (dwMode) {
		//	DbgPrint("Allow directive cannot parse invalid virtual path \"%s\". Virtual paths must begin with a slash.", dwLine, strVirtual.c_str());
		//} else {
		//	DbgPrint("Deny directive cannot parse invalid virtual path \"%s\". Virtual paths must begin with a slash.", dwLine, strVirtual.c_str());
		//}
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
			//if (dwMode) {
			//	DbgPrint("Allow directive does not recognize argument \"%s\".",dwLine,pszPerms);
			//} else {
			//	DbgPrint("Deny directive does not recognize argument \"%s\".",dwLine,pszPerms);
			//}
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

	DbgPrint("Waiting for incoming connections...\n");

	// Accept incoming connections and pass them to connection threads
	while ((sIncoming=NetDll_accept(XNCALLER_SYSAPP, sListen,0,0))!=INVALID_SOCKET) {

		HANDLE g_hThread;
		//g_hThread = CreateThread( 0, 0x20000, (LPTHREAD_START_ROUTINE)ConnectionThread, (void *)sIncoming, CREATE_SUSPENDED, &dw );
		// 0x04000026 0x104000026ULL
		// 0x040004A6 0x04000426
		// 0x010000A2 XamCallBackgroundModeNotificationRoutinesThreadProc
		// 0x08000426 FastpipeDiskThread
		// 0x04000426 FastpipeNetThread
		ExCreateThread(&g_hThread, 0x20000, &dw, (VOID*) XapiThreadStartup , (LPTHREAD_START_ROUTINE)ConnectionThread, (void *)sIncoming, 0x18000426);// 0x18000426 // 0x040004A6 (natelx suggested)
		XSetThreadProcessor( g_hThread, 2 );
		//SetThreadPriority(g_hThread,THREAD_PRIORITY_TIME_CRITICAL);
		//ResumeThread( g_hThread );
	}
	NetDll_closesocket(XNCALLER_SYSAPP, sListen);

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

//XamLoaderLaunchTitleEx(szLaunchPath, szMountPath, szCmdLine, 0);
static char execpath[MAX_PATH];
BOOL execLaunch(const char* vpath)
{
	const char* path;
	int i;
	NTSTATUS sta;
	for(i = 0; i < nummnt; i++)
	{
		if(strncmp(vpath, mnt[i].name, strlen(mnt[i].name)) == 0)
		{
			XAMLOADERLAUNCHTITLEEX XamLoaderLaunchTitleEx = (XAMLOADERLAUNCHTITLEEX)resolveFunct("xam.xex", XAMLOADERLAUNCHTITLEEX_ORD);
			if(XamLoaderLaunchTitleEx != NULL)
			{
				path = &vpath[(strlen(mnt[i].name)+1)];
				strcpy(execpath, mnt[i].path);
				strcat(execpath, "\\");
				strcat(execpath, path);
				//DbgPrint("exe vpath: %s path: %s final: %s\n", vpath, path, execpath);
				i = nummnt+1;
				//sta = XamLoaderLaunchTitleEx("\\Device\\Flash\\lhelper.xex", NULL, NULL, 0);
				sta = XamLoaderLaunchTitleEx(execpath, NULL, NULL, 0);
				DbgPrint("exe vpath: %s path: %s final: %s ex status: %08x\n\n", vpath, path, execpath, sta);
				//DbgPrint("ex status: %08x\n", sta);
				if(sta >= 0)
					return TRUE;
			}
		}
	}
	return FALSE;
}

char* threadTypes[] = {
	"none",
	"title",
	"system"
};

bool WINAPI ConnectionThread(SOCKET sCmd)
{
	SOCKET sData=0, sPasv=0;
	SOCKADDR_IN saiCmd, saiCmdPeer, saiData, saiPasv;
	char szPeerName[64], szOutput[1024], szCmd[512], *pszParam=0;
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
	NetDll_getpeername(XNCALLER_SYSAPP, sCmd, (SOCKADDR *)&saiCmdPeer, (int *)&dw);
	LookupHost(saiCmdPeer.sin_addr, szPeerName, 64);

	// Log incoming connection
	sprintf_s(szOutput, "[%u] Incoming connection from %s:%u.", sCmd, szPeerName, ntohs(saiCmdPeer.sin_port));
	//DbgPrint(szOutput);

	// Send greeting
	sprintf_s(szOutput, "220-%s\r\n220-You are connecting from %s:%u.\r\n220 Proceed with login.\r\n", SERVERID, szPeerName, ntohs(saiCmdPeer.sin_port));
	SocketSendString(sCmd, szOutput);

	// Get host address
	dw=sizeof(SOCKADDR_IN);
	NetDll_getsockname(XNCALLER_SYSAPP, sCmd, (SOCKADDR *)&saiCmd, (int *)&dw);
	//DbgPrint("connection thread is on hardware %d type %s\n", GetCurrentProcessorNumber(), threadTypes[KeGetCurrentProcessType()]);
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
		if (pszParam) *(pszParam++) = 0;
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
						sprintf_s(szOutput, "[%u] User \"%s\" logged in.", sCmd, strUser.c_str());
						//DbgPrint(szOutput);
						pVFS = pUsers->GetVFS(strUser.c_str());
						pPerms = pUsers->GetPermDB(strUser.c_str());
					} else {
						SocketSendString(sCmd, "421 Your login was refused due to a server connection limit.\r\n");
						sprintf_s(szOutput, "[%u] Login for user \"%s\" refused due to connection limit.", sCmd, strUser.c_str());
						//DbgPrint(szOutput);
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
				sprintf_s(szOutput, "[%u] User \"%s\" logged out.", sCmd, strUser.c_str());
				//DbgPrint(szOutput);
				strUser.clear();
			}
			SocketSendString(sCmd, "220 REIN command successful.\r\n");
		}

		else if (!_stricmp(szCmd, "HELP")) {
			SocketSendString(sCmd, "214 Help? Orlly? Srsly?\r\n");
		}

		else if (!_stricmp(szCmd, "FEAT")) {
			SocketSendString(sCmd, "211-Extensions supported:\r\n UTF8\r\n SIZE\r\n REST STREAM\r\n MDTM\r\n TVFS\r\n EXEC\r\n SHDN\r\n211 END\r\n");
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
				sprintf_s(szOutput, "[%u] User \"%s\" logged out.", sCmd, strUser.c_str());
				//DbgPrint(szOutput);
			}
			SocketSendString(sCmd, "221 Goodbye!\r\n");
			break;
		}

		else if (!_stricmp(szCmd, "SHDN")) {
			if (isLoggedIn) {
				isLoggedIn = false;
				dwActiveConnections--;
				sprintf_s(szOutput, "221-User \"%s\" logged out.\r\n", strUser.c_str());
				SocketSendString(sCmd, szOutput);
				sprintf_s(szOutput, "[%u] User \"%s\" logged out.", sCmd, strUser.c_str());
				//DbgPrint(szOutput);
			}
			sprintf_s(szOutput, "221-FTP server going down...\r\n");
			SocketSendString(sCmd, szOutput);
			SocketSendString(sCmd, "221 Goodbye!\r\n");
			if (sPasv) {
				NetDll_closesocket(XNCALLER_SYSAPP, sPasv);
				sPasv = 0;
			}
			dwRestOffset = 0;
			NetDll_closesocket(XNCALLER_SYSAPP, sCmd);
			HalReturnToFirmware(5);
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

		else if (!(_stricmp(szCmd, "REST"))) {
			dw = StrToInt(pszParam);
			if (!*pszParam || (!(dw) && (*pszParam!='0'))) {
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
						NetDll_closesocket(XNCALLER_SYSAPP, sPasv);
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
				if (sPasv) NetDll_closesocket(XNCALLER_SYSAPP, sPasv);
				ZeroMemory(&saiPasv, sizeof(SOCKADDR_IN));
				saiPasv.sin_family = AF_INET;
				saiPasv.sin_addr.S_un.S_addr = INADDR_ANY;
				saiPasv.sin_port = 0;
				sPasv = NetDll_socket(XNCALLER_SYSAPP,AF_INET, SOCK_STREAM, IPPROTO_TCP);

				PatchSocket( sPasv );

				NetDll_bind(XNCALLER_SYSAPP, sPasv, (SOCKADDR *)&saiPasv, sizeof(SOCKADDR_IN));
				NetDll_listen(XNCALLER_SYSAPP, sPasv, 1);
				dw = sizeof(SOCKADDR_IN);
				NetDll_getsockname(XNCALLER_SYSAPP, sPasv, (SOCKADDR *)&saiPasv, (int *)&dw);
				sprintf_s(szOutput, "227 Entering Passive Mode (%u,%u,%u,%u,%u,%u)\r\n", saiCmd.sin_addr.S_un.S_un_b.s_b1, saiCmd.sin_addr.S_un.S_un_b.s_b2, saiCmd.sin_addr.S_un.S_un_b.s_b3, saiCmd.sin_addr.S_un.S_un_b.s_b4, ((unsigned char *)&saiPasv.sin_port)[0], ((unsigned char *)&saiPasv.sin_port)[1]);
				SocketSendString(sCmd, szOutput);
			}
		}

		else if (!_stricmp(szCmd, "LIST") || !_stricmp(szCmd, "NLST")) {
			if (!isLoggedIn) {
				SocketSendString(sCmd, "530 Not logged in.\r\n");
			} else {
				if (*pszParam == '-') {
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
							NetDll_closesocket(XNCALLER_SYSAPP, sData);
							sprintf_s(szOutput, "226 %s command successful.\r\n", _stricmp(szCmd, "NLST") ? "LIST" : "NLST");
							SocketSendString(sCmd, szOutput);
						} else {
							listing.clear();
							SocketSendString(sCmd, "425 Can't open data connection.\r\n");
						}
					} else {
						sprintf_s(szOutput, "550 \"%s\": Path not found.\r\n", strNewVirtual.c_str());
						//sprintf_s(szOutput, "226 %s command successful.\r\n", _stricmp(szCmd, "NLST") ? "LIST" : "NLST");
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
						char tst[5]={0,0,0,0,0};
						DWORD trd;
						BOOL trf = ReadFile(hFile, tst, 4, &trd ,NULL); // reads 4 bytes of the file to determine type
						CloseHandle(hFile);
						if(trf)
						{
							tst[4] = NULL;
							if(_stricmp(tst, "XEX2") == 0) 
							{
								char spath[MAX_PATH];
								if( parsePath( strNewVirtual.c_str(), spath, MAX_PATH ) )
								{
									sprintf_s( szOutput, "250 \"%s\" attempting XEX launch.\r\n", spath );
									SocketSendString( sCmd, szOutput );

									memset( szOutput, 0, 1024 );
									sprintf_s( szOutput, "%s", spath );
									if(execLaunch(szOutput))
									{
										sprintf_s(szOutput, "250 \"%s\": launched OK.\r\n", spath);
									}
									else
									{
										sprintf_s(szOutput, "550 \"%s\": could not launch.\r\n", spath);
									}
									SocketSendString(sCmd, szOutput);
								}
							}
							else
							{
								sprintf_s(szOutput, "550 \"%s\" this is not a XEX, cannot launch.\r\n", strNewVirtual.c_str());
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
							sprintf_s(szOutput, "[%u] User \"%s\" began downloading \"%s\".", sCmd, strUser.c_str(), strNewVirtual.c_str());
							//DbgPrint(szOutput);
							if (DoSocketFileIO(sCmd, sData, hFile, SOCKET_FILE_IO_DIRECTION_SEND, &dw)) {
								sprintf_s(szOutput, "226 \"%s\" transferred successfully.\r\n", strNewVirtual.c_str());
								SocketSendString(sCmd, szOutput);
								sprintf_s(szOutput, "[%u] Download completed.", sCmd);
								//DbgPrint(szOutput);
							} else {
								SocketSendString(sCmd, "426 Connection closed; transfer aborted.\r\n");
								if (dw) SocketSendString(sCmd, "226 ABOR command successful.\r\n");
								sprintf_s(szOutput, "[%u] Download aborted.", sCmd);
								//DbgPrint(szOutput);
							}
							NetDll_closesocket(XNCALLER_SYSAPP, sData);
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
							sprintf_s(szOutput, "[%u] User \"%s\" began uploading \"%s\".", sCmd, strUser.c_str(), strNewVirtual.c_str());
							//DbgPrint(szOutput);
							if (DoSocketFileIO(sCmd, sData, hFile, SOCKET_FILE_IO_DIRECTION_RECEIVE, 0)) {
								sprintf_s(szOutput, "226 \"%s\" transferred successfully.\r\n", strNewVirtual.c_str());
								SocketSendString(sCmd, szOutput);
								sprintf_s(szOutput, "[%u] Upload completed.", sCmd);
								//DbgPrint(szOutput);
							} else {
								SocketSendString(sCmd, "426 Connection closed; transfer aborted.\r\n");
								sprintf_s(szOutput, "[%u] Upload aborted.", sCmd);
								//DbgPrint(szOutput);
							}
							NetDll_closesocket(XNCALLER_SYSAPP, sData);
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
				NetDll_closesocket(XNCALLER_SYSAPP, sPasv);
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
							sprintf_s(szOutput, "[%u] User \"%s\" deleted \"%s\".", sCmd, strUser.c_str(), strNewVirtual.c_str());
							//DbgPrint(szOutput);
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
						sprintf_s(szOutput, "[%u] User \"%s\" renamed \"%s\" to \"%s\".", sCmd, strUser.c_str(), strRnFr.c_str(), strNewVirtual.c_str());
						//DbgPrint(szOutput);
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
						sprintf_s(szOutput, "[%u] User \"%s\" created directory \"%s\".", sCmd, strUser.c_str(), strNewVirtual.c_str());
						//DbgPrint(szOutput);
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
						sprintf_s(szOutput, "[%u] User \"%s\" removed directory \"%s\".", sCmd, strUser.c_str(), strNewVirtual.c_str());
						//DbgPrint(szOutput);
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

		else {
			sprintf_s(szOutput,"500 Syntax error, command \"%s\" unrecognized.\r\n",szCmd);
			SocketSendString(sCmd,szOutput);
		}

	}

	if (sPasv) NetDll_closesocket(XNCALLER_SYSAPP, sPasv);
	NetDll_closesocket(XNCALLER_SYSAPP, sCmd);

	if (isLoggedIn) {
		dwActiveConnections--;
	}

	//sprintf_s(szOutput,"[%u] Connection closed.",sCmd);
	//DbgPrint(szOutput);

	return false;
}

bool SocketSendString(SOCKET s, const char *psz)
{
	if (NetDll_send(XNCALLER_SYSAPP, s,psz,(INT)strlen(psz),0)==SOCKET_ERROR) return false;
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
		dw=NetDll_select(XNCALLER_SYSAPP, 0,&fds,0,0,&tv);
		if (dw==SOCKET_ERROR || dw==0) return 0xFFFFFFFF;//-1; // Timeout
		dw=NetDll_recv(XNCALLER_SYSAPP, s,psz,1,0);
		if (dw==SOCKET_ERROR || dw==0) return 0xFFFFFFFF;//-1; // Network error
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
	dw=NetDll_select(XNCALLER_SYSAPP, 0,&fds,0,0,&tv);
	if (dw==SOCKET_ERROR || dw==0) return 0xFFFFFFFF;//-1; // Timeout
	dw=NetDll_recv(XNCALLER_SYSAPP, s, psz, dwBytesToRead, 0);
	if (dw==SOCKET_ERROR) return 0xFFFFFFFF;//-1; // Network error
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
		dw=NetDll_select(XNCALLER_SYSAPP, 0,&fds,0,0,&tv);
		if (dw && dw!=SOCKET_ERROR) {
			dw=sizeof(SOCKADDR_IN);
			sData=NetDll_accept(XNCALLER_SYSAPP, *psPasv,(SOCKADDR *)psaiData,(int *)&dw);
		} else {
			sData=0;
		}
		NetDll_closesocket(XNCALLER_SYSAPP, *psPasv);
		*psPasv=0;
		return sData;
	} else {
		sData=NetDll_socket(XNCALLER_SYSAPP, AF_INET,SOCK_STREAM, IPPROTO_TCP);
		PatchSocket( sData );
		if (NetDll_connect(XNCALLER_SYSAPP, sData,(SOCKADDR *)psaiData,sizeof(SOCKADDR_IN))) {
			NetDll_closesocket(XNCALLER_SYSAPP, sData);
			return false;
		} else {
			return sData;
		}
	}
}

void LookupHost(IN_ADDR ia, char *pszHostName, size_t stHostName)
// Performs a reverse DNS lookup on ia. If no host name could be resolved, or
// if LookupHosts is Off, pszHostName will contain a string representation of
// the given IP address.
{
	//strcpy_s( pszHostName, 10, "somewhere" );
	//DWORD add = (DWORD)ia.S_un.S_addr;
	//char* addp = (char*) add;
	//sprintf_s(pszHostName, stHostName, "%d:%d:%d:%d",addp[0],addp[1],addp[2],addp[3]);
	//sprintf_s(pszHostName, stHostName, "%d.%d.%d.%d",ia.S_un.S_un_b.s_b1, ia.S_un.S_un_b.s_b2, ia.S_un.S_un_b.s_b3, ia.S_un.S_un_b.s_b4);
	sprintf_s(pszHostName, stHostName, "%d.%d.%d.%d",ia.s_net, ia.s_host, ia.s_lh, ia.s_impno);
	// ia.S_un.S_un_b.s_b1, ia.S_un.S_un_b.s_b2, ia.S_un.S_un_b.s_b3, ia.S_un.S_un_b.s_b4
}

bool DoSocketFileIO(SOCKET sCmd, SOCKET sData, HANDLE hFile, DWORD dwDirection, DWORD *pdwAbortFlag)
{
	char szBuffer[PACKET_SIZE_RECV];
	DWORD dw;

	if (pdwAbortFlag) *pdwAbortFlag = 0;
	switch (dwDirection) {
	case SOCKET_FILE_IO_DIRECTION_SEND:
		for (;;) {
			if (!ReadFile(hFile, szBuffer, PACKET_SIZE_SEND, &dw, 0)) return false;
			if (!dw) return true;
			if (NetDll_send(XNCALLER_SYSAPP, sData, szBuffer, dw, 0) == SOCKET_ERROR) return false;
			NetDll_ioctlsocket(XNCALLER_SYSAPP, sCmd, FIONREAD, &dw);
			if (dw) {
				SocketReceiveString(sCmd, szBuffer, 511);
				if (!_stricmp(szBuffer, "ABOR")) {
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
			dw = SocketReceiveData(sData, szBuffer, PACKET_SIZE_RECV);
			if (dw == 0xFFFFFFFF) return false;
			if (dw == 0) return true;
			if (!WriteFile(hFile, szBuffer, dw, &dw, 0)) return false;
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
		if (!dw || (dw && !dwBytesRead && !dwCount)) return 0xFFFFFFFF;//-1;
		if (!dwBytesRead || *pszBuf=='\n') break;
		if (*pszBuf!='\r') {
			dwCount++;
			if (dwCount<dwBufLen) pszBuf++;
		}
	}
	*pszBuf=0;

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
	BOOL bBroadcast = TRUE;
	if( NetDll_setsockopt(XNCALLER_SYSAPP, sock, SOL_SOCKET, 0x5802, (PCSTR)&bBroadcast, sizeof(BOOL) ) != 0 )//PATCHED!
	{
		DbgPrint( "Failed to set socket to 5802, error\n");
		return FALSE;
	}

	if( NetDll_setsockopt(XNCALLER_SYSAPP, sock, SOL_SOCKET, 0x5801, (PCSTR)&bBroadcast, sizeof(BOOL) ) != 0 )//PATCHED!
	{
		DbgPrint( "Failed to set socket to 5801, error\n");
		return FALSE;
	}	
	return TRUE;
}

