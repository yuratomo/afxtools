// afxview.cpp : アプリケーション用クラスの定義を行います。
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <share.h>
#include "afxcom.h"

IDispatch* pAfxApp = NULL;

DWORD run(char* cmdline, char* redirectTo)
{
    STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcessInfo;
    char Args[4096];

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(STARTUPINFO);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_HIDE;

    Args[0] = 0;
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
	if (WaitForSingleObject(ProcessInfo.hProcess, INFINITE) == WAIT_OBJECT_0) {
		AfxTrace(pAfxApp, "MesOk");
		return 1;
	}

    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	int retval = 0;
	DWORD ret;

	if (__argc < 2) {
		return -1;
	}

	AfxInit(pAfxApp);

	char dir[MAX_PATH];
	char path[MAX_PATH];
	GetTempPath(sizeof(dir), dir);
	GetTempFileName(dir, "afx", 0, path);

	ret = run(lpCmdLine, path);
	if (ret == 0) {
		AfxTrace(pAfxApp, path);
	}

	// &VIEW
	char viewcmd[MAX_PATH];
	sprintf(viewcmd, "&VIEW %s", path);
	AfxExec(pAfxApp, viewcmd);

	DeleteFile(path);
	AfxCleanup(pAfxApp);

	return retval;
}

