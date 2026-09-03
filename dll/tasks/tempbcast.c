#include "_task_includes.h"

//#define CUR_VER (((XboxKrnlVersion->Major&0xF)<<28) | ((XboxKrnlVersion->Minor)<<24) | ((XboxKrnlVersion->Build &0xFFFF)<<8) | ((XboxKrnlVersion->Qfe&0xFF)))
BOOL isSendRdy = FALSE; // whether the netlink is ready
BOOL isSending = FALSE; // whether to broadcast temp
DWORD tempUsePort = 7030;
SOCKET tempSend;
SOCKADDR_IN tempSendSocketAddr;
HXAMTASK tempsXamTask = INVALID_HANDLE_VALUE;
extern BYTE tempdata[0x10];
extern BYTE g_PowerOnReason;

#define MESSAGE_TEMPDATA_SZ	0x10
#define MESSAGE_PE_MAX_SZ	0x50
#define MESSAGE_COMMAND_SZ	0x100

#define NO_PATH_FOUND	"\\no\\title"
#define NO_PE_FOUND		"noxex"

#pragma pack(push, 1)
typedef struct _TEMP_MESSAGE {
	BYTE smcMsg[MESSAGE_TEMPDATA_SZ]; // 0x0 sz 0x10
	char peName[MESSAGE_PE_MAX_SZ]; // 0x10 sz 0x50
	char imagePath[MESSAGE_COMMAND_SZ]; // 0x60 sz 0x100
	DWORD Tid; // 0x160 sz 0x4
	DWORD Mid; // 0x164 sz 0x4
	BYTE PwrRsn; // 0x168
	BYTE padding[0x97]; // pad to 0x200
}TEMP_MESSAGE, *PTEMP_MESSAGE;
C_ASSERT(sizeof(TEMP_MESSAGE) == 0x200);
#pragma pack(pop)

static TEMP_MESSAGE tmsg;

void sendTemp(void)
{
	BYTE smcMsg[MESSAGE_TEMPDATA_SZ];
	BOOL fakeIt = TRUE;
	ZeroMemory(&tmsg, sizeof(TEMP_MESSAGE));
	ZeroMemory(smcMsg, MESSAGE_TEMPDATA_SZ);
	smcMsg[0] = smc_query_sensor;
	HalSendSMCMessage(smcMsg, tmsg.smcMsg);

	if(tmsg.smcMsg[0] == smc_query_sensor)
	{
		memcpy(tempdata, tmsg.smcMsg, MESSAGE_TEMPDATA_SZ);
		if(isSending)
		{
			PLDR_DATA_TABLE_ENTRY ldat;
			char* imgPath = (char*)resolveFunct(MODULE_KERNEL, 431);
			if(XexGetModuleHandle(NULL, (PHANDLE)&ldat) == 0)
			{
				if(ldat->XexHeaderBase != NULL)
				{
					PXEX_HEADER_STRING peName = (PXEX_HEADER_STRING)RtlImageXexHeaderField(ldat->XexHeaderBase, XEX_HEADER_PE_MODULE_NAME);
					PXEX_EXECUTION_ID exId = (PXEX_EXECUTION_ID)RtlImageXexHeaderField(ldat->XexHeaderBase, XEX_HEADER_EXECUTION_ID);
					if(peName != NULL)
					{
						if(peName->Data != NULL)
						{
							//DbgPrint("peName->Data: %s len: %d\n",peName->Data, peName->Size);
							if(peName->Size >= (MESSAGE_PE_MAX_SZ-1))
								memcpy(tmsg.peName,peName->Data, (MESSAGE_PE_MAX_SZ-1));
							else
								memcpy(tmsg.peName,peName->Data, peName->Size);
							fakeIt = FALSE;
						}
						//else
							//DbgPrint("peName->Data is NULL\n");
					}
					if(exId != NULL)
					{
						tmsg.Tid = exId->TitleID;
						tmsg.Mid = exId->MediaID;

					}
					//else
						//DbgPrint("peName is NULL\n");
				}
				//else
					//DbgPrint("ldat->XexHeaderBase is %08x\n", ldat->XexHeaderBase);
			}
			//else
				//DbgPrint("XexGetModuleHandle failed %08x\n", ldat);
			if(fakeIt)
				memcpy(tmsg.peName, NO_PE_FOUND, 6);
			fakeIt = TRUE;

			if(imgPath)
			{
				if((imgPath[0]&0xFF) != 0)
				{
					//DbgPrint("img: %s\n", imgPath);
					memcpy(tmsg.imagePath, imgPath, MESSAGE_COMMAND_SZ);
					fakeIt = FALSE;
				}
			}
			//else
			//	DbgPrint("noimgpath???\n");
			if(fakeIt)
				memcpy(tmsg.imagePath, NO_PATH_FOUND, 10); // copy "\\no\\title" plus NULL
			tmsg.PwrRsn = g_PowerOnReason;
			NetDll_sendto(XNCALLER_SYSAPP, tempSend, &tmsg, sizeof(TEMP_MESSAGE), 0, &tempSendSocketAddr, sizeof(SOCKADDR_IN));
		}
// 		double cpu = (smcresp[1] | (smcresp[2] << 8)) / 256.0;
// 		double gpu = (smcresp[3] | (smcresp[4] << 8)) / 256.0;
// 		double edram = (smcresp[5] | (smcresp[6] << 8)) / 256.0;
// 		double mb = (smcresp[7] | (smcresp[8] << 8)) / 256.0;
// 		DbgPrint("got smc msg CPU:%3.1fC GPU:%3.1fC EDRAM:%3.1fC MB:%3.1fC\n", cpu, gpu, edram, mb );
// 		if(NetDll_sendto(XNCALLER_SYSAPP, tempSend, smcresp, 0x10, 0, &tempSendSocketAddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
// 			DbgPrint("sendto error %d\n", WSAGetLastError());
	}
// 	else
// 		DbgPrint("message wasn't expected, reply type 0x%x", smcresp[0]);
}

