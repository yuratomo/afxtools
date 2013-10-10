@echo off
REM afxmake.exeのフルパス設定
SET AFXMAKE=%~d0%~p0afxmake.exe

REM Visual Studio 2005 vcproje用設定
IF EXIST "C:\Program Files (x86)\Microsoft Visual Studio 8.0\VC\bin\vcvars32.bat" (SET VC8VAR=C:\Program Files (x86)\Microsoft Visual Studio 8.0\VC\bin\vcvars32.bat) ELSE (SET VC8VAR=C:\Program Files\Microsoft Visual Studio 8.0\VC\bin\vcvars32.bat)

REM Visual Studio 2008 vcproje用設定
IF EXIST "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin\vcvars32.bat" (SET VC9VAR=C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin\vcvars32.bat) ELSE (SET VC9VAR=C:\Program Files\Microsoft Visual Studio 9.0\VC\bin\vcvars32.bat)

REM apache antのパス指定 (通常の開発環境ならパスが通っている)
SET ANT=ant

REM javaコンパイラのパス指定 (通常の開発環境ならパスが通っている)
SET JAVAC=javac

REM WDK(Windows Driver Kit)のインストールディレクトリ設定
SET WDK7DIR=C:\WinDDK\7600.16385.1

