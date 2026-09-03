#include "_hook_includes.h"

#define XAM_OBFUSCATE_ORD		596
#define XAM_UNOBFUSCATE_ORD		597
#define XAM_XEKYESHMAC_ORD		585

static BYTE devroamkey[] = {0xDA, 0xB6, 0x9A, 0xD9, 0x8E, 0x28, 0x76, 0x4F, 0x97, 0x7E, 0xE2, 0x48, 0x7E, 0x4F, 0x3F, 0x68};
// XAMACCOUNTINFO 0x17C
BOOL XamObfuscateHook(XEKEY_OBFUSCATE keySel, const PBYTE pbInp1, DWORD cbInp1, PBYTE pbOut, PDWORD cbOut)
{
	if((keySel == XEKEY_OBFUSCATE_ROAM)&&(cbInp1 == 0x17C))
	{
		PXAMACCOUNTINFO acct = (PXAMACCOUNTINFO)pbInp1;
		if(acct->szOnlineKerberosRealm[0x16] == '*')
		{
			BYTE digest[0x10];
			//DbgPrint("dev account encryption spotted!!\n");
			acct->szOnlineKerberosRealm[0x16] = 0;
			memcpy(&cbOut[0x18], pbInp1, cbInp1);
			XeCryptRandom(&pbOut[0x10], 8);
			*cbOut = cbInp1+0x18;
			XeCryptHmacSha(devroamkey, 0x10, &pbOut[0x10], (*cbOut-0x10), NULL, 0, NULL, 0, pbOut, 0x10);
			XeCryptHmacSha(devroamkey, 0x10, pbOut, 0x10, NULL, 0, NULL, 0, digest, 0x10);
			XeCryptRc4(digest, 0x10, &pbOut[0x10], *cbOut-0x10);
			return TRUE;
		}
	}
	return XeKeysObfuscate(keySel, pbInp1, cbInp1, pbOut, cbOut);
}

BOOL XamUnObfuscateHook(XEKEY_OBFUSCATE keySel, const PBYTE pbInp1, DWORD cbInp1, PBYTE pbOut, PDWORD cbOut)
{
	BOOL ret = XeKeysUnObfuscate(keySel, pbInp1, cbInp1, pbOut, cbOut);
	//DbgPrint("unobfuscate key %d pbin %08x cbin %x pbout %08x cbout %x\n", keySel, pbInp1, cbInp1, pbOut, cbOut);
	if((keySel == XEKEY_OBFUSCATE_ROAM)&&(ret == FALSE))
	{
		if(cbInp1 == 0x194) // encrypted account size
		{
			BYTE hashInfo[0x18];
			BYTE digest[0x10];
			XECRYPT_RC4_STATE xst;
			//DbgPrint("possible dev profile spotted!\n");
			memcpy(hashInfo, pbInp1, 0x18);
			*cbOut = cbInp1-0x18;
			memcpy(pbOut, &pbInp1[0x18], *cbOut);
			XeCryptHmacSha(devroamkey, 0x10, hashInfo, 0x10, NULL, 0, NULL, 0, digest, 0x10);
			XeCryptRc4Key(&xst, digest, 0x10);
			XeCryptRc4Ecb(&xst, &hashInfo[0x10], 8); // decrypt the result
			XeCryptRc4Ecb(&xst, pbOut, *cbOut); // decrypt the profile (hopefully)
			XeCryptHmacSha(devroamkey, 0x10, &hashInfo[0x10], 0x8, pbOut, *cbOut, NULL, 0, digest, 0x10);
			if(memcmp(hashInfo, digest, 0x10) == 0)
			{
				PXAMACCOUNTINFO acct = (PXAMACCOUNTINFO)pbOut;
				//DbgPrint("decrypted dev account %S OK\n", acct->szGamerTag);
				if((acct->szOnlineKerberosRealm[0x15] == 0)&&(acct->szOnlineKerberosRealm[0x16] == 0)&&(acct->szOnlineKerberosRealm[0x17] == 0))
				{
					acct->szOnlineKerberosRealm[0x16] = '*';
					ret = TRUE;
				}
			}
		}
	}
	return ret;
}

static IMPORT_HOOK_SAVE obfuscateSave;
static IMPORT_HOOK_SAVE unobfuscateSave;
static BOOL isObfuscateHooked = FALSE;
void xamObfuscateHook(void)
{
	if(!isObfuscateHooked)
	{
		if(hookImpStub(MODULE_XAM, MODULE_KERNEL, XAM_OBFUSCATE_ORD, (DWORD)XamObfuscateHook, &obfuscateSave))
		{
			if(hookImpStub(MODULE_XAM, MODULE_KERNEL, XAM_UNOBFUSCATE_ORD, (DWORD)XamUnObfuscateHook, &unobfuscateSave))
			{
				isObfuscateHooked = TRUE;
			}
			else
			{
				//DbgPrint("Error hooking xam unobfuscate!!\n");
				unhookImpStub(&obfuscateSave);
			}
		}
		//else
			//DbgPrint("Error hooking xam obfuscate!!\n");
	}
}

void xamObfuscateUnhook(void)
{
	if(isObfuscateHooked)
	{
		unhookImpStub(&obfuscateSave);
		unhookImpStub(&unobfuscateSave);
		isObfuscateHooked = FALSE;
	}
}

/* ******************** xam xekeyshmacsha hook ********************* */
BOOL XamKeysHmacHook(XEKEY_INDEX keySel, PBYTE pbInp1, DWORD cbInp1, PBYTE pbInp2, DWORD cbInp2, PBYTE pbInp3, DWORD cbInp3, PBYTE pbOut, DWORD cbOut)
{
	BOOL ret = FALSE;
	if(keySel == XEKEY_ROAMABLE_OBFUSCATION_KEY)
	{
		XeCryptHmacSha(devroamkey, 0x10, pbInp1, cbInp1, pbInp2, cbInp2, pbInp3, cbInp3, pbOut, cbOut);
		ret = TRUE;
	}
	else // call original
	{
		ret = XeKeysHmacSha(keySel, pbInp1, cbInp1, pbInp2, cbInp2, pbInp3, cbInp3, pbOut, cbOut);
	}
	return ret;
}

static IMPORT_HOOK_SAVE keysHmacSave;
static BOOL isKeysHmacHooked = FALSE;

void xamObfuscateSyslinkHook(void)
{
	if(!isKeysHmacHooked)
	{
		if(hookImpStub(MODULE_XAM, MODULE_KERNEL, XAM_XEKYESHMAC_ORD, (DWORD)XamKeysHmacHook, &keysHmacSave))
			isKeysHmacHooked = TRUE;
	}
}

void xamObfuscateSyslinkUnhook(void)
{
	if(isKeysHmacHooked)
	{
		unhookImpStub(&keysHmacSave);
		isKeysHmacHooked = FALSE;
	}
}

