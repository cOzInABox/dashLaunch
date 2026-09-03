#pragma once
#include <xtl.h>
#include <xui.h>
#include <xuiapp.h>
#include "xkelib.h"

class Strings
{
public:	
	static Strings& GetInstance(){static Strings singleton; return singleton;}
	LPCWSTR Lookup(PWCHAR name);
	PWCHAR Look(PWCHAR name);
	LPCWSTR Lookup(PWCHAR basename, PWCHAR tailname);
	PWCHAR Look(PWCHAR basename, PWCHAR tailname);
	VOID LookupAndSetChild(CXuiScene obj, PWCHAR child, PWCHAR sname);

private:
	HXUISTRINGTABLE m_hStrings;
	
	Strings();
	~Strings() {}
	Strings(const Strings&);                 // Prevent copy-construction
	Strings& operator=(const Strings&);      // Prevent assignment
};


