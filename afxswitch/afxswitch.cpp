#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HWND _hWndTarget = NULL;
HWND _hWndAfxCkw = NULL;

BOOL CALLBACK EnumProc4FindTarget(HWND hWnd , LPARAM lp)
{
	DWORD dwPID = NULL; 
	DWORD dwThreadID = ::GetWindowThreadProcessId(hWnd, &dwPID);
	if (dwPID == (DWORD)lp && GetWindow(hWnd, GW_OWNER) == 0) {
		_hWndTarget = hWnd;
		return FALSE;
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
	HWND hWndAfx = ::FindWindow("TAfxWForm", NULL);
	if (hWndAfx == INVALID_HANDLE_VALUE) {
		hWndAfx = ::FindWindow("TAfxForm", NULL);
		if (hWndAfx == INVALID_HANDLE_VALUE) {
			return -1;
		}
	}
	RECT rect;
	::GetWindowRect(hWndAfx, &rect);

	PROCESS_INFORMATION pi;
	STARTUPINFO si;

	ZeroMemory(&si,sizeof(si));
	si.cb=sizeof(si);
	//si.dwFlags = STARTF_USESHOWWINDOW;
	//si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESIZE | STARTF_USEPOSITION;
	si.dwX = rect.left;
	si.dwY = rect.top;
	si.dwXSize = rect.right - rect.left;
	si.dwYSize = rect.bottom - rect.top;

	if (!CreateProcess(
			NULL,
			lpCmdLine,
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

	// 入力可能になるまで待つ
	::WaitForInputIdle(pi.hProcess, INFINITE);

	// ターゲットウィンドウを検索してウィンドウサイズ位置・サイズを変更する。
	int cnt = 0;
	while (1) {
		::EnumWindows(EnumProc4FindTarget, (LPARAM)pi.dwProcessId);
		if (_hWndTarget != NULL) {
			break;
		}
		Sleep(50);
		if (cnt++ >= 100) {
			break;
		}
	}

	::SetWindowPos(
			_hWndTarget,
			HWND_TOP,
			rect.left, rect.top, 
			//rect.right - rect.left - GetSystemMetrics(SM_CXBORDER)*2 - 4,
			//rect.bottom - rect.top - GetSystemMetrics (SM_CYCAPTION) - GetSystemMetrics(SM_CYBORDER)*2 - 4,
			rect.right - rect.left,
			rect.bottom - rect.top,
			NULL);
	::ShowWindow(_hWndTarget, SW_SHOW);

	// あふwを隠して、ターゲットが終了するのを待つ。その後あふwを再表示する。
	::ShowWindow(hWndAfx, SW_HIDE);

	WaitForSingleObject(pi.hProcess, INFINITE);

	::ShowWindow(hWndAfx, SW_SHOW);

	// 2011.05.24 終了時はあふwにフォーカスを戻す
	SetFocusOtherProcess(hWndAfx);

	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );

	return 0;
}
