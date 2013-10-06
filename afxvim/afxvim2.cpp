#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HWND _hVimWnd = 0;
HANDLE _vimPid = 0;
DWORD _dwVimTID = 0;

BOOL CALLBACK EnumChildProc(HWND hWnd , LPARAM lp)
{
	char className[256];
	if (::GetClassName(hWnd, className, sizeof(className))) {
		if (strcmp(className, "Vim") == 0) {
			DWORD pid;
			DWORD tid = ::GetWindowThreadProcessId(hWnd, &pid);
			_hvimwnd = hwnd;
			_dwvimtid = tid;
			return FALSE;
		}
	}

	return TRUE;
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

	char cmd[256];
	sprintf(cmd, 
			"%s --windowid 0x%08x --cmd \":silent set go+=C\"",
			lpCmdLine,
			_hAfxWnd);

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

	//WaitForSingleObject( pi.hProcess, INFINITE );
	DWORD dwExCode;
	while (1) {
		Sleep(500);
		if (_hVimWnd == INVALID_HANDLE_VALUE || _hVimWnd == 0) {
			EnumChildWindows(_hAfxWnd, EnumChildProc , 0);
		}
		GetExitCodeProcess(pi.hProcess, &dwExCode);
		if (dwExCode == STILL_ACTIVE) {
			HWND active = ::GetForegroundWindow();
			if (active == _hAfxWnd) {
				if (_hVimWnd != INVALID_HANDLE_VALUE) {
					DWORD pid;
					DWORD tid = ::GetWindowThreadProcessId(_hAfxWnd, &pid);
					::AttachThreadInput(tid, _dwVimTID, TRUE);
					::SetActiveWindow(_hVimWnd);
					::SetForegroundWindow(_hVimWnd);
					::SetFocus(_hVimWnd);
					::AttachThreadInput(tid, _dwVimTID, FALSE); 
				}
			}
			continue;
		} else  {
			break;
		}

	}

	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );

	return 0;
}

