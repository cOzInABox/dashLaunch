#ifndef _INIPARSE_H
#define _INIPARSE_H

/*** public functions ***/
// loads ini and does pre-parsing, returns TRUE on success
BOOL iniStart(const char* path);

// finalizes and frees ini resources
void iniEnd();

// gets string pointer, returns NULL on fail
const char* iniGetString(const char* name);

// gets bool value, returns defValue on fail
BOOL iniGetBool(const char* name, BOOL defValue);

// gets u32 hex value, returns defValue on fail
DWORD iniGetDword(const char* name, DWORD defValue);

/** local functions **/
unsigned char myatox(char c);
DWORD myatox32(char* dat, DWORD defValue);
char myatol(char c);
BOOL iniFindLabelOffset(const char* name, PDWORD offset);
DWORD iniReadFile(const char* path, char* buf);
BOOL iniCheckSpaces(DWORD offset);
DWORD iniGetSpaces(DWORD offset);
void iniPreParse(void);

#endif // _INIPARSE_H
