#include <tchar.h>
#include <windows.h>
#include <crtdbg.h>
#include "IEComponent.h"

#define IDC_BTN_EXIT       1100
#define MAX_URL            4096
#define WM_LOADURL         WM_USER + 1000
#define __EVENT_NAME__     "__AFXBRWS_EVENT_NAME__"
#define __SHAREDMEM_NAME__ "__AFXBRWS_SHAREMEM_NAME__"

static IEComponent* _ie = NULL;
static HWND   _hAfxWnd = NULL;
static HWND   _hBroWnd = NULL;
static HWND   _hIEWnd = NULL;
static DWORD  _bro_pid = 0;
static BOOL   _thread_exit = FALSE;
static BOOL   _fullscreen = FALSE;
static RECT   _rect_pre_fullscreen;
static HANDLE _hAfxBrwsEvent;
static char*  _SharedBufer = NULL;
static HANDLE _hThread;
static BOOL   _bHide = FALSE;
static BOOL   _stay_mode = FALSE;
static HWND   _hActiveXWnd = NULL;
static BOOL   _exist_activex = FALSE;
static BOOL   _replace_activex_proc = FALSE;

static WNDPROC DefIEProc;
static WNDPROC DefActiveXProc;

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

LRESULT CALLBACK _WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, WNDPROC proc)
{
	switch(msg) {
		case WM_LOADURL:
			if (_ie != NULL) {
				_ie->LoadPage(_SharedBufer);
			}
			SetFocusOtherProcess(_hIEWnd);
			::SetFocus(_hIEWnd);
			break;

		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE) {
				if (_stay_mode) {
					SuspendThread(_hThread);

					if (_fullscreen) {
						::MoveWindow(_hAfxWnd, 
							_rect_pre_fullscreen.left,
							_rect_pre_fullscreen.top,
							_rect_pre_fullscreen.right - _rect_pre_fullscreen.left,
							_rect_pre_fullscreen.bottom - _rect_pre_fullscreen.top,
							TRUE);
					}
					::ShowWindow(_hBroWnd, SW_HIDE);
					SetFocusOtherProcess(_hAfxWnd);
					::SetFocus(_hAfxWnd);
					_bHide = TRUE;

					ResumeThread(_hThread);
				} else {
					PostQuitMessage (0);
				}
			} else if (wParam == VK_F11) {
				if (_fullscreen) {
					::MoveWindow(_hAfxWnd, 
						_rect_pre_fullscreen.left,
						_rect_pre_fullscreen.top,
						_rect_pre_fullscreen.right - _rect_pre_fullscreen.left,
						_rect_pre_fullscreen.bottom - _rect_pre_fullscreen.top,
						TRUE);
					_fullscreen = FALSE;
				} else {
					::GetWindowRect(_hAfxWnd, &_rect_pre_fullscreen);
					::MoveWindow(_hAfxWnd, 
						GetSystemMetrics(SM_XVIRTUALSCREEN),
						GetSystemMetrics(SM_YVIRTUALSCREEN),
						GetSystemMetrics(SM_CXVIRTUALSCREEN),
						GetSystemMetrics(SM_CYVIRTUALSCREEN),
						TRUE);
					_fullscreen = TRUE;
				}
			}
			return 0;
	}

	return CallWindowProc(proc, hWnd, msg, wParam, lParam);
}
LRESULT CALLBACK WndProc4IE(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return _WndProc(hWnd, msg, wParam, lParam, DefIEProc);
}
LRESULT CALLBACK WndProcActiveX(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return _WndProc(hWnd, msg, wParam, lParam, DefActiveXProc);
}

HWND GetIEHWND(HWND hParent)
{
    HWND hWnd;
    char szClassName[255];
    hWnd = GetWindow(hParent,GW_CHILD);
    if (hWnd) {
        ::GetClassName(hWnd, szClassName, 255);
		_RPT1(_CRT_WARN, "%s\n", szClassName);
        if (strcmp(szClassName,"Internet Explorer_Server") == 0 /*|| 
			strcmp(szClassName,"Shell Embedding") == 0*/)  {
            return hWnd;
        } else {
            return GetIEHWND(hWnd);
        }
    }
    return NULL;
}

