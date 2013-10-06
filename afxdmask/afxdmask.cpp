#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <windowsx.h>
#include "resource.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL InitApp(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
void SetFocusOtherProcess(HWND hWnd);
HWND GetFocusOtherProcess(HWND hWnd);
void SendCommand(HWND hWnd);
LRESULT CALLBACK SubProc(HWND hWnd, UINT message, WPARAM wp, LPARAM lp);
static const char AktAfxMessage[] = "AfxW_CD_Message_ID_AKT";

// グローバル変数
HINSTANCE	hInst;
HBRUSH		_bkBrush = NULL;
char		_app_path[MAX_PATH];
char		_class_name[] = "afxdmask";
char*		_cmd_line;
int			_cmd_line_len;
HWND		_hWndEdit;
HWND		_hWndThis;
HWND		_hWndAfx;
FARPROC		_hSubWnd;
HWND		_hWndAfxRightLabel = 0;
HWND		_hWndAfxLeftLabel = 0;
HWND		_hWndAfxCurFileList = 0;
int			_TPalnelCnt = 0;
int			_TFileBoxCnt = 0;
BOOL		_is_left;

BOOL CALLBACK EnumChildProc(HWND hWnd , LPARAM lp)
{
	char className[256];
	if (::GetClassName(hWnd, className, sizeof(className))) {
		if (strcmp(className, "TPanel") == 0) {
			_TPalnelCnt++;
			if (_TPalnelCnt == 4) {
				_hWndAfxLeftLabel = hWnd;
			}
			if (_TPalnelCnt == 8) {
				_hWndAfxRightLabel = hWnd;
			}
		}
		if (strcmp(className, "TFileBox") == 0) {
			_TFileBoxCnt++;
			if (_TFileBoxCnt == 1) {
                if (::GetFocusOtherProcess(_hWndAfx) == hWnd) {
                    _is_left = TRUE;
                    _hWndAfxCurFileList = hWnd;
                } else {
                    _is_left = FALSE;
                }
            } else if (_TFileBoxCnt == 2) {
                if (_is_left == FALSE) {
                    _hWndAfxCurFileList = hWnd;
                }
            }
        }
	}

	return TRUE;
}

