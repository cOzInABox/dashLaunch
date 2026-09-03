#pragma once
#include <xtl.h>
#include <xui.h>
#include <xuiapp.h>
#include "xkelib.h"

class DataEntry
{
public:	
	static DataEntry& GetInstance(){static DataEntry singleton; return singleton;}
	VOID Init(CXuiScene scn);
	VOID Reset(VOID);
	VOID SetRange(DWORD min, DWORD max) {m_rangeMin = min; m_rangeMax = max;}
	VOID SetCurrVal(DWORD val, BOOL useHex);
	VOID ProcessButton(HXUIOBJ obj);
	DWORD GetValue(VOID) {return GetNumFromText(); Reset();}
	VOID ProcessBackspace(VOID);

private:
	VOID ProcessNumber(WCHAR val, BOOL makeHex = FALSE);
	VOID MakeHex(VOID);
	DWORD GetNumFromText(VOID);

	WCHAR textValue[64];
	DWORD currChar;
	DWORD maxChars;
	BOOL isHex;
	DWORD m_rangeMin;
	DWORD m_rangeMax;
	CXuiScene m_DataScene;
	CXuiTextElement m_TextBox;
	CXuiTextElement m_InfoBox;
	CXuiControl m_letterA;
	CXuiControl m_letterB;
	CXuiControl m_letterC;
	CXuiControl m_letterD;
	CXuiControl m_letterE;
	CXuiControl m_letterF;
	CXuiControl m_number0;
	CXuiControl m_number1;
	CXuiControl m_number2;
	CXuiControl m_number3;
	CXuiControl m_number4;
	CXuiControl m_number5;
	CXuiControl m_number6;
	CXuiControl m_number7;
	CXuiControl m_number8;
	CXuiControl m_number9;
	CXuiControl m_Back;

	DataEntry();
	~DataEntry() {}
	DataEntry(const DataEntry&);                 // Prevent copy-construction
	DataEntry& operator=(const DataEntry&);      // Prevent assignment
};
