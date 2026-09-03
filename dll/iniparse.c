// so long as categories aren't an issue
// this will parse a simiplini ascii or utf-8 file with ascii only in it (no multibyte)

#include <xtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xkelib.h"
#include "iniparse.h"

static char* iniData;
static DWORD size;

static unsigned char validChars[] = {
	'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
	'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
	'0','1','2','3','4','5','6','7','8','9','.', ';', '[', ']', '_','-',' ', '=', '\\', ':', '(', ')', '$'
};

// loads ini and does pre-parsing
BOOL iniStart(const char* path)
{
	size = iniReadFile(path, iniData);
	if(size == 0)
		size = iniReadFile(path, iniData);
	if(size != 0)
	{
		if(((iniData[0]&0xFF) == 0xFF)||((iniData[1]&0xFF) == 0xFF)) // multibyte file
		{
			iniEnd();
			return FALSE;
		}
		iniPreParse();
		return TRUE;
	}
	return FALSE;
}

// finalizes and frees ini resources
void iniEnd(void)
{
	if(iniData != NULL)
	{
		ExFreePool(iniData);
		iniData = NULL;
	}
}

// gets string pointer, returns NULL on fail
const char* iniGetString(const char* name)
{
	DWORD off = 0;
	if(iniFindLabelOffset(name, &off))
	{
		if(iniData[off] == 0x0)
			return NULL;
		else
			return &iniData[off];
	}
	return NULL;
}

// gets bool value, returns defValue on fail
BOOL iniGetBool(const char* name, BOOL defValue)
{
	DWORD off = 0;
	if(iniFindLabelOffset(name, &off))
	{
		if(strnicmp(&iniData[off], "true", strlen("true")) == 0)
			return TRUE;
		else if(strnicmp(&iniData[off], "false", strlen("false")) == 0)
			return FALSE;
	}
	return defValue;
}

// gets u32 hex value, returns defValue on fail
DWORD iniGetDword(const char* name, DWORD defValue)
{
	DWORD off = 0;
	if(iniFindLabelOffset(name, &off))
	{
		if(((iniData[off]&0xFF) == '0')&&(myatol(iniData[off+1]&0xFF) == 'x'))
		{
			off+=2;
			return myatox32(&iniData[off], defValue);
		}
		else
		{
			DWORD ret = atoi(&iniData[off]);
			if(ret == 0)
				return defValue;
			return ret;
		}
	}
	return defValue;
}

// converts a ascii hex char to a nibble
unsigned char myatox(char c)
{
	if((c>0x60)&&(c<0x67)) // a thru f
		return (unsigned char)(c-0x57);
	else if((c>0x29)&&(c<0x40)) // 0 thru 9
		return (unsigned char)(c-0x30);
	else if((c>0x40)&&(c<0x47)) // A thru F
		return (unsigned char)(c-0x37);
	return 0xFF;
}

// converts uppercase to lower case
char myatol(char c)
{
	char ret = c;
	if((c>0x40)&&(c<0x5A)) // A thru Z
		ret = c+0x20;
	return ret;
}

// converts (up to) 16 uchar hex nibbles into a u32
DWORD myatox32(char* dat, DWORD defValue)
{
	DWORD retval = 0, i; //, tmp;
	unsigned char res;
	for(i = 0; i < 16; i++)
	{
		if((dat[i]&0xFF) == 0x0)
			i = 16;
		else
		{
			res = (myatox(dat[i]&0xFF));
			if((res&0xFF) == 0xFF) // abort in case of bad value
			{
				i = 16;
				retval = defValue;
			}
			else
				retval = (retval<<4)+(res&0xF);
		}
	}
	return retval;
}

// finds an instance of a label and returns offset of key txt in offset pointer
BOOL iniFindLabelOffset(const char* name, PDWORD offset)
{
	DWORD i = 0;
	for(;i < size; i++)
	{
		if(myatol(name[0]&0xFF) == myatol(iniData[i]&0xFF))
		{
			if(_strnicmp(name, &iniData[i], strlen(name)) == 0)
			{
				// get over ' = ' and similar
				i += strlen(name);
				i += (iniGetSpaces(i));
				if((iniData[i]&0xFF) == '=')
				{
					i++;
					i += (iniGetSpaces(i));
					offset[0] = i;
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

// reads file to a buffer
DWORD iniReadFile(const char* path, char* buf)
{
	HANDLE file;
	DWORD ttlread = 0;
	file = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file != INVALID_HANDLE_VALUE)
	{
		DWORD High, Low = GetFileSize(file, &High);
		if (Low != 0xFFFFFFFF)
		{
			DWORD read = 0;
			iniData = (char*)ExAllocatePoolTypeWithTag(((Low+1+0xFFFF)&~0xFFFF), 'DLNI', PoolTypeSystem); // use pool on 360
			memset(iniData, 0x0, Low+1);
			iniData[Low] = 0xa; // just in case the file doesn't end with a carriage return
			if(iniData != NULL)
			{
				while(Low > 0)
				{
					ReadFile(file, iniData, Low, &read, NULL);
					ttlread+=read;
					Low-=read;
				}
			}
			ttlread+=1; // add one byte for the 'just in case' appended 0xa
		}
		CloseHandle(file);
	}
	return ttlread;
}

// check to see if it is only spaces from current offset to next 0x0
BOOL iniCheckSpaces(DWORD offset)
{
	DWORD i=offset+1;
	BOOL ret = FALSE;
	for(; i < size; i++)
	{
		if((iniData[i]&0xFF) != ' ')
		{
			if((iniData[i]&0xFF) == 0x0)
			{
				ret = TRUE;
			}
			i = size;
		}
	}
	return ret;
}

// counts the number of spaces
DWORD iniGetSpaces(DWORD offset)
{
	DWORD i = 0, ret = 0;
	for(; i < (size-offset); i++)
	{
		if((iniData[i+offset]&0xFF) != ' ')
		{
			ret = i;
			i = size;
		}
	}
	return ret;
}

// prepares buffer data for retrieve commands
void iniPreParse(void)
{
	DWORD i, j;
	// remove comments
	for(i = 0; i < size; i++)
	{
		if((iniData[i]&0xFF) == ';')
		{
			while(((iniData[i]&0xFF)!= 0xa)&&(i < size))
			{
				iniData[i] = 0x0;
				i++;
			}
		}
	}

	// remove invalid chars
	for(i = 0; i < size; i++)
	{
		j=0;
		for(; j < sizeof(validChars); j++)
		{
			if((iniData[i]&0xFF) == (validChars[j]&0xFF))
			{
				j = sizeof(validChars)+1;
			}
		}
		if(j == sizeof(validChars))
			iniData[i] = 0x0;
	}
	// remove end of line white spaces
	for(i = 0; i < size; i++)
	{
		if((iniData[i]&0xFF) == ' ')
		{
			// if it's spaces to the next 0x0, 0x0 them all
			if(iniCheckSpaces(i))
			{
				while(((iniData[i]&0xFF) != 0x0)&&(i < size))
				{
					iniData[i] = 0x0;
					i++;
				}
			}
		}
	}
}
