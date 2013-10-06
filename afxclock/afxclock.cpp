// afxclock.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include <windows.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#define __EVENT_NAME__  "__AFXCLOCK_EVENT_NAME__"
#define LEFT_MODE       0
#define RIGHT_MODE      1

static int    _TPalnelCnt = 0;
static HWND   _hWndAfxRightLabel = 0;
static HANDLE _hKillEvent;
static DWORD  _mode = RIGHT_MODE;

BOOL CALLBACK EnumChildProc(HWND hWnd , LPARAM lp)
{
	char className[256];
	if (::GetClassName(hWnd, className, sizeof(className))) {
		if (strcmp(className, "TPanel") == 0) {
			_TPalnelCnt++;
			if (_mode == LEFT_MODE) {
				if (_TPalnelCnt == 3) {
					_hWndAfxRightLabel = hWnd;
					return FALSE;
				}
			} else if (_mode == RIGHT_MODE) {
				if (_TPalnelCnt == 7) {
					_hWndAfxRightLabel = hWnd;
					return FALSE;
				}
			}
		}
	}

	return TRUE;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	char tszText[256];
	HANDLE hAfxClockEvent;
	DWORD ret;

	HANDLE hMutex = ::CreateMutex(NULL, TRUE, "AfxClockMutex");
	if (::GetLastError() == ERROR_ALREADY_EXISTS) {
		hAfxClockEvent = ::OpenEvent(EVENT_ALL_ACCESS, FALSE, __EVENT_NAME__);
		if (hAfxClockEvent != NULL) {
			::SetEvent(hAfxClockEvent);
			::CloseHandle(hAfxClockEvent);
		}
		return -1;
	}

	if (__argc >= 2) {
		if (strcmp(__argv[1], "-l") == 0) {
			_mode = LEFT_MODE;
		} else if (strcmp(__argv[1], "-r") == 0) {
			_mode = RIGHT_MODE;
		}
	}

	hAfxClockEvent = ::CreateEvent(NULL, FALSE, FALSE, __EVENT_NAME__);

	HWND hWndAfx = ::FindWindow("TAfxWForm", NULL);
	if (hWndAfx == INVALID_HANDLE_VALUE) {
		hWndAfx = ::FindWindow("TAfxForm", NULL);
		if (hWndAfx == INVALID_HANDLE_VALUE) {
			::CloseHandle(hMutex);
			::CloseHandle(_hKillEvent);
			return -1;
		}
	}

	::EnumChildWindows(hWndAfx, EnumChildProc, NULL);
	HWND hWnd = _hWndAfxRightLabel;

	while (1) {
		// time
		time_t _time;
		_time = time(NULL);
		tm* _tm = localtime(&_time);
		memset(tszText, 0, sizeof(tszText));
		strftime(tszText, sizeof(tszText)-1, "AFXCLOCK: %Y/%m/%d %H:%M:%S", _tm);

		BOOL bRet = ::SendMessage(hWnd, WM_SETTEXT, NULL, (LPARAM)tszText);
		if (bRet == FALSE) {
			break;
		}
		::InvalidateRect(hWnd, NULL, TRUE);

		//Sleep(1000);
		ret = WaitForSingleObject(hAfxClockEvent, 1000);
		if (ret != WAIT_TIMEOUT) {
			break;
		}
	}

	::SendMessage(hWnd, WM_SETTEXT, NULL, (LPARAM)"AFXCLOCK END!!");
	::InvalidateRect(hWnd, NULL, TRUE);

	::CloseHandle(hMutex);
	::CloseHandle(hAfxClockEvent);

	return 0;
}

