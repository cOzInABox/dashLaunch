#ifndef _H_SLIMFTPD_
#define _H_SLIMFTPD_

#ifdef __cplusplus
extern "C" {
#endif
	BOOL InitFtpd(USHORT port);
	BOOL StartFtpd(USHORT port);
	VOID StopFtpd();
	VOID FtpdSetDevices();
#ifdef __cplusplus
}
#endif

#endif // _H_SLIMFTPD_
