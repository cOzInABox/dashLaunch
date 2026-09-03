#ifndef _MOUNTS_H
#define _MOUNTS_H

typedef struct _MOUNTS {
	char name[20];
	char path[256];
	BOOL isMounted;
} MOUNTS, *PMOUNTS;

#endif
