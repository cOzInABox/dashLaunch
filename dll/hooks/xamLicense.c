#include "_hook_includes.h"

#define CONTENT_ADDON	(0x00000002) // also indie
#define CONTENT_XBLA	(0x000D0000)
#define CONTENT_DEMO	(0x00080000)
#define CONTENT_NXEDISK	(0x00004000)

#define XAM_CONTENT_GET_LIC_MASK_ORD	0x266 // 614 XamContentGetLicenseMask
#define XAM_USER_GET_OL_XUID_ORD		0x4F0 // 1264 XamUserGetOnlineXUIDFromOfflineXUID

typedef DWORD (*LICHOOKSAVEFUN)(PXCONTENT_HEADER xhead, HANDLE hPF, PDWORD respDw, PDWORD unkIo);
typedef DWORD (*LICMASKSAVEFUN)(PDWORD maskout, DWORD unk);
// returns 0 for success, negative for unsucsessful
typedef DWORD (*XAMUSERGETONLINEXUID)(XUID offline, PXUID onlineOut); // ord 1264

XAMUSERGETONLINEXUID XamUserGetOnlineXuid = NULL; // XAM_USER_GET_OL_XUID_ORD

void __declspec(naked) LicenseMaskSaveVar(void)
{
	__asm{
		li r3, LICMASK_VAL
		nop
		nop
		nop
		nop
		nop
		nop
		blr
	}
}
LICMASKSAVEFUN LicenseMaskSave = (LICMASKSAVEFUN)LicenseMaskSaveVar;
DWORD LicenseMaskFun(PDWORD maskout, DWORD unk)
{
	DWORD ret = LicenseMaskSave(maskout, unk);
	if(getOpt(OPT_LIC_PATCH))
	{
#ifdef DEBUG_XAMLIC_OUT
		if(maskout != NULL)
			DbgPrint("licmask: 0x%08x ret 0x%08x unk 0x%08x\n", maskout[0], ret, unk);
		else
			DbgPrint("licmask: (NULL) ret 0x%08x unk 0x%08x\n", ret, unk);
#endif

		// this function returns errors 0x65b (Function failed during execution) and 0x57 (parameter is incorrect)
		if(ret == 0x65b) // this case replace the return info, fixes extracted XBLA default.xex exec
		{
			if(maskout != NULL)
				maskout[0] = 1;
			return 0;
		}
	}
	return ret;
}

void __declspec(naked) LicenseHookSaveVar(void)
{
	__asm{
		li r3, LICHOOK_VAL
		nop
		nop
		nop
		nop
		nop
		nop
		blr
	}
}
LICHOOKSAVEFUN LicenseHookSave = (LICHOOKSAVEFUN)LicenseHookSaveVar;

