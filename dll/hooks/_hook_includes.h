#ifndef __HOOK_INCLUDES_H
#define __HOOK_INCLUDES_H

#define _DISABLE_HOOK_DEBUG		1

/* ***** files that the hooks themselves will always include... ***** */

#include <xtl.h>
#include "../../_common.h"
#include "xkelib.h"
#include "../utility.h"
#include "../launch.h"
#include "except.h"

/* ***** debug toggles for hooks ***** */
#ifndef _DISABLE_HOOK_DEBUG
// 	#define DEBUG_EXCEPT_OUT		1
// 	#define DEBUG_LAUNCHTITLEEXOUT	1
// 	#define DEBUG_LOADPREP_OUT		1
// 	#define DEBUG_PRIVS_OUT			1
// 	#define DEBUG_SIGNINSTATE_OUT	1
//	#define DEBUG_XAMAPPLOAD_OUT	1
#define DEBUG_XAMLIC_OUT		1
// 	#define DEBUG_XNETDNS_OUT		1
// 	#define DEBUG_XNOTIFYBCAST_OUT	1
// 	#define DEBUG_XSECURITY_OUT		1
	//#define DEBUG_MULTIDISK_OUT			1
	/* ***** verbose output of the rule searches ***** */

	#ifdef DEBUG_XNETDNS_OUT
		//#define DEBUG_XNETDNS_OUT_RULES 1
	#endif
#endif

#endif // __HOOK_INCLUDES_H
