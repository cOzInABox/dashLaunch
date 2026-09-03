#pragma once
#include <xtl.h>
#include "xkelib.h"
#include "../usvr/nsvrExp.h"

class Updsvr
{
public:	
	static Updsvr& GetInstance(){static Updsvr singleton; return singleton;}
	BOOL usvrCanUse(void){return canUse;}
	BOOL usvrLoad(void);
	BOOL startup(void);
	void shutdown(void);
	DWORD status(void);
	DWORD getConType(void);
	BOOL getPatches(void* buf, int blen);
	BOOL setPatches(void* buf, int blen);

private:
	BOOL Updsvr::usvrResFuncts(HMODULE hMod);
	BOOL canUse;
	Updsvr();
	~Updsvr() {}
	Updsvr(const Updsvr&);                 // Prevent copy-construction
	Updsvr& operator=(const Updsvr&);      // Prevent assignment
};


