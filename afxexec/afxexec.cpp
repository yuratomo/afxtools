// AfxFind.cpp : アプリケーション用クラスの定義を行います。
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <share.h>
#include <locale.h>
#include "afxexec.h"
#include "afxcom.h"

#define MAX_LINE_SIZE     1024

char _prefix[64] = ">";
char _ini_path[MAX_PATH];
IDispatch* pAfxApp = NULL;
PROCESS_INFORMATION ProcessInfo;

static void Message2(const char* line)
{
	int len = strlen(line)+1;
	wchar_t* wbuf = new wchar_t[len];
	memset(wbuf, 0, 2*len);
	mbstowcs(wbuf, line, 2*len);
	wchar_t* cr = wcsstr(wbuf, L"\r");
	if (cr != NULL) *cr = L'\0';

	VARIANT x;
	x.vt = VT_BSTR;
	x.bstrVal = ::SysAllocString(wbuf);
	AfxExec(DISPATCH_METHOD, NULL, pAfxApp, L"MesPrint", 1, x);
	delete [] wbuf;

	return;
}

static void Message(const char* fmt, ...)
{
	char line[MAX_LINE_SIZE];
	va_list arg;
	va_start(arg, fmt);
	vsprintf(line, fmt, arg);
	va_end(arg);
	Message2(line);
}

//DWORD RunSilent(char* strFunct, char* strstrParams)
DWORD RunSilent(char* cmdline, char* redirectTo)
{
    STARTUPINFO StartupInfo;
    char Args[4096];
    //char *pEnvCMD = NULL;
    //char *pDefaultCMD = "CMD.EXE";

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(STARTUPINFO);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_HIDE;

    Args[0] = 0;

/*
    pEnvCMD = getenv("COMSPEC");

    if(pEnvCMD){

        strcpy(Args, pEnvCMD);
    }
    else{
        strcpy(Args, pDefaultCMD);
    }

    // "/c" option - Do the command then terminate the command window
    strcat(Args, " /c "); 
	*/
    //the application you would like to run from the command window
    strcat(Args, cmdline);  
    strcat(Args, " > "); 
    strcat(Args, redirectTo);
    strcat(Args, " 2>&1"); 

    if (!CreateProcess( NULL, Args, NULL, NULL, FALSE,
        CREATE_NEW_CONSOLE, 
        NULL, 
        NULL,
        &StartupInfo,
        &ProcessInfo))
    {
        return GetLastError();          
    }

    return 0;
}


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
/*
int main(int __argc, char** __argv)
*/
{
	int retval = 0;
	DWORD ret;
	HANDLE handles[2];

	setlocale( LC_ALL, "Japanese" );
	memset(handles, 0, sizeof(handles));

	if (__argc < 2) {
		Message2("[usage]");
		Message2("  afxexec.exe command");
		return -1;
	}

	// iniファイル解析
	GetModuleFileName(hInstance, _ini_path, sizeof(_ini_path));
	int len = strlen(_ini_path);
	_ini_path[len-1] = 'i';
	_ini_path[len-2] = 'n';
	_ini_path[len-3] = 'i';
	::GetPrivateProfileString("Config", "PromptString", ">", _prefix, sizeof(_prefix), _ini_path);

	AfxInit(pAfxApp);

	char dir[MAX_PATH];
	char path[MAX_PATH];
	GetTempPath(sizeof(dir), dir);
	GetTempFileName(dir, "afx", 0, path);
	handles[0] = FindFirstChangeNotification(
			dir, TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE);

	ret = RunSilent(lpCmdLine, path);
	if (ret > 0) {
		retval = -2;
		goto END;
	}
	Message2(lpCmdLine);
	
	handles[1] = ProcessInfo.hProcess;

	FILE* fp = NULL;
	int cur = 0;
	int last = 0;
	char line[MAX_LINE_SIZE];
	while (1) {
		fp = _fsopen(path, "r", _SH_DENYNO);
		if (fp == NULL) {
			Message("[afxexec] Can not open file (%s)", path);
			break;
		}
		fseek(fp, 0, SEEK_END);
        last = ftell(fp);
		if (last > cur) {
			fseek(fp, cur, SEEK_SET);
		} else if (cur > last) {
			fseek(fp, 0, SEEK_SET);
		}
		if (last != cur) {
			int adjust = 0;
			while (fgets(line, sizeof(line)-1, fp) != NULL) {
				char* ln = strchr(line, '\r');
				if (ln == NULL) {
					ln = strchr(line, '\n');
				}
				if (ln == NULL) {
					if (strlen(line) < MAX_LINE_SIZE-1) {
						adjust = strlen(line);
						break;
					}
				} else {
					*ln = '\0';
				}
				Message("%s%s", _prefix, line);
			}
			cur = last - adjust;
		}
		fclose(fp);

		while (WaitForMultipleObjects(2, handles, FALSE, 1000) != WAIT_OBJECT_0) {
			if (WaitForSingleObject(handles[1], 0) == WAIT_OBJECT_0) {
				AfxExec(DISPATCH_METHOD, NULL, pAfxApp, L"MesOk", 0);
				retval = -3;
				goto END;
			}
		}
		if (!FindNextChangeNotification(handles[0])) {
			break;
		}
	}

END:
	for (int idx=0; idx<2; idx++) {
		if (handles[idx] != NULL) {
			::CloseHandle(handles[idx]);
			handles[idx] = NULL;
		}
	}

	DeleteFile(path);
	AfxCleanup(pAfxApp);

	return retval;
}