BOOL getValidOnlineXuid(PXUID dxuid)
{
	XUID xd, olxd;
	DWORD i;
	for(i = 0; i < 4; i++)
	{
		if(XUserGetXUID(i, &xd) == STATUS_SUCCESS)
		{
#ifdef DEBUG_XAMLIC_OUT
			DbgPrint("valid user idx %d XUID: %016I64x\n", i, xd);
#endif
			if(XamUserGetOnlineXuid != NULL)
			{
				if(XamUserGetOnlineXuid(xd, &olxd) == STATUS_SUCCESS)
				{
	#ifdef DEBUG_XAMLIC_OUT
					DbgPrint("valid user idx %d XUID: %016I64x OLXUID: %016I64x \n", i, xd, olxd);
	#endif
					*dxuid = olxd;
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

void patchBitsXBLA(PXCONTENT_LICENSE lic)
{
	if(lic->LicenseeId.AsULONGLONG == XCONTENT_UNRESTRICTED_LICENSEE)
	{
		if(lic->LicenseBits == 0)
			lic->LicenseBits = 1;
		return;
	}
	else
	{
		switch(lic->LicenseeId.Bits.Type)
		{
			case LICENSEE_TYPE_XUID:
			// check if the xuid LicenseeId.AsULONGLONG is on this console, if not return 0
			// if bit XCONTENT_LICENSE_FLAG_REQUIRE_ONLINE not set return 1
			// else, get signing state and logon result, if not logged on return 0
			case LICENSEE_TYPE_CONSOLE_ID:
			// gets console ID
			// compares it to LicenseeId.AsULONGLONG
			case LICENSEE_TYPE_MEDIA_FLAGS:
			// does ContentComputePackageMediaTypes on PXCONTENT_LICENSE
			// return in variable
			case LICENSEE_TYPE_KEY_VAULT_PRIVILEGES:
			// checks something from hv flag space
			case LICENSEE_TYPE_HV_FLAGS:
			// checks something from hv flag space
			case LICENSEE_TYPE_USER_PRIVILEGES:
			// returns result of VerifyUserPrivilegeLicense
				lic->LicenseeId.AsULONGLONG = XCONTENT_UNRESTRICTED_LICENSEE;
				if(lic->LicenseBits == 0)
					lic->LicenseBits = 1;
				break;
			default:
#ifdef DEBUG_XAMLIC_OUT
				DbgPrint("XBLA unknown license type: 0x%x\n", lic->LicenseeId.Bits.Type);
#endif
				break;
		}
	}
}

void patchBitsDLC(PXCONTENT_LICENSE lic)
{
	PBYTE ptr;
	// calls expansion PV03 with the license content... decrypt?
	if(lic->LicenseeId.AsULONGLONG == XCONTENT_UNRESTRICTED_LICENSEE)
	{
		if(lic->LicenseBits == 0)
			lic->LicenseBits = 1;
		return;
	}
	else
	{
		switch(lic->LicenseeId.Bits.Type)
		{
			case LICENSEE_TYPE_XUID:
				// check if the xuid LicenseeId.AsULONGLONG is on this console, if not return 0
				// if bit XCONTENT_LICENSE_FLAG_REQUIRE_ONLINE not set return 1
				// else, get signin state and logon result, if not logged on return 0
				if(getValidOnlineXuid(&lic->LicenseeId.AsULONGLONG) == FALSE)
				{
					lic->LicenseeId.AsULONGLONG = XCONTENT_UNRESTRICTED_LICENSEE;
					if(lic->LicenseBits == 0)
						lic->LicenseBits = 1;
				}
				break;
			case LICENSEE_TYPE_CONSOLE_ID:
				// gets console ID, compares it to LicenseeId.AsULONGLONG
				ptr = (PBYTE)&lic->LicenseeId.AsULONGLONG;
				ptr+=3;
				XeKeysGetConsoleID(ptr, NULL);
				break;
			case LICENSEE_TYPE_MEDIA_FLAGS:
				// does ContentComputePackageMediaTypes on PXCONTENT_LICENSE, return in variable
			case LICENSEE_TYPE_KEY_VAULT_PRIVILEGES:
				// checks something from hv flag space
			case LICENSEE_TYPE_HV_FLAGS:
				// checks something from hv flag space
			case LICENSEE_TYPE_USER_PRIVILEGES:
				// returns result of VerifyUserPrivilegeLicense
				lic->LicenseeId.AsULONGLONG = XCONTENT_UNRESTRICTED_LICENSEE;
				if(lic->LicenseBits == 0)
					lic->LicenseBits = 1;
				break;
			default:
#ifdef DEBUG_XAMLIC_OUT
				DbgPrint("DLC unknown license type: 0x%x\n", lic->LicenseeId.Bits.Type);
#endif
				break;
		}
	}
}

void patchBitsNXERIP(PXCONTENT_LICENSE lic)
{
	// calls expansion PV03 with the license content... decrypt?
	if(lic->LicenseeId.AsULONGLONG == XCONTENT_UNRESTRICTED_LICENSEE)
		return;
	else
	{
		if(lic->LicenseeId.Bits.Type == LICENSEE_TYPE_CONSOLE_ID)
		{
			PBYTE ptr;
			ptr = (PBYTE)&lic->LicenseeId.AsULONGLONG;
			ptr+=3;
			XeKeysGetConsoleID(ptr, NULL);
		}
#ifdef DEBUG_XAMLIC_OUT
		else
		{
			DbgPrint("NXERIP unknown license type: 0x%x\n", lic->LicenseeId.Bits.Type);
		}
#endif
	}
}

DWORD readContentType(HANDLE hPF)
{
	DWORD ret = 0;
	if(hPF != NULL)
	{
		DWORD dwPos, dwPosRet, bRead;
		LONG dwPosHi = 0;
		dwPos = SetFilePointer(hPF, 0, &dwPosHi, FILE_CURRENT);
		//DbgPrint("current pos hi: %016I64x low: %08x\n", dwPosHi, dwPos);
		if(dwPos != INVALID_SET_FILE_POINTER)
		{
			dwPosRet = SetFilePointer(hPF, 0x344, NULL, FILE_BEGIN);
			if(dwPosRet != INVALID_SET_FILE_POINTER)
			{
				ReadFile(hPF, &ret, 4, &bRead, NULL);
				//DbgPrint("read ctyp as %08x\n", ret);
				dwPosRet = SetFilePointer(hPF, dwPos, &dwPosHi, FILE_BEGIN);
				//if(dwPosRet == INVALID_SET_FILE_POINTER)
				//	DbgPrint("SetFilePointer3 invalid! %08x\n", GetLastError());
			}
			//else
			//	DbgPrint("SetFilePointer2 invalid! %08x\n", GetLastError());
		}
		//else
		//	DbgPrint("SetFilePointer1 invalid! %08x\n", GetLastError());
	}
#ifdef DEBUG_XAMLIC_OUT
	//else
	//	DbgPrint("handle is NULL, can't read ctyp!\n");
#endif
	return ret;
}

BOOL CheckContType(DWORD ctyp)
{
	if(ctyp == CONTENT_NXEDISK)
		return TRUE;
	if(ctyp == CONTENT_ADDON)
		return getOpt(OPT_CONT_PATCH);
	if(ctyp == CONTENT_XBLA)
		return getOpt(OPT_XBLA_PATCH);
	return FALSE;
}

#ifdef DEBUG_XAMLIC_OUT
void showLicDif(int num, PXCONTENT_LICENSE bef, PXCONTENT_LICENSE aft)
{
	BOOL hasDiff = FALSE;
	if(bef->LicenseeId.AsULONGLONG != aft->LicenseeId.AsULONGLONG)
		hasDiff = TRUE;
	else if(bef->LicenseBits != aft->LicenseBits)
		hasDiff = TRUE;
	else if(bef->LicenseFlags != aft->LicenseFlags)
		hasDiff = TRUE;
	if(hasDiff)
	{
		DbgPrint("%02d before: Licensee ID : %016I64x ", num, bef->LicenseeId.AsULONGLONG);
		DbgPrint("ID type: %04x dHi: %04x dLo: %08x ", bef->LicenseeId.Bits.Type, bef->LicenseeId.Bits.DataHi, bef->LicenseeId.Bits.DataLo);
		DbgPrint("LicenseBits : %08x ", bef->LicenseBits);
		DbgPrint("LicenseFlags: %08x\n", bef->LicenseFlags);
		DbgPrint("%02d after : Licensee ID : %016I64x ", num, aft->LicenseeId.AsULONGLONG);
		DbgPrint("ID type: %04x dHi: %04x dLo: %08x ", aft->LicenseeId.Bits.Type, aft->LicenseeId.Bits.DataHi, aft->LicenseeId.Bits.DataLo);
		DbgPrint("LicenseBits : %08x ", aft->LicenseBits);
		DbgPrint("LicenseFlags: %08x\n", aft->LicenseFlags);
	}
}
#endif


DWORD LicenseHook(PXCONTENT_HEADER xhead, HANDLE hPF, PDWORD respDw, PDWORD unkIo)
{
	DWORD ctyp = 0;
#ifdef DEBUG_XAMLIC_OUT
	DWORD ret = 0, t = 0;
#endif
	if(xhead != NULL)
	{
#ifdef DEBUG_XAMLIC_OUT
		t = xhead->SignatureType;
#endif
		//if((xhead->SignatureType == 'LIVE')||(xhead->SignatureType == 'PIRS')) // 'PIRS' 'LIVE' 'CON '
		//{
			ctyp = readContentType(hPF);
			if(CheckContType(ctyp))//||(xhead->ContentType == 0x00009000))
			{
				int i;
#ifdef DEBUG_XAMLIC_OUT
				DbgPrint("\nyaris patching %c%c%c%c header type %08x...\n", ((t>>24)&0xFF), ((t>>16)&0xFF), ((t>>8)&0xFF), (t&0xFF), ctyp);
#endif
				for(i = 0; i < XCONTENT_HEADER_LICENSES; i++)
				{
					if(xhead->LicenseDescriptors[i].LicenseeId.AsULONGLONG != 0)
					{
#ifdef DEBUG_XAMLIC_OUT
						XCONTENT_LICENSE lbef;
						lbef.LicenseBits = xhead->LicenseDescriptors[i].LicenseBits;
						lbef.LicenseFlags = xhead->LicenseDescriptors[i].LicenseFlags;
						lbef.LicenseeId.AsULONGLONG = xhead->LicenseDescriptors[i].LicenseeId.AsULONGLONG;
#endif
						switch(ctyp)
						{
							case CONTENT_NXEDISK:
								patchBitsNXERIP(&xhead->LicenseDescriptors[i]);
								break;
							case CONTENT_XBLA:
								patchBitsXBLA(&xhead->LicenseDescriptors[i]);
								break;
							case CONTENT_ADDON:
								patchBitsDLC(&xhead->LicenseDescriptors[i]);
								break;
							default:
								break;
						}
#ifdef DEBUG_XAMLIC_OUT
						showLicDif(i, &lbef, &xhead->LicenseDescriptors[i]);
// 						DbgPrint("%d before: Licensee ID : %016I64x ", i, xhead->LicenseDescriptors[i].LicenseeId);
// 						DbgPrint("LicenseBits : %08x ", xhead->LicenseDescriptors[i].LicenseBits);
// 						DbgPrint("LicenseFlags: %08x\n", xhead->LicenseDescriptors[i].LicenseFlags);
// 						DbgPrint("%d after : Licensee ID : %016I64x ", i, xhead->LicenseDescriptors[i].LicenseeId);
// 						DbgPrint("LicenseBits : %08x ", xhead->LicenseDescriptors[i].LicenseBits);
// 						DbgPrint("LicenseFlags: %08x\n", xhead->LicenseDescriptors[i].LicenseFlags);
#endif
					}
					else
						i = XCONTENT_HEADER_LICENSES;
				}
			}
#ifdef DEBUG_XAMLIC_OUT
			else
				DbgPrint("\nnot patching %c%c%c%c header type %08x...\n", ((t>>24)&0xFF), ((t>>16)&0xFF), ((t>>8)&0xFF), (t&0xFF), ctyp);
#endif

		//}
	}
#ifdef DEBUG_XAMLIC_OUT
		ret = LicenseHookSave(xhead, hPF, respDw, unkIo);
		if(respDw != NULL)
			DbgPrint("lichook: sigtype %c%c%c%c ty: %08x ret %08x resp %08x\n", ((t>>24)&0xFF), ((t>>16)&0xFF), ((t>>8)&0xFF), (t&0xFF), ctyp, ret, respDw[0]);
		else
			DbgPrint("lichook: sigtype %c%c%c%c ty: %08x ret %08x resp NULL\n", ((t>>24)&0xFF), ((t>>16)&0xFF), ((t>>8)&0xFF), (t&0xFF), ctyp, ret);
		return ret;
#else
	return LicenseHookSave(xhead, hPF, respDw, unkIo);
#endif
}

static PDWORD licenseHookAddr;
static DWORD licenseMaskOld[4];
static DWORD licensHookOld[4];
static BOOL isLicenseHooked = FALSE;
#pragma optimize( "", off )
void xamLicenseHook(PDWORD addr)
{
	if(!isLicenseHooked)
	{
		licenseHookAddr = addr;
		hookFunctionStart(licenseHookAddr, (PDWORD)LicenseHookSaveVar, licensHookOld, (DWORD)LicenseHook);
		hookFunctionStartOrd(MODULE_XAM, XAM_CONTENT_GET_LIC_MASK_ORD, (PDWORD)LicenseMaskSaveVar, licenseMaskOld, (DWORD)LicenseMaskFun);
		XamUserGetOnlineXuid = (XAMUSERGETONLINEXUID)resolveFunct(MODULE_XAM,XAM_USER_GET_OL_XUID_ORD);
		//dbgPrintFake("XamUserGetOnlineXuid = %08x\n", XamUserGetOnlineXuid);
		isLicenseHooked = TRUE;
	}
}
#pragma optimize( "", on ) 

void xamLicenseUnhook(void)
{
	if(isLicenseHooked)
	{
		unhookFunctionStart(licenseHookAddr, licensHookOld);
		unhookFunctionStartOrd(MODULE_XAM, XAM_CONTENT_GET_LIC_MASK_ORD, licenseMaskOld);
		XamUserGetOnlineXuid = NULL;
		isLicenseHooked = FALSE;
	}
}