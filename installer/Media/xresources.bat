@echo off
setlocal
set srcpath=%1
set xzppath=%2
set xzpsrc=%3
set xzpblsrc=%4

XuiPkg /nologo /o %xzppath% /c %xzpsrc%

if defined xzpblsrc call xskinsrcr.bat


endlocal
