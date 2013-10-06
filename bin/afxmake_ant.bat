@echo off
call "%~d0%~p0afxmake_common.bat"
%~d1
cd "%1"
call %ANT% -logfile .antmake %2

SET FIRST="OFF"
for /f "usebackq delims= eol=" %%s in (.antmake) do @(
    echo %%s > .javamake
	goto break
)
:break

type .antmake | findstr javac >> .javamake
"%AFXMAKE%" ant .javamake

:ANTEND
del .antmake
del .javamake
