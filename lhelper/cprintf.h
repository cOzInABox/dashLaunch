#ifndef __cprintf_h
#define __cprintf_h

//int sprintf(char * buf, const char *fmt, ...);
//int printf( const char *fmt, ...);
#ifdef __cplusplus
extern "C" {
#endif
void cprintf(const char* s, ...);
#ifdef __cplusplus
}
#endif

#endif
