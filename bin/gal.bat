@echo off
for /f "delims=" %%a in ('afxams.exe Extract $L') do cd "%%a\\"
