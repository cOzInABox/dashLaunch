#include "Strings.h"
#include <string>
#include <cstring>
#include "Resource.h"

using namespace std;

Strings::Strings()
{
	PWCHAR infolist = Resource::GetInstance().GetFilePath(L"strings.xus");
	if(XuiLoadStringTableFromFile(infolist, &m_hStrings) != S_OK)
		m_hStrings = (HXUISTRINGTABLE)INVALID_HANDLE_VALUE;
	delete [] infolist;
}

const WCHAR empty[] = L" ";
LPCWSTR Strings::Lookup(PWCHAR name)
{
	if(m_hStrings != (HXUISTRINGTABLE)INVALID_HANDLE_VALUE)
	{
		LPCWSTR info = XuiLookupStringTable(m_hStrings, name);
		if(info != NULL)
			return info;
	}
	return empty;
}

PWCHAR Strings::Look(PWCHAR name)
{
	if(m_hStrings != (HXUISTRINGTABLE)INVALID_HANDLE_VALUE)
	{
		LPCWSTR info = XuiLookupStringTable(m_hStrings, name);
		if(info != NULL)
			return (PWCHAR)info;
	}
	return (PWCHAR)empty;
}


LPCWSTR Strings::Lookup(PWCHAR basename, PWCHAR tailname)
{
	wstring name(basename);
	name += tailname;
	if(m_hStrings != (HXUISTRINGTABLE)INVALID_HANDLE_VALUE)
	{
		LPCWSTR info = XuiLookupStringTable(m_hStrings, name.c_str());
		if(info != NULL)
			return info;
	}
	return empty;
}

PWCHAR Strings::Look(PWCHAR basename, PWCHAR tailname)
{
	wstring name(basename);
	name += tailname;
	if(m_hStrings != (HXUISTRINGTABLE)INVALID_HANDLE_VALUE)
	{
		LPCWSTR info = XuiLookupStringTable(m_hStrings, name.c_str());
		if(info != NULL)
			return (PWCHAR)info;
	}
	return (PWCHAR)empty;
}

VOID Strings::LookupAndSetChild(CXuiScene obj, PWCHAR child, PWCHAR sname)
{
	LPCWSTR tmp = Lookup(sname);
	CXuiTextElement tob;
	obj.GetChildById(child, &tob);
	tob.SetText(tmp);
}