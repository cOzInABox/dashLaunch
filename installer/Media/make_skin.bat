@echo off
setlocal

set PATH=%XEDK%\bin\win32;%PATH%;
REM ResX2Bin /nologo optionInfo.resx
REM ResX2Bin /nologo strings.resx
REM ResX2Bin /nologo ja-jp\optionInfo.resx
REM ResX2Bin /nologo ja-jp\strings.resx
REM ResX2Bin /nologo de-de\optionInfo.resx
REM ResX2Bin /nologo de-de\strings.resx
REM ResX2Bin /nologo fr-fr\optionInfo.resx
REM ResX2Bin /nologo fr-fr\strings.resx
REM ResX2Bin /nologo es-es\optionInfo.resx
REM ResX2Bin /nologo es-es\strings.resx
REM ResX2Bin /nologo it-it\optionInfo.resx
REM ResX2Bin /nologo it-it\strings.resx
REM ResX2Bin /nologo ko-kr\optionInfo.resx
REM ResX2Bin /nologo ko-kr\strings.resx
REM ResX2Bin /nologo zh-cht\optionInfo.resx
REM ResX2Bin /nologo zh-cht\strings.resx
REM ResX2Bin /nologo pt-br\optionInfo.resx
REM ResX2Bin /nologo pt-br\strings.resx
REM ResX2Bin /nologo zh-chs\optionInfo.resx
REM ResX2Bin /nologo zh-chs\strings.resx
REM ResX2Bin /nologo pl-pl\optionInfo.resx
REM ResX2Bin /nologo pl-pl\strings.resx
REM ResX2Bin /nologo ru-ru\optionInfo.resx
REM ResX2Bin /nologo ru-ru\strings.resx

XuiPkg /nologo /o skin.xzp /c xzp.txt



endlocal
pause