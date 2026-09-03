// ************* dummySaveVar hook start *************
// ------------------------------------------------------------------------------------------------------------
// to hook
// #define DUMMY_ORD		0x43// 67
// hookFunctionStartOrd(MODULE_XAM, DUMMY_ORD, (PDWORD)dummySaveVar, (DWORD)dummySaveHook);

typedef VOID (*DUMMYFUN)(DWORD stopCode, PDWORD Parm1, PDWORD Parm2, PDWORD Parm3, PDWORD Parm4);
#define DUMMY_VAL 0xFF

VOID __declspec(naked) dummySaveVar(VOID)
{
	__asm{
		li r3, DUMMY_VAL
		nop
		nop
		nop
		nop
		nop
		nop
		blr
	}
}
DUMMYFUN dummySave = (DUMMYFUN)dummySaveVar;

VOID dummySaveHook(DWORD stopCode, PDWORD Parm1, PDWORD Parm2, PDWORD Parm3, PDWORD Parm4)
{
}
// ------------------------------------------------------------------------------------------------------------
// ************* dummySaveVar hook end *************