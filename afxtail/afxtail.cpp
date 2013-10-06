// AfxFind.cpp : アプリケーション用クラスの定義を行います。
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <share.h>
#include <locale.h>
#include "afxtail.h"
#include "afxcom.h"

#define MAX_LINE_SIZE     1024
#define __AFXTAIL_EVENT__ "__AFXTAIL_EVENT__"

char _prefix[64] = "[afxtail]";
IDispatch* pAfxApp = NULL;

static void Message(const char* fmt, ...)
{
	char line[MAX_LINE_SIZE];
	va_list arg;
	va_start(arg, fmt);
	vsprintf(line, fmt, arg);
	va_end(arg);

	//printf("%s\n", line);
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

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
/*
int main(int __argc, char** __argv)
*/
{
	int retval = 0;

	setlocale( LC_ALL, "Japanese" );

	if (__argc > 2) {
		Message("[usage]");
		Message("  afxtail.exe [file]");
		Message("  ※引数を省略した場合は既存のtailを終了する。");
		return -1;
	}
	if (__argc == 1) {
		HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, TRUE, __AFXTAIL_EVENT__);
		if (hEvent != INVALID_HANDLE_VALUE) {
			SetEvent(hEvent);
			CloseHandle(hEvent);
		}
		return 0;
	}

	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, __AFXTAIL_EVENT__);

	char path[MAX_PATH];
	char dir[MAX_PATH];
	strcpy(path, __argv[1]);
	if (path[1] == ':') {
		strcpy(dir, __argv[1]);
		char* en = strrchr(dir, '\\');
		if (en == NULL) {
			Message("illegal parameter (%s)", __argv[1]);
			retval = -1;
			goto END;
		}
		*(en+1) = '\0';
	} else {
		DWORD size = sizeof(dir);
		if (::GetCurrentDirectory(size, dir) == FALSE) {
			Message("GetCurrentDirectory error(%d)", GetLastError());
			retval = -1;
			goto END;
		}
	}

	AfxInit(pAfxApp);

	HANDLE hNotif;
	FILE* fp = NULL;
	int cur = 0;
	int last = 0;
	bool first = true;
	char line[MAX_LINE_SIZE];
	while (1) {
		fp = _fsopen(path, "r", _SH_DENYNO);
		if (fp == NULL) {
			Message("Can not open file (%s)", path);
			break;
		}
		fseek(fp, 0, SEEK_END);
        last = ftell(fp);
		if (first) {
			first = false;
			cur = last - MAX_LINE_SIZE * 10;
			if (cur < 0) cur = 0;
			hNotif = FindFirstChangeNotification(dir, TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE);
			int hist[6] = { -1, -1, -1, -1, -1, -1 };
			fseek(fp, cur, SEEK_SET);
			while (fgets(line, sizeof(line)-1, fp) != NULL) {
				hist[5] = hist[4];
				hist[4] = hist[3];
				hist[3] = hist[2];
				hist[2] = hist[1];
				hist[1] = hist[0];
				hist[0] = ftell(fp);
			}
			for (int idx=5; idx>=0; idx--) {
				if (hist[idx] >= 0) {
					cur = hist[idx];
					break;
				}
			}
			Message("-- afxtail start --");
		}
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

		if (hNotif == INVALID_HANDLE_VALUE) {
			Sleep(5000);
		} else {
			while (WaitForSingleObject(hNotif, 1000) != WAIT_OBJECT_0) {
				if (WaitForSingleObject(hEvent, 0) == WAIT_OBJECT_0) {
					SetEvent(hEvent);
					Message("-- afxtail end(%s) --", path);
					goto END;
				}
			}
			if (!FindNextChangeNotification(hNotif)) {
				break;
			}
		}
	}

END:
	if (hEvent != INVALID_HANDLE_VALUE) {
		CloseHandle(hEvent);
	}

	AfxCleanup(pAfxApp);

	return retval;
}