int WINAPI WinMain(HINSTANCE hCurInst, HINSTANCE hPrevInst,
                   LPSTR lpsCmdLine, int nCmdShow)
{
	_cmd_line = lpsCmdLine;
	_cmd_line_len = strlen(_cmd_line);

    if (!hPrevInst) {
        if (!InitApp(hCurInst))
            return FALSE;
    }
    if (!InitInstance(hCurInst, nCmdShow)) {
        return FALSE;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DeleteObject(_bkBrush);
    SetFocusOtherProcess(_hWndAfx);
    LockSetForegroundWindow(LSFW_UNLOCK);

    return msg.wParam;
}

BOOL InitApp(HINSTANCE hInst)
{
	// あふwのウィンドウ検索
	_hWndAfx = ::FindWindow("TAfxWForm", NULL);
	if (_hWndAfx == INVALID_HANDLE_VALUE) {
		_hWndAfx = ::FindWindow("TAfxForm", NULL);
		if (_hWndAfx == INVALID_HANDLE_VALUE) {
			return -1;
		}
	}

	// リソース初期化
	_bkBrush = CreateSolidBrush(RGB(0,0,0));

	// afxw.exe パス解決
	GetModuleFileName(hInst, _app_path, sizeof(_app_path));
	char* en = strrchr(_app_path, '\\');
	if (en == NULL) {
		return FALSE;
	}
	*(en+1) = '\0';
	strcat(_app_path, "afxw.exe");

	// ウィンドウクラスの登録
    WNDCLASS wc;
    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc      = WndProc;    //プロシージャ名
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = hInst;        //インスタンス
    wc.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = (LPCSTR)_class_name;
    return (RegisterClass(&wc));
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    // ウィンドウ位置解決
	::EnumChildWindows(_hWndAfx, EnumChildProc, NULL);
    POINT parentPos;
    parentPos.x = 0; parentPos.y = 0;
    ::ClientToScreen(_hWndAfx, &parentPos);
    RECT rectCns;
    if (_is_left) {
        ::GetWindowRect(_hWndAfxLeftLabel, &rectCns);
    } else {
        ::GetWindowRect(_hWndAfxRightLabel, &rectCns);
    }
    int posx = rectCns.left - parentPos.x;
    int posy = rectCns.top - parentPos.y;
    int width = rectCns.right - rectCns.left;
    int height = rectCns.bottom - rectCns.top;

    HWND hWnd;
    hInst = hInstance;
    hWnd = CreateWindow(_class_name,
            "afxdmask",
            WS_CHILD,
            posx,
            posy,
            width,
            height,
            _hWndAfx,
            NULL,
            hInstance,
            NULL);
    if (!hWnd) {
        return FALSE;
	}

    ::ShowWindow(hWnd, nCmdShow);
    ::BringWindowToTop(hWnd);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    RECT rc;
	HDC hDC;
    switch (msg) {
        case WM_CREATE:
            _hWndThis = hWnd;
            GetClientRect(hWnd, &rc);
            _hWndEdit = CreateWindow(
                "EDIT",
                NULL,
                WS_CHILD | WS_VISIBLE,
                0, 0,
                rc.right, rc.bottom,
                hWnd,
                (HMENU)ID_EDIT,
                hInst,
                NULL);
            SendMessage(_hWndEdit, EM_SETLIMITTEXT, (WPARAM)256, 0);
            SetWindowText(_hWndEdit, _cmd_line);
            ::SendMessage(_hWndEdit, EM_SETSEL, _cmd_line_len, _cmd_line_len);
			/*
			2011.06.01 v0.1.1 起動時にちらつくので修正
			if (_cmd_line_len > 0) {
            	SendCommand(_hWndEdit);
			}
			*/
            _hSubWnd = (FARPROC)GetWindowLong(_hWndEdit, GWL_WNDPROC);
            SetWindowLong(_hWndEdit, GWL_WNDPROC, (LONG)SubProc);
            SetFocus(_hWndEdit);
            break;

        case WM_CTLCOLOREDIT:
            if ((HWND)lp == _hWndEdit) {
                hDC = (HDC)wp;
                SetBkMode(hDC, TRANSPARENT);
                SetTextColor(hDC, RGB(200,200,200));
                return (LRESULT)_bkBrush;
            }
            break;

        case WM_KILLFOCUS:
            DestroyWindow(hWnd);
            break;

        case WM_ACTIVATE:
            if ((wp & 0xFFFF) == 0x0000) {
                DestroyWindow(hWnd);
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return (DefWindowProc(hWnd, msg, wp, lp));
    }
    return 0L;
}

LRESULT CALLBACK SubProc(HWND hWnd, UINT message, WPARAM wp, LPARAM lp)
{
    switch (message)
    {
        case WM_KEYDOWN:
            LockSetForegroundWindow(LSFW_LOCK);
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case WM_KEYUP:
            SendCommand(hWnd);
            break;

        case WM_CHAR:
            if (wp == VK_RETURN || wp == VK_ESCAPE) {
                if (wp == VK_ESCAPE) {
                    SetWindowText(_hWndEdit, "*");
                    SendCommand(hWnd);
                }
                DestroyWindow(_hWndThis);
            }
    }

    return (CallWindowProc((WNDPROC)_hSubWnd, hWnd, message, wp, lp));
}

void SetFocusOtherProcess(HWND hWnd)
{
	DWORD buf;
	DWORD pid;
	DWORD tid   = ::GetWindowThreadProcessId(hWnd, &pid);
	DWORD tidto = ::GetCurrentThreadId();
	::AttachThreadInput(tid, tidto, TRUE);
	::SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &buf, 0);
    ::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, NULL, 0);
	::BringWindowToTop(hWnd);
	::SetForegroundWindow(hWnd);
    ::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &buf, 0);
	::AttachThreadInput(tidto, tid, FALSE);
}

HWND GetFocusOtherProcess(HWND hWnd)
{
	DWORD pid;
	DWORD tid   = ::GetWindowThreadProcessId(hWnd, &pid);
	DWORD tidto = ::GetCurrentThreadId();
	::AttachThreadInput(tid, tidto, TRUE);
    hWnd = GetFocus();
	::AttachThreadInput(tidto, tid, FALSE);
    return hWnd;
}

void SendCommand(HWND hWnd)
{
	char szMask[256];
	::GetWindowText(hWnd, szMask, sizeof(szMask));

	/*
	PROCESS_INFORMATION pi;
	*/
	STARTUPINFO si;
	ZeroMemory(&si,sizeof(si));
	si.cb=sizeof(si);
	
	int len = strlen(szMask);
	if (len < sizeof(szMask)-1) {
		if (len == 0 || szMask[len-1] != '*') {
			strcat(szMask, "*");
		}
	}

	char cmd[256];
	sprintf(cmd, 
//			"%s -S -*P\"%s\"",
			"%s -*P\"%s\"",
			_app_path,
			szMask);

	/*
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
		return;
	}
    */
	UINT mes;
	ATOM atCmd = 0;
    if ((mes = RegisterWindowMessage(AktAfxMessage)) != 0 &&
        (atCmd = GlobalAddAtom(cmd)) != 0) {
        PostMessage(_hWndAfx, mes, (WPARAM)1, (LPARAM)atCmd);
    }

	/*
	::CloseHandle(pi.hProcess);
	::CloseHandle(pi.hThread);
	*/

	return;
}

