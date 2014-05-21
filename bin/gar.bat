@echo off
for /f "delims=" %%a in ('afxams.exe Extract $R') do cd "%%a\\"
