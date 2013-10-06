#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HWND _hVimWnd = 0;
HANDLE _vimPid = 0;
DWORD _dwVimTID = 0;
HWND _hWndAfxCkw = NULL;

BOOL CALLBACK EnumChildProc(HWND hWnd , LPARAM lp)
{
	char className[256];
	if (::GetClassName(hWnd, className, sizeof(className))) {
		if (strcmp(className, "xyzzy") == 0) {
			DWORD pid;
			DWORD tid = ::GetWindowThreadProcessId(hWnd, &pid);
			_hVimWnd = hWnd;
			_dwVimTID = tid;
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CALLBACK EnumChildProc4Ckw(HWND hWnd , LPARAM lp)
{
	char className[256];
	if (::GetClassName(hWnd, className, sizeof(className))) {
		if (strcmp(className, "CkwAfxWindowClass") == 0) {
			_hWndAfxCkw = hWnd;
			return FALSE;
		}
	}

	return TRUE;
}

void SetFocusOtherProcess(HWND hWnd)
{
	DWORD buf;
	DWORD pid;
	DWORD tid = ::GetWindowThreadProcessId(hWnd, &pid);
	DWORD tidto = ::GetWindowThreadProcessId(::GetForegroundWindow(), &pid);
	::AttachThreadInput(tid, tidto, TRUE);
	::SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &buf, 0);
    ::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, NULL, 0);
	::BringWindowToTop(hWnd);
	::SetForegroundWindow(hWnd);
    ::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &buf, 0);
	::AttachThreadInput(tidto, tid, FALSE);
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	HWND _hAfxWnd = ::FindWindow("TAfxWForm", NULL);
	if (_hAfxWnd == INVALID_HANDLE_VALUE) {
		_hAfxWnd = ::FindWindow("TAfxForm", NULL);
		if (_hAfxWnd == INVALID_HANDLE_VALUE) {
			return -1;
		}
	}

	PROCESS_INFORMATION pi;
	STARTUPINFO si;

	ZeroMemory(&si,sizeof(si));
	si.cb=sizeof(si);

	char* pAppPath = lpCmdLine;
	char* pParams = "";
	char szCmdLineTemp[1024];
	memset(szCmdLineTemp, 0, sizeof(szCmdLineTemp));
	strcpy(szCmdLineTemp, lpCmdLine);
	char* exe = strstr(szCmdLineTemp, ".exe");
	if (exe != NULL) {
		char* sep = strchr(exe, ' ');
		if (sep == NULL) {
			sep = strchr(exe, '\t');
		}
		if (sep != NULL) {
			*sep = '\0';
			pAppPath = szCmdLineTemp;
			pParams = sep+1;
		}
	}

	char cmd[256];
	sprintf(cmd, 
			"%s -windowid 0x%08x %s",
			pAppPath,
			_hAfxWnd,
			pParams);

	if (!CreateProcess(
			NULL,
			cmd,
			NULL,
			NULL,
			FALSE,
			NORMAL_PRIORITY_CLASS,
			NULL,
			NULL,
			&si,
			&pi)) {
		return -1;
	}

	_hVimWnd = (HWND)INVALID_HANDLE_VALUE;
	_vimPid = pi.hProcess;

	// 2011.05.24 0.2.0 入力可能になるまで待つ
	::WaitForInputIdle(pi.hProcess, INFINITE);

	// 2011.05.02 0.1.1 afxckwが起動していたら非表示にする
	::EnumChildWindows(_hAfxWnd, EnumChildProc4Ckw, NULL);
	if (_hWndAfxCkw != NULL) {
		ShowWindow(_hWndAfxCkw, SW_HIDE);
	}

	RECT rect;
	RECT rectAfx;
	memset(&rectAfx, 0, sizeof(rectAfx));

	DWORD dwExCode;
	while (1) {
		if (_hVimWnd == INVALID_HANDLE_VALUE || _hVimWnd == 0) {
			EnumChildWindows(_hAfxWnd, EnumChildProc , 0);
		}
		dwExCode = WaitForSingleObject(pi.hProcess, 300);
		if (dwExCode == WAIT_TIMEOUT) {
			HWND active = ::GetForegroundWindow();
			if (active == _hAfxWnd) {
				if (_hVimWnd != INVALID_HANDLE_VALUE) {
					SetFocusOtherProcess(_hVimWnd);
				}
			}

			/* 2011.09.20 コメントアウト 
			// 2011.05.24 あふのウィンドウサイズを監視する。
			::GetWindowRect(_hAfxWnd, &rect);
			if (( rect.right - rect.left) != ( rectAfx.right - rectAfx.left ) || 
				( rect.bottom - rect.top) != ( rectAfx.bottom - rectAfx.top ) ) {
				::SetWindowPos(
						_hVimWnd,
						HWND_TOP,
						0, 0, 
						rect.right - rect.left - GetSystemMetrics(SM_CXBORDER)*2 - 4,
						rect.bottom - rect.top - GetSystemMetrics (SM_CYCAPTION) - GetSystemMetrics(SM_CYBORDER)*2 - 4,
						SWP_NOMOVE);
				rectAfx = rect;
			}
			*/
			::GetClientRect(_hAfxWnd, &rect);
			if (( rect.right - rect.left) != ( rectAfx.right - rectAfx.left ) || 
				( rect.bottom - rect.top) != ( rectAfx.bottom - rectAfx.top ) ) {
				::SetWindowPos( _hVimWnd, HWND_TOP, rect.left, rect.top, rect.right, rect.bottom, SWP_NOMOVE);
				rectAfx = rect;
			}

			continue;
		} else  {
			break;
		}

	}

	// 2011.05.02 0.1.1 隠していたafxckwを表示にする。
	if (_hWndAfxCkw != INVALID_HANDLE_VALUE) {
		ShowWindow(_hWndAfxCkw, SW_SHOW);
	}

	// 2011.05.24 終了時はあふwにフォーカスを戻す
	SetFocusOtherProcess(_hAfxWnd);

	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );

	return 0;
}

