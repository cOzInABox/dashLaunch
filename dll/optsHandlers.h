#ifndef __OPTS_HANDLERS_H
#define __OPTS_HANDLERS_H

void optsHandleAutoShut(PDWORD val);
void optsHandleAutoOff(PDWORD val);
void optsHandlePingPatch(PDWORD val);
void optsHandleFatalFreeze(PDWORD val);
void optsHandleFatalReboot(PDWORD val);
void optsHandleSafeReboot(PDWORD val);
void optsHandleRegionSpoof(PDWORD val);
void optsHandleRegion(PDWORD val);
void optsHandleNoHud(PDWORD val);
void optsHandleNoUpdater(PDWORD val);
void optsHandleExchandler(PDWORD val);
void optsHandleLiveBlock(PDWORD val);
void optsHandleLiveStrong(PDWORD val);
void optsHandleSignNotice(PDWORD val);
void optsHandleXhttp(PDWORD val);
void optsHandleFakeLive(PDWORD val);
void optsHandleAutoFake(PDWORD val);
void optsHandleAutoCont(PDWORD val);

void optsHandleHddAlive(PDWORD val);
void optsHandleHddAliveTimer(PDWORD val);
void optsHandleTempBcast(PDWORD val);
void optsHandleTempTime(PDWORD val);
void optsHandleTempPort(PDWORD val);
void optsHandleHideNetStore(PDWORD val);
void optsHandleShutdownTemps(PDWORD val);
void optsHandleDevProfiles(PDWORD val);
void optsHandleDevSyslink(PDWORD val);
void optsHandleMultidisk(PDWORD val);
void optsHandlePreview(PDWORD val);
void optsHandleOobe(PDWORD val);
void optsHandleKhealth(PDWORD val);

#endif // __OPTS_HANDLERS_H
