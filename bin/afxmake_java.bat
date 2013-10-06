REM @echo off
call "%~d0%~p0afxmake_common.bat"
%~d2
cd %2

SET CLASS_ROOT=%2
SET TARGET=%3

IF %CLASS_ROOT%=="" goto ERROR
IF %TARGET%=="" goto ERROR

SET /A LEN=0
SET LAST=%CLASS_ROOT%
:LOOP_CLASS_ROOT_LEN
SET /A LEN+=1
SET LAST=%LAST:~1%
IF NOT "%LAST%=="" GOTO LOOP_CLASS_ROOT_LEN
SET /A LEN+=1
CALL SET SRC=%%TARGET:~%LEN%^%%
SET SRC="%SRC%

%JAVAC% %SRC% %4 %5 %6 %7 %8 %9 2> .javamake
type .javamake
"%~d0%~p0afxmake_java.vbs" %2\
"%AFXMAKE%" java .javamake
del .javamake

goto FINISH

:ERROR
echo ƒGƒ‰[‚ª”­¶‚µ‚Ü‚µ‚½B
echo   ‘æ1ˆø”:%1
echo   ‘æ2ˆø”:%2
echo   ‘æ3ˆø”:%3

:FINISH
cd "%1"
