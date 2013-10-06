#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __AFXVIM_SHAREDMEM__ "__AFXVIM_SHAREDMEM__"
#define __AFXVIM_EVENT__ "__AFXVIM_EVENT__"

HWND _hVimWnd = 0;
HANDLE _vimPid = 0;
HWND _hWndAfxCkw = NULL;

BOOL CALLBACK EnumChildProc(HWND hWnd , LPARAM lp)
{
	char className[256];
	if (::GetClassName(hWnd, className, sizeof(className))) {
		if (strcmp(className, "Vim") == 0) {
			_hVimWnd = hWnd;
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

	BOOL bFirstExec = FALSE;
	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, __AFXVIM_EVENT__);
	if (hEvent == NULL) {
		bFirstExec = TRUE;
		hEvent = CreateEvent(NULL, FALSE, FALSE, __AFXVIM_EVENT__);
	}

	// 2011.10.29 共有メモリ上に設定を持つ
	int *smode = NULL;
	HANDLE hShare = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
			0, 32, __AFXVIM_SHAREDMEM__);
	if (hShare != NULL) {
		smode = (int*)::MapViewOfFile(hShare, FILE_MAP_WRITE, 0, 0, 32);
		if (bFirstExec) {
			memset(smode, 0, 32);
		}
	}

	PROCESS_INFORMATION pi;
	STARTUPINFO si;

	ZeroMemory(&si,sizeof(si));
	si.cb=sizeof(si);

	char cmd[256];
	if (bFirstExec) {
		sprintf(cmd, 
				"%s --windowid 0x%08x --servername afxvim --cmd \":silent set go+=C\"",
				lpCmdLine,
				_hAfxWnd);
	} else {
		char *sep = strstr(lpCmdLine, "gvim.exe");
		if (sep != NULL && strlen(sep+8) > 0 ) {
			sep += 8;
			sprintf(cmd, 
					"%s --servername afxvim --remote-tab-silent %s",
					lpCmdLine, sep);
		} else {
			if ((smode != NULL) && (smode[0] != NULL)) {
				ShowWindow((HWND)smode[0], SW_RESTORE);
				::UnmapViewOfFile(smode);
				::CloseHandle(hShare);
				::CloseHandle(hEvent);
				return 0;
			}
			sprintf(cmd,
					"%s --servername afxvim --remote-tab-silent __dummy__", lpCmdLine);
		}
	}

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
		::UnmapViewOfFile(smode);
		::CloseHandle(hShare);
		::CloseHandle(hEvent);
		::MessageBox(NULL, "err", "execute command error.", MB_OK);
		return -1;
	}

	_hVimWnd = (HWND)INVALID_HANDLE_VALUE;
	_vimPid = pi.hProcess;

	if (!bFirstExec) {
		SetEvent(hEvent);
	} else {
		// 2011.05.24 0.2.0 入力可能になるまで待つ
		::WaitForInputIdle(pi.hProcess, INFINITE);

		// 2011.05.02 0.1.1 afxckwが起動していたら非表示にする
		::EnumChildWindows(_hAfxWnd, EnumChildProc4Ckw, NULL);
		if (_hWndAfxCkw != NULL) {
			ShowWindow(_hWndAfxCkw, SW_HIDE);
		}

		RECT rect;
		RECT vimRect;
		RECT rectAfx;
		POINT vimLeftTopPoint;
		memset(&rectAfx, 0, sizeof(rectAfx));

		BOOL bFirstIconic = TRUE;
		DWORD dwExCode;
		while (1) {
			if (_hVimWnd == INVALID_HANDLE_VALUE || _hVimWnd == 0) {
				EnumChildWindows(_hAfxWnd, EnumChildProc , 0);
				if (_hVimWnd != NULL) {
					smode[0] = (int)_hVimWnd;
				}
			}
			dwExCode = WaitForSingleObject(pi.hProcess, 300);
			if (dwExCode == WAIT_TIMEOUT) {
				if (IsIconic(_hVimWnd)) {
					if (bFirstIconic == TRUE) {
						SetFocusOtherProcess(_hAfxWnd);
						bFirstIconic = FALSE;
					}
					if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0) {
						rectAfx.left = -100;
						ResetEvent(hEvent);
					}
				} else {
					bFirstIconic = TRUE;
					HWND active = ::GetForegroundWindow();
					if (active == _hAfxWnd) {
						if (_hVimWnd != INVALID_HANDLE_VALUE) {
							SetFocusOtherProcess(_hVimWnd);
						}
					}

					// 2011.05.12 vimにフォーカスが移っているならafxを最前面に移動
					/*
					if (active == _hVimWnd) {
						::SetFocusOtherProcess(_hAfxWnd, _hVimWnd);
					}
					*/

					// 2011.10.23 vimの位置監視
					::GetWindowRect(_hVimWnd, &vimRect);
					vimLeftTopPoint.x = vimRect.left;
					vimLeftTopPoint.y = vimRect.top;
					::ScreenToClient(_hAfxWnd, &vimLeftTopPoint);
					if (vimLeftTopPoint.x != 0 || vimLeftTopPoint.y != 0) {
						::SetWindowPos( _hVimWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE);
					}

					::GetClientRect(_hAfxWnd, &rect);
					if (( rect.right - rect.left) != ( rectAfx.right - rectAfx.left ) || 
						( rect.bottom - rect.top) != ( rectAfx.bottom - rectAfx.top ) ) {
						::SetWindowPos( _hVimWnd, HWND_TOP, 0, 0, rect.right, rect.bottom, 0);
						rectAfx = rect;
					}
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

		// 2011.05.10 終了時はあふwにフォーカスを戻す
		SetFocusOtherProcess(_hAfxWnd);
	}

	::UnmapViewOfFile(smode);
	::CloseHandle(hShare);
	::CloseHandle(pi.hProcess);
	::CloseHandle(pi.hThread);
	::CloseHandle(hEvent);

	return 0;
}

