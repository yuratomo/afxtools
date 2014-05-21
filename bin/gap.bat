@echo off
for /f "delims=" %%a in ('afxams.exe Extract $P') do cd "%%a\\"
