#ifndef __GENERIC_PATCHES_H
#define __GENERIC_PATCHES_H

#include "launch.h"

void genericPingPatch(PDWORD adr);
void genericPingUnpatch(void);

void genericShowHudPatch(PDWORD adr);
void genericShowHudUnpatch(void);

void genericRevokeCheckPatch(PDWORD adr);
void genericRevokeCheckUnpatch(void);

void genericXhttpPatch(PXHTTP_PATCH_ADDR pats);
void genericXhttpUnpatch(void);

void genericNetworkStorageHidePatch(void);
void genericNetworkStorageHideUnpatch(void);

void genericNxeInstallPatch(PDWORD XamContLivePirs, PDWORD XamContDevice);
void genericNxeInstallUnpatch(void);

void genericKHealthPatch(void);
void genericKHealthUnpatch(void);
void genericKHealthSetAddr(PDWORD addr);

void genericNoNewUpdateSetAddr(PDWORD addr);
void genericNoNewUpdatePatch(void);
void genericNoNewUpdateUnpatch(void);

#endif // __GENERIC_PATCHES_H
