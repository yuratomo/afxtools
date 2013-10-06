@echo off
call "%~d0%~p0afxmake_common.bat"
call "%VC8VAR%"
%~d1
cd "%1"
for %%i in (*.vcproj) do (
  vcbuild %%i %2 %3 %4 %5 %6 %7 %8 %9 /nologo /nohtmllog /logfile:".vcmake"
)
"%AFXMAKE%" vc .vcmake
del .vcmake
