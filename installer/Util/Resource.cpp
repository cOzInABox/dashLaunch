#include <xtl.h>
#include <xui.h>
#include <xuiapp.h>
#include "Resource.h"
#include "XboxUtil.h"
#include "logging.h"

#define MAX_LANG_RESOURCE 13
LPCWSTR wLocale[MAX_LANG_RESOURCE] =
{
	L"en-en", // the default locale
	L"en-en", // English
	L"ja-jp", // Japanese
	L"de-de", // German
	L"fr-fr", // French
	L"es-es", // Spanish
	L"it-it", // Italian
	L"ko-kr", // Korean
	L"zh-cht",// Traditional Chinese
	L"pt-br",  // Portuguese
	L"zh-chs", // simplified chinese
	L"pl-pl", // polish
	L"ru-ru", // russian
};

Resource::Resource()
{
	m_useExternal = FALSE;
	XexGetModuleHandle(NULL, &m_selfHandle);
	m_externalData = (PBYTE)LoadExternalFile("GAME:\\skin.xzp", &m_extDataSize);
	if(m_externalData != NULL)
	{
		m_useExternal = TRUE;
		wsprintfW(m_basePath, L"memory://%08X,%X#", m_externalData, m_extDataSize);
	}
	else
	{
		wsprintfW(m_basePath, L"section://%X,skin#", m_selfHandle);
	}
	// Get the current language setting from the console
	m_language = XGetLanguage();
	//lDbgPrint("get lang returned %d\n", m_language);
	if(m_language > 1)
	{
		if(m_language >= MAX_LANG_RESOURCE)
		{
			// Use default locale if out of bounds
			m_language = 0;
		}
		// Tell XUI what the locale is
		m_locale = wLocale[m_language];
		lDbgPrint("Setting locale to %S\n", m_locale);
		XuiSetLocale(m_locale);
	}
}

PWCHAR Resource::GetFilePath(PWCHAR fileName)
{
	PWCHAR rval = new WCHAR[MAX_PATH];
	wsprintfW(rval, L"%s%s", m_basePath, fileName);
	return rval;
}

BOOL Resource::IsXzpResourceExist(PWCHAR resourceName)
{
	BOOL ret = FALSE;
	HXUIRESOURCE hpk;
	BOOL isMem;

	HRESULT hr = XuiResourceOpenNoLoc(resourceName, &hpk, &isMem);
	//lDbgPrint("resource open returned %x for %S ismem %d\n", hr, resourceName, isMem);
	if(hr == S_OK)
	{
		XuiResourceClose(hpk);
		ret = TRUE;
	}
	return ret;
}

BOOL Resource::GetEmbeddedFile(PCHAR name, PVOID* dataAddr, PDWORD size)
{
	return XGetModuleSection(m_selfHandle, name, dataAddr, size);
}

PVOID Resource::LoadExternalFile(PCHAR szPath, PDWORD size)
{
	if(XboxUtil::GetInstance().IsFileExist(szPath))
	{
		DWORD fsize, bRead;
		PBYTE data;
		HANDLE fHand = CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		fsize = GetFileSize(fHand, NULL);
		data = (PBYTE)VirtualAlloc(NULL, fsize, MEM_COMMIT|MEM_LARGE_PAGES, PAGE_READWRITE);
		if(data != NULL)
		{
			ReadFile(fHand, data, fsize, &bRead, NULL);
			*size = fsize;
		}
		CloseHandle(fHand);
		return data;
	}
	return NULL;
}