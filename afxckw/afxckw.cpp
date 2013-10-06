#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "resource.h"

#define AFXCKW_SHAREDMEM_NAME "__afxckw_shared_memory__"
#define CKW_SHAREDBUFFER_NAME "__ckw_shared_buffer__"
#define FOCUS_AFX 0
#define FOCUS_CKW 1

typedef enum {
	OPTION_n,		// none spec
	OPTION_e,
	OPTION_E,
	OPTION_f,
	OPTION_F,
	OPTION_g,
	OPTION_MAX,
} OPTION_MODE;

HINSTANCE _hInstance;
HWND _hWndAfxCkw = NULL;
OPTION_MODE _mode = OPTION_n;
HANDLE _ckwPid = 0;
HANDLE _ckwTid = 0;
HWND _hConsoleWnd = NULL;
HWND _hTextView = NULL;
BOOL _halfMode = TRUE;

void DebugPrint(const char* str, ...)
{
    va_list argp;
    char szBuf[256];

    va_start(argp, str);
    vsprintf(szBuf, str, argp);
    va_end(argp);
    OutputDebugString(szBuf);
}


BOOL CreateCkwProcess(HWND parent)
{
	char app_path[MAX_PATH];
	GetModuleFileName(_hInstance, app_path, sizeof(app_path));
	char* en = strrchr(app_path, '\\');
	if (en == NULL) {
		return FALSE;
	}
	*(en+1) = '\0';
	strcat(app_path, "ckw.exe");

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	ZeroMemory(&si,sizeof(si));
	si.cb=sizeof(si);

	char cmd[256];
	sprintf(cmd, 
			"%s -c 0x%08x",
			app_path,
			parent);

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
		return TRUE;
	}

	_ckwPid = pi.hProcess;
	_ckwTid = pi.hThread;

	return TRUE;
}


BOOL CALLBACK EnumChildProc(HWND hWnd , LPARAM lp)
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

BOOL CALLBACK EnumChildProc4FindConsole(HWND hWnd , LPARAM lp)
{
	char className[256];
	if (::GetClassName(hWnd, className, sizeof(className))) {
		if (strcmp(className, "TScrollBox") == 0) {
			_hConsoleWnd = hWnd;
		}
	}

	return TRUE;
}

BOOL CALLBACK EnumChildProc4TextView(HWND hWnd , LPARAM lp)
{
	char className[256];
	if (::GetClassName(hWnd, className, sizeof(className))) {
		if (strcmp(className, "TTextView") == 0) {
			_hTextView = hWnd;
		}
	}

	return TRUE;
}

void SendDataToCkw(HWND hWndCkw, LPTSTR lpCmdLine)
{
	COPYDATASTRUCT cd;
	memset(&cd, 0, sizeof(cd));
	int nData = strlen(&lpCmdLine[2])+1;
	if (nData <= 1) return;

	char* pData = new char[nData];
	memset(pData, 0, nData);
	strcpy(pData, &lpCmdLine[2]);
	cd.dwData = 1;
	cd.cbData = nData;
	cd.lpData = pData;
	::SendMessage(hWndCkw, WM_COPYDATA, NULL, (LPARAM)&cd);
	delete [] pData;
}

void SetFocusOtherProcess(HWND hWndDst)
{
	DWORD pid;
	DWORD tidSrc = ::GetCurrentThreadId();
	DWORD tidDst = ::GetWindowThreadProcessId(hWndDst, &pid);
	::AttachThreadInput(tidSrc, tidDst, TRUE);
	::SetActiveWindow(hWndDst);
	::SetForegroundWindow(hWndDst);
	::SetFocus(hWndDst);
	::AttachThreadInput(tidSrc, tidDst, FALSE);
}

