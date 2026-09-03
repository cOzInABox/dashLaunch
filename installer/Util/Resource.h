#pragma once
#include <xtl.h>
#include "xkelib.h"

class Resource
{
public:	
	static Resource& GetInstance(){static Resource singleton; return singleton;}
	PWCHAR GetBasePath() {return m_basePath;}
	PWCHAR GetFilePath(PWCHAR fileName);
	BOOL GetEmbeddedFile(PCHAR name, PVOID* dataAddr, PDWORD size);
	BOOL IsXzpResourceExist(PWCHAR resourceName);
	PVOID LoadExternalFile(PCHAR szPath, PDWORD size);

private:
	HANDLE m_selfHandle;
	BOOL m_useExternal;
	PBYTE m_externalData;
	DWORD m_extDataSize;
	WCHAR m_basePath[128];
	DWORD m_language;
	LPCWSTR m_locale;

	Resource();
	~Resource() {}
	Resource(const Resource&);                 // Prevent copy-construction
	Resource& operator=(const Resource&);      // Prevent assignment
};


