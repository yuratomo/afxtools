@echo off
call "%~d0%~p0afxmake_common.bat"
call %WDK7DIR%\bin\setenv.bat %WDK7DIR%\ %2 %3 %4
%~d1
cd %1
build %5
echo build %2 %3 %4 > .wdkmake
set CPU=%3
if CPU=="x86" goto END_CPU
	set CPU="amd64"
:END_CPU
type build%2_%4_%CPU%.err >> .wdkmake
echo build end >> .wdkmake
"%AFXMAKE%" vc .wdkmake
del .wdkmake