HWND GetFocusOwnProcss(HWND hWndDst)
{
	DWORD pid;
	DWORD tidSrc = ::GetCurrentThreadId();
	DWORD tidDst = ::GetWindowThreadProcessId(hWndDst, &pid);
	::AttachThreadInput(tidSrc, tidDst, TRUE);
	HWND hwnd = ::GetFocus();
	::AttachThreadInput(tidSrc, tidDst, FALSE);
	return hwnd;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	_hInstance = hInstance;
	if (strncmp(lpCmdLine, "-e", 2) == 0) {
		_mode = OPTION_e;
	} else
	if (strncmp(lpCmdLine, "-E", 2) == 0) {
		_mode = OPTION_E;
	} else
	if (strncmp(lpCmdLine, "-f", 2) == 0) {
		_mode = OPTION_f;
	} else
	if (strncmp(lpCmdLine, "-F", 2) == 0) {
		_mode = OPTION_F;
	}
	if (strncmp(lpCmdLine, "-g", 2) == 0) {
		_mode = OPTION_g;
	}

	HWND hWndLastFocus = ::GetFocus();

	// あふのウィンドウを探す
	HWND hWndAfx = ::FindWindow("TAfxWForm", NULL);
	if (hWndAfx == INVALID_HANDLE_VALUE) {
		hWndAfx = ::FindWindow("TAfxForm", NULL);
		if (hWndAfx == INVALID_HANDLE_VALUE) {
			return -1;
		}
	}

	char ini_path[MAX_PATH];
	GetModuleFileName(NULL, ini_path, sizeof(ini_path));
	int len = strlen(ini_path);
	if (len+2 >= MAX_PATH) {
		return -1;
	}
	ini_path[len-3] = 'i';
	ini_path[len-2] = 'n';
	ini_path[len-1] = 'i';
	_halfMode = ::GetPrivateProfileInt("Config",  "HalfMode", 0, ini_path);

	// 共有メモリからバッファを取得して書き出す
	if (_mode == OPTION_g) {
		char *SharedBufer = NULL;
		HANDLE hSharedBuffer = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
				0, 4096, CKW_SHAREDBUFFER_NAME);
		if (hSharedBuffer != NULL) {
			SharedBufer = (char*)::MapViewOfFile(hSharedBuffer, FILE_MAP_READ, 0, 0, 4096);
		}
		printf(SharedBufer);
		
		return 0;
	}

	// 既存のCKWがあれば、コマンドを流し込むだけ
	::EnumChildWindows(hWndAfx, EnumChildProc, NULL);
	if (_hWndAfxCkw != NULL) {
		if (_mode != OPTION_n) {
			SendDataToCkw(_hWndAfxCkw, lpCmdLine);
		}
		if (_mode == OPTION_E || _mode == OPTION_n) {
			SetFocusOtherProcess(_hWndAfxCkw);
		}
		return 0;
	}

	// 2011.05.09 共有メモリ上に動作モードを持つ
	int *smode = NULL;
	HANDLE hShare = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
			0, 8, AFXCKW_SHAREDMEM_NAME);
	if (hShare != NULL) {
		smode = (int*)::MapViewOfFile(hShare, FILE_MAP_WRITE, 0, 0, 4);
	}
	smode[0] = _mode;
	smode[1] = FOCUS_CKW;

	// ckwを起動する
	if (CreateCkwProcess(hWndAfx) == FALSE) {
		return -1;
	}

	// ckwの起動を待ってからコマンドを流す
	for (int idx=0; idx<30; idx++) {
		::EnumChildWindows(hWndAfx, EnumChildProc, NULL);
		if (_hWndAfxCkw != NULL) {
			break;
		}
		Sleep(100);
	}
	if (_hWndAfxCkw == NULL) {
 		return -1;
	}

	// 2011.05.06 最前面に移動
	::BringWindowToTop(_hWndAfxCkw);
	::SetForegroundWindow(_hWndAfxCkw);

	// 2011.05.06 コンソール窓のウィンドウハンドル取得
	::EnumChildWindows(hWndAfx, EnumChildProc4FindConsole, NULL);

	if (_mode != OPTION_n) {
		SendDataToCkw(_hWndAfxCkw, lpCmdLine);
	}

	if (_mode == OPTION_e) {
		SetFocusOtherProcess(hWndAfx);
		smode[1] = FOCUS_AFX;
	} else {
		SetFocusOtherProcess(_hWndAfxCkw);
	}

	RECT rect;
	RECT rectAfx = { 0 };

	// 2011.05.07 コンソールは非表示にしておく
	if (!_halfMode) {
		::ShowWindow(_hConsoleWnd, SW_HIDE);
	}

	bool bFirstLoop = true;
	// fオプションの場合は、ckwを監視してフォーカスがあふに移ったら、ckwにフォーカスを移す。
	DWORD dwExCode;
	while (1) {
		if (bFirstLoop) {
			dwExCode = WAIT_TIMEOUT;
		} else {
			dwExCode = WaitForSingleObject(_ckwPid, 500);
		}
		if (dwExCode == WAIT_TIMEOUT) {
			if (smode[0] != _mode) {
				_mode = (OPTION_MODE)smode[0];
				memset(&rectAfx, 0, sizeof(rectAfx));
			}

			if (_mode == OPTION_f || _mode == OPTION_F) {
				HWND active = ::GetForegroundWindow();
				if (active == hWndAfx) {
					SetFocusOtherProcess(_hWndAfxCkw);
				}
			}

			// 2011.06.19 フォーカス監視対応
			HWND hWndFocus = GetFocusOwnProcss(hWndAfx);
			DebugPrint("0x%08x\n", hWndFocus);
			if (hWndFocus == _hWndAfxCkw) {
				DebugPrint("ckw\n");
			} else if (IsChild(hWndAfx, hWndFocus)) {
				DebugPrint("あふw\n");
				if (smode[1] == FOCUS_CKW) {
					SetFocusOtherProcess(_hWndAfxCkw);
				}
			}

			// 2011.06.19 テキストビュー監視
			::EnumChildWindows(hWndAfx, EnumChildProc4TextView, NULL);
			if (IsWindowVisible(_hTextView)) {
				if (IsWindowVisible(_hWndAfxCkw)) {
					if (!_halfMode) {
						::ShowWindow(_hConsoleWnd, SW_SHOW);
					}
					::ShowWindow(_hWndAfxCkw,  SW_HIDE);
				}
			} else {
				if (!IsWindowVisible(_hWndAfxCkw)) {
					::ShowWindow(_hWndAfxCkw,  SW_SHOW);
					if (!_halfMode) {
						::ShowWindow(_hConsoleWnd, SW_HIDE);
					}
				}
			}

			// 2011.05.06 あふのウィンドウサイズを監視対応
			//::GetWindowRect(hWndAfx, &rect);
			::GetClientRect(hWndAfx, &rect);
			if (( rect.right - rect.left) != ( rectAfx.right - rectAfx.left ) || 
				( rect.bottom - rect.top) != ( rectAfx.bottom - rectAfx.top ) ) {

				if (_mode == OPTION_f || _mode == OPTION_F) {
					::SetWindowPos(
							_hWndAfxCkw,
							HWND_TOP,
							0, 0, 
							//rect.right - rect.left - GetSystemMetrics(SM_CXBORDER)*2 - 4,
							//rect.bottom - rect.top - GetSystemMetrics (SM_CYCAPTION) - GetSystemMetrics(SM_CYBORDER)*2 - 4,
							rect.right - rect.left,
							rect.bottom - rect.top,
							0);
					rectAfx = rect;
				} else {
					if (_hConsoleWnd != NULL) {
						POINT parentPos;
						RECT rectCns, rectAfxCli;
						parentPos.x = 0; parentPos.y = 0;
						::ClientToScreen(hWndAfx, &parentPos);
						::GetWindowRect(_hConsoleWnd, &rectCns);
						::GetClientRect(hWndAfx, &rectAfxCli);
						int posx = rectCns.left - parentPos.x;
						int posy = rectCns.top - parentPos.y;
						//int width = rectCns.right - rectCns.left;
						//int height = rectcns.bottom - rectcns.top + 2;
						int width = rectAfxCli.right - rectAfxCli.left;
						int height = rectCns.bottom - rectCns.top;
						if (_halfMode) {
							width  /= 2;
							::SetWindowPos( _hConsoleWnd, HWND_TOP, posx, posy, width, height, SWP_NOMOVE);
							posx = rectCns.left - parentPos.x + width;
						}
						//::MoveWindow(_hWndAfxCkw, posx, posy, width, height, TRUE);
						::SetWindowPos( _hWndAfxCkw, HWND_TOP, posx, posy, width, height, 0);
						::InvalidateRect(_hWndAfxCkw, NULL, FALSE);
						::UpdateWindow(_hWndAfxCkw);
						rectAfx = rect;
					}
					if (bFirstLoop && _mode == OPTION_e) {
						//xxx
						smode[1] = FOCUS_AFX;
						SetFocusOtherProcess(hWndAfx);
					}
				}
			}
			if (bFirstLoop) {
				bFirstLoop = false;
			}
			continue;
		} else  {
			break;
		}
	}

	// 2011.05.07 コンソールを表示する
	if (!_halfMode) {
		::ShowWindow(_hConsoleWnd, SW_SHOW);
	} else {
		POINT parentPos;
		RECT rectCns, rectAfxCli;
		parentPos.x = 0; parentPos.y = 0;
		::ClientToScreen(hWndAfx, &parentPos);
		::GetWindowRect(_hConsoleWnd, &rectCns);
		::GetClientRect(hWndAfx, &rectAfxCli);
		int posx = rectCns.left - parentPos.x;
		int posy = rectCns.top - parentPos.y;
		int width = rectAfxCli.right - rectAfxCli.left;
		int height = rectCns.bottom - rectCns.top;
		::SetWindowPos( _hConsoleWnd, HWND_TOP, posx, posy, width, height, SWP_NOMOVE);
	}

	// 2011.05.07 終了時はあふwにフォーカスを戻す
	SetFocusOtherProcess(hWndAfx);

	::UnmapViewOfFile(smode);
	::CloseHandle(hShare);

	CloseHandle( _ckwPid );
	CloseHandle( _ckwTid );

	return 0;
}