void endTempSend(void)
{
	NetDll_closesocket(XNCALLER_SYSAPP, tempSend);
	isSendRdy = FALSE;
}

BOOL startTempSend(void)
{
	BOOL bBroadcast = TRUE;
	tempSend = NetDll_socket(XNCALLER_SYSAPP, AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(NetDll_setsockopt(XNCALLER_SYSAPP, tempSend, SOL_SOCKET, SO_PRIVATE, (PCSTR)&bBroadcast, sizeof(BOOL) ) != 0 )//PATCHED!
	{
// 		DbgPrint("Failed to set debug send socket SO_PRIVATE, error\n");
		return FALSE;
	}

	if(NetDll_setsockopt(XNCALLER_SYSAPP, tempSend, SOL_SOCKET, SO_MARKINSECURE, (PCSTR)&bBroadcast, sizeof(BOOL) ) != 0 )//PATCHED!
	{
// 		DbgPrint("Failed to set debug send socket SO_MARKINSECURE, error\n");
		return FALSE;
	}

	if(NetDll_setsockopt(XNCALLER_SYSAPP, tempSend, SOL_SOCKET, SO_BROADCAST, (PCSTR)&bBroadcast, sizeof(BOOL) ) != 0 )
	{
// 		DbgPrint("Failed to set debug send socket to SO_BROADCAST, error\n");
		return FALSE;
	}

	tempSendSocketAddr.sin_family = AF_INET;
	tempSendSocketAddr.sin_addr.s_addr = INADDR_BROADCAST;
	tempSendSocketAddr.sin_port = htons(tempUsePort);
// 	DbgPrint("network ready\n");
	return TRUE;
}

void sendTempTask(void)
{
	// check if there is a link...
	//DWORD sta  = NetDll_XNetGetEthernetLinkStatus(XNCALLER_SYSAPP);
	BOOL sta = ((NetDll_XNetGetEthernetLinkStatus(XNCALLER_SYSAPP)&XNET_ETHERNET_LINK_ACTIVE) != 0);
	if(sta) // if the link is active we need to see if we should send the data
	{
//		DbgPrint("connection found\n");
		if((!isSendRdy)&&(isSending))
			isSendRdy = startTempSend();
	}
	if((isSendRdy)&&(isSending == FALSE)) // if the link has gone away or dl was told to stop sending temps, shut down the socket
	{
//		DbgPrint("ending socket\n");
		endTempSend();
	}
	sendTemp();
}

void scheduleTempBroadcast(void)
{
	if(tempsXamTask == INVALID_HANDLE_VALUE)
	{
		HRESULT hr;
		XAMTASKATTRIBUTES xta;
		RtlZeroMemory(&xta, sizeof(xta)); // XAMPROPERTY_MISC_ONSYSTEMBEHALF
		xta.dwProperties = XAMPROPERTY_TYPE_PERIODIC | XAMPROPERTY_CPUUSAGE_LO | XAMPROPERTY_DURATION_SHORT | XAMPROPERTY_PRI_NORMAL | XAMPROPERTY_MISC_ONSYSTEMBEHALF;
		xta.typ.dwPeriod = (10*1000); // default time of 10s
		hr = XamTaskSchedule((PXAMTASKPROC)sendTempTask, NULL, &xta, &tempsXamTask);
		//if(SUCCEEDED(hr))
		//	dbgPrintFake("tempbcast scheduled at %dms interval\n", xta.typ.dwPeriod);
		//else
		//	dbgPrintFake("failed to schedule tempbcast at %dms intervals\n", xta.typ.dwPeriod);
	}
}

void modifyTempBroadcast(BOOL isOnLan)
{
	isSending = isOnLan;
}

void modifyTempPort(DWORD port)
{
	tempUsePort = port;
	endTempSend();
}

void modifyTempTimer(DWORD timeInS)
{
	if(tempsXamTask != INVALID_HANDLE_VALUE)
	{
		XAMTASKATTRIBUTES xta;
		RtlZeroMemory(&xta, sizeof(xta));
		XamTaskGetAttributes(tempsXamTask, &xta);
		if(timeInS == 0)
			xta.typ.dwPeriod = 1000; // minimum is 1s
		else
			xta.typ.dwPeriod = (timeInS*1000);
		XamTaskModify(tempsXamTask, XAMTASKMODIFY_ATTRIBUTES, NULL, NULL, &xta);
	}
}

void endTempBroadcast(void)
{
	if(tempsXamTask != INVALID_HANDLE_VALUE)
	{
		isSending = FALSE;
		endTempSend(); // kill the socket
		XamTaskCancel(tempsXamTask); // kill the task
		XamTaskCloseHandle(tempsXamTask); // end the handle
		tempsXamTask = INVALID_HANDLE_VALUE;
	}
}
