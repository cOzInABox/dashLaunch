#ifndef _SWAP_H
#define _SWAP_H

#define MOUNT_NAME	"swp:"
#define INI_NAME	"swp:\\multi.ini"

typedef struct _NumDevEnum {
	const char* mountPoint;
	int startdev;
	int numdev;
	int stlen;
} NumDevEnum;

enum{
	SWAP_FIND_FAILED = 0,
	SWAP_SAME_DISK,
	SWAP_READY,
	SWAP_GOD_READY,
};

enum{
	MOUNT_MASS0 = 0,	// raw usb devices
	MOUNT_HDD = 3,		// hard disk
	MOUNT_SFC = 4,		// flash memory unit big block
	MOUNT_INTUSB = 5,	// trinity internal usb
	MOUNT_MU0 = 6,		// memory units
	MOUNT_USBMU0 = 8,	// usb memory units
	MOUNT_MAX_ITEMS = 11
};

const char* mountPaths[MOUNT_MAX_ITEMS] =
{
	"\\Device\\Mass0\\",
	"\\Device\\Mass1\\",
	"\\Device\\Mass2\\",
	"\\Device\\Harddisk0\\Partition1\\",
	"\\Device\\BuiltInMuSfc\\", // flash memory unit big block
	"\\Device\\BuiltInMuUsb\\Storage\\", // flash memory unit slim arcade internal
	"\\Device\\Mu0\\",
	"\\Device\\Mu1\\",
	"\\Device\\Mass0PartitionFile\\Storage\\",
	"\\Device\\Mass1PartitionFile\\Storage\\",
	"\\Device\\Mass2PartitionFile\\Storage\\",
};

#define DEVS_MAX 6
NumDevEnum numDevs[DEVS_MAX] =
{
	{"Usb:",		MOUNT_MASS0,	3, (strlen(numDevs[0].mountPoint)-1)},
	{"Hdd:",		MOUNT_HDD,		1, (strlen(numDevs[1].mountPoint)-1)},
	{"FlashMu:",	MOUNT_SFC,		1, (strlen(numDevs[2].mountPoint)-1)}, // flash memory unit big block
	{"IntMu:",		MOUNT_INTUSB,	1, (strlen(numDevs[3].mountPoint)-1)}, // flash memory unit slim arcade internal
	{"Mu:",			MOUNT_MU0,		2, (strlen(numDevs[4].mountPoint)-1)},
	{"UsbMu:",		MOUNT_USBMU0,	3, (strlen(numDevs[5].mountPoint)-1)},
};


#endif // _SWAP_H
