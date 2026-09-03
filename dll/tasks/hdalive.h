#ifndef _HDALIVE_H
#define _HDALIVE_H

void hddKeepAlive(void);
void scheduleHdAliveTask(DWORD timeInS);
void modifyHdAliveTimer(DWORD timeInS);
void endHdAliveTask(void);

#endif // _HDALIVE_H
