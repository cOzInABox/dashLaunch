#include "DataEntry.h"
#include <stdio.h>
#include "logging.h"


DataEntry::DataEntry(VOID)
{
}

VOID DataEntry::Init(CXuiScene scn)
{
	m_DataScene = scn;
	scn.GetChildById(L"ButtonA",&m_letterA);
	scn.GetChildById(L"ButtonB",&m_letterB);
	scn.GetChildById(L"ButtonC",&m_letterC);
	scn.GetChildById(L"ButtonD",&m_letterD);
	scn.GetChildById(L"ButtonE",&m_letterE);
	scn.GetChildById(L"ButtonF",&m_letterF);
	scn.GetChildById(L"Button0",&m_number0);
	scn.GetChildById(L"Button1",&m_number1);
	scn.GetChildById(L"Button2",&m_number2);
	scn.GetChildById(L"Button3",&m_number3);
	scn.GetChildById(L"Button4",&m_number4);
	scn.GetChildById(L"Button5",&m_number5);
	scn.GetChildById(L"Button6",&m_number6);
	scn.GetChildById(L"Button7",&m_number7);
	scn.GetChildById(L"Button8",&m_number8);
	scn.GetChildById(L"Button9",&m_number9);
	scn.GetChildById(L"Backspace",&m_Back);
	scn.GetChildById(L"InputFeedback", &m_TextBox);
	scn.GetChildById(L"BoxReason", &m_InfoBox);
}

VOID DataEntry::Reset(VOID)
{
	m_number5.SetFocus();
	memset(textValue, 0, sizeof(textValue));
	currChar = 0;
	isHex = FALSE;
	m_rangeMin = 0;
	m_rangeMax = 0xFFFFFFFF;
	maxChars = 10;
	m_InfoBox.SetText(textValue);
}

VOID DataEntry::SetCurrVal(DWORD val, BOOL useHex)
{
	isHex = useHex;
	if(isHex)
		wsprintfW(textValue, L"0x%x", val);
	else
		wsprintfW(textValue, L"%d", val);
	m_TextBox.SetText(textValue);
	currChar = wcslen(textValue);
}

VOID DataEntry::MakeHex(VOID)
{
	WCHAR temp[128];
	wcscpy(temp, textValue);
	wsprintfW(textValue, L"0x%s", temp);
	currChar+=2;
	isHex = TRUE;
}

VOID DataEntry::ProcessBackspace(VOID)
{
	if((isHex)&&(currChar == 2))
	{
		memset(textValue, 0, sizeof(textValue));
		currChar = 0;
		isHex = FALSE;
		m_TextBox.SetText(textValue);
	}
	else
	{
		if(currChar > 0)
		{
			currChar --;
			textValue[currChar] = 0;
			m_TextBox.SetText(textValue);
		}
	}
}

VOID DataEntry::ProcessNumber(WCHAR val, BOOL makeHex)
{
	if((makeHex)&&(!isHex))
		MakeHex();
	if((isHex)&&(currChar == 3)) // handle when displaying 0x0
	{
		if(textValue[2] == '0')
			currChar = 2;
	}
	else if(currChar == 1) // handle when displaying 0
	{
		if(textValue[0] == '0')
			currChar = 0;
	}
	textValue[currChar] = val;
	currChar++;
	m_TextBox.SetText(textValue);
}

DWORD DataEntry::GetNumFromText(VOID)
{
	DWORD ret = 0;
	if(isHex)
		ret = wcstoul(textValue, NULL, 16);
	else
		ret = wcstoul(textValue, NULL, 10);
	return ret;
}

VOID DataEntry::ProcessButton(HXUIOBJ obj)
{
	if(obj == m_Back)
		ProcessBackspace();
	else if(currChar != maxChars)
	{
		if(obj == m_number0)
		{
			if(currChar != 0)
				ProcessNumber('0');
		}
		else if(obj == m_number1)
			ProcessNumber('1');
		else if(obj == m_number2)
			ProcessNumber('2');
		else if(obj == m_number3)
			ProcessNumber('3');
		else if(obj == m_number4)
			ProcessNumber('4');
		else if(obj == m_number5)
			ProcessNumber('5');
		else if(obj == m_number6)
			ProcessNumber('6');
		else if(obj == m_number7)
			ProcessNumber('7');
		else if(obj == m_number8)
			ProcessNumber('8');
		else if(obj == m_number9)
			ProcessNumber('9');
		else if(obj == m_letterA)
			ProcessNumber('A', TRUE);
		else if(obj == m_letterB)
			ProcessNumber('B', TRUE);
		else if(obj == m_letterC)
			ProcessNumber('C', TRUE);
		else if(obj == m_letterD)
			ProcessNumber('D', TRUE);
		else if(obj == m_letterE)
			ProcessNumber('E', TRUE);
		else if(obj == m_letterF)
			ProcessNumber('F', TRUE);
	}
	//WCHAR wccurVal[0x120];
	//wsprintfW(wccurVal, L"%d (0x%x)", GetNumFromText(), GetNumFromText());
	//m_InfoBox.SetText(wccurVal);
	//lDbgPrint("current val: %d (0x%x)\n", GetNumFromText(), GetNumFromText());
}