BOOL IsInternalAfx(HWND hWnd)
{
	DWORD tid;
	if (_bro_pid == 0) {
		//tid = ::GetWindowThreadProcessId(_hBroWnd, &_bro_pid);
		tid = ::GetWindowThreadProcessId(_hIEWnd, &_bro_pid);
	}

	DWORD pid;
	tid = ::GetWindowThreadProcessId(hWnd, &pid);

	HWND parent = GetParent(hWnd);
	if (pid == _bro_pid) {
		return TRUE;
	}

	if ((GetWindowLong(hWnd, GWL_STYLE) & WS_CHILDWINDOW) != 0) {
		return TRUE;
	}

	if (IsWindowVisible(hWnd) == FALSE) {
		return TRUE;
	}

	return FALSE;
}

BOOL CALLBACK EnumChildProc(HWND hWnd , LPARAM lp)
{
	char class_name[255];
	GetClassName(hWnd, class_name, sizeof(class_name));
	if (strcmp(class_name, "AcrobatSDIWindow") == 0 ||
		strcmp(class_name, "OpusApp")          == 0 ||
		strcmp(class_name, "PP7FrameClass")    == 0 ||
		strcmp(class_name, "PP8FrameClass")    == 0 ||
		strcmp(class_name, "PP9FrameClass")    == 0 ||
		strcmp(class_name, "PP10FrameClass")   == 0 ||
		strcmp(class_name, "XLMAIN")           == 0 // ||
		//strcmp(class_name, "Shell Embedding")  == 0
		) {
	//if (strcmp(class_name, "AcrobatSDIWindow") == 0 ) {
		if (IsInternalAfx(hWnd)) {
			_exist_activex = TRUE;
			_hActiveXWnd = hWnd;
			return FALSE;
		}
	}
	return TRUE;
}


