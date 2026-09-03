#ifndef _TEMPBCAST_H
#define _TEMPBCAST_H

void scheduleTempBroadcast(void);
void modifyTempBroadcast(BOOL isOnLan);
void modifyTempPort(DWORD port);
void modifyTempTimer(DWORD timeInS);
void endTempBroadcast(void);

#endif // _TEMPBCAST_H