// ウィンドウ監視スレッド
DWORD WINAPI Thread1(LPVOID data)
{
	RECT rect;
	DWORD WndStyle;
	DWORD WndStyleEx;
	WndStyle   = ::GetWindowLong(_hAfxWnd, GWL_STYLE);
	WndStyleEx = ::GetWindowLong(_hAfxWnd, GWL_EXSTYLE);
	DWORD pid;
	DWORD tid = ::GetWindowThreadProcessId(_hAfxWnd, &pid);
	HANDLE hAfxProc = OpenProcess(SYNCHRONIZE, TRUE, pid);
	RECT rectAfx = { 0 };

	while (_thread_exit == FALSE) {
		DWORD ret = WaitForSingleObject(hAfxProc, 0);
		if (ret == WAIT_OBJECT_0) {
			break;
		}

		if (_bHide) {
			int ret = WaitForSingleObject(_hAfxBrwsEvent, 300);
			if (ret == WAIT_TIMEOUT) {
				continue;
			}

			::ShowWindow(_hBroWnd, SW_SHOW);
			if (_fullscreen) {
				::MoveWindow(_hAfxWnd, 
					GetSystemMetrics(SM_XVIRTUALSCREEN),
					GetSystemMetrics(SM_YVIRTUALSCREEN),
					GetSystemMetrics(SM_CXVIRTUALSCREEN),
					GetSystemMetrics(SM_CYVIRTUALSCREEN),
					TRUE);
			}
			::SendMessage(_hIEWnd, WM_LOADURL, 0, 0);
			_bHide = FALSE;
			rect.left = rect.right = rect.top = rect.bottom = 0;

		} else {

			HWND active = ::GetForegroundWindow();
			if (active == _hAfxWnd) {
				if (_hIEWnd != NULL) {
					_exist_activex = FALSE;
					EnumWindows(EnumChildProc , 0);
					if (!_exist_activex) {
						SetFocusOtherProcess(_hIEWnd);
						::SetFocus(_hIEWnd);
						_replace_activex_proc = FALSE;
					} else {
						SetFocusOtherProcess(_hActiveXWnd);
						::SetFocus(_hActiveXWnd);
						if (!_replace_activex_proc) {
							DefActiveXProc = (WNDPROC)SetWindowLong(_hActiveXWnd, GWL_WNDPROC, (LONG)WndProcActiveX);
							_replace_activex_proc = TRUE;
						}
					}
				}
			}

			if (_hIEWnd == NULL) {
				_hIEWnd = GetIEHWND(_hBroWnd);
				if (_hIEWnd != NULL) {
					DefIEProc = (WNDPROC)SetWindowLong(_hIEWnd, GWL_WNDPROC, (LONG)WndProc4IE);
				}
			}

			/* 2011.09.20 コメントアウト 
			::GetWindowRect(_hAfxWnd, &rect);
			if (( rect.right - rect.left) != ( rectAfx.right - rectAfx.left ) || 
				( rect.bottom - rect.top) != ( rectAfx.bottom - rectAfx.top ) ) {
				rectAfx = rect;

				AdjustWindowRectEx(&rect, WndStyle, FALSE, WndStyleEx);
				::SetWindowPos(
						_hBroWnd,
						HWND_TOP,
						0, 0, 
						rect.right - rect.left - GetSystemMetrics(SM_CXVSCROLL),
						rect.bottom - rect.top - GetSystemMetrics(SM_CYCAPTION) - GetSystemMetrics(SM_CYVTHUMB)*2,
						0);
			}
			*/
			::GetClientRect(_hAfxWnd, &rect);
			if (( rect.right - rect.left) != ( rectAfx.right - rectAfx.left ) || 
				( rect.bottom - rect.top) != ( rectAfx.bottom - rectAfx.top ) ) {
				::SetWindowPos( _hBroWnd, HWND_TOP, rect.left, rect.top, rect.right, rect.bottom, SWP_NOMOVE);
				rectAfx = rect;
			}

			Sleep(300);
		}
	}

	::CloseHandle(hAfxProc);

	PostQuitMessage (0);
	exit(0);

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nShowCmd)
{
	HANDLE hSharedBuffer = CreateFileMapping(
			INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
			0, MAX_URL, __SHAREDMEM_NAME__);
	if (hSharedBuffer != NULL) {
		_SharedBufer = (char*)::MapViewOfFile(hSharedBuffer, FILE_MAP_ALL_ACCESS, 0, 0, MAX_URL);
	}
	if (::GetLastError() == ERROR_ALREADY_EXISTS) {
		hSharedBuffer = OpenFileMapping( FILE_MAP_ALL_ACCESS, TRUE, __SHAREDMEM_NAME__);
		if (hSharedBuffer != NULL) {
			_SharedBufer = (char*)::MapViewOfFile(hSharedBuffer, FILE_MAP_ALL_ACCESS, 0, 0, MAX_URL);
			memset(_SharedBufer, 0, MAX_URL);
			strncpy(_SharedBufer, lpCmdLine, MAX_URL-1);
			_hAfxBrwsEvent = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, __EVENT_NAME__);
			if (_hAfxBrwsEvent != NULL) {
				::SetEvent(_hAfxBrwsEvent);
				::CloseHandle(_hAfxBrwsEvent);
			}
			::UnmapViewOfFile(_SharedBufer);
			::CloseHandle(hSharedBuffer);
		}
		return -1;
	}

	_hAfxWnd = ::FindWindow(_T("TAfxWForm"), NULL);
	if (_hAfxWnd == INVALID_HANDLE_VALUE) {
		_hAfxWnd = ::FindWindow(_T("TAfxForm"), NULL);
		if (_hAfxWnd == INVALID_HANDLE_VALUE) {
			::UnmapViewOfFile(_SharedBufer);
			::CloseHandle(hSharedBuffer);
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
	_stay_mode = ::GetPrivateProfileInt("Config",  "stay", 1, ini_path);

	_hAfxBrwsEvent = ::CreateEvent(NULL, FALSE, FALSE, __EVENT_NAME__);

	RECT rect;
	::GetWindowRect(_hAfxWnd, &rect);
	IEComponent ie(_hAfxWnd, hInstance, 0, 0, rect.right - rect.left, rect.bottom - rect.top);
	ie.LoadPage(lpCmdLine);
	_hBroWnd = ie.GetWndIE();
	SetFocusOtherProcess(ie.GetWndIE());
	IOleInPlaceActiveObject* actObj = ie.GetActObj();
	_ie = &ie;

	DWORD thId1;
	_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Thread1, NULL, 0, &thId1);

	MSG	msg;
	while (GetMessage (&msg, NULL, 0, 0))
	{
		if (actObj->TranslateAccelerator(&msg)) {
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}
	}

	if (_fullscreen) {
		::MoveWindow(_hAfxWnd, 
			_rect_pre_fullscreen.left,
			_rect_pre_fullscreen.top,
			_rect_pre_fullscreen.right - _rect_pre_fullscreen.left,
			_rect_pre_fullscreen.bottom - _rect_pre_fullscreen.top,
			TRUE);
	}

	_thread_exit = TRUE;
	::WaitForSingleObject(_hThread, INFINITE);
	::CloseHandle(_hAfxBrwsEvent);
	::UnmapViewOfFile(_SharedBufer);
	::CloseHandle(hSharedBuffer);
	::CloseHandle(_hThread);

	SetFocusOtherProcess(_hAfxWnd);

	return msg.wParam;
};

