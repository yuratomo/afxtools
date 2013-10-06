// afxfazzy.cpp : アプリケーション用クラスの定義を行います。
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <locale.h>
#include <richedit.h>
#include <Tlhelp32.h>
#include "resource.h"
#include <vector>
#include "afxcom.h"

#define MODIFIER_ALT         0x0004
#define MODIFIER_CONTROL     0x0002
#define MODIFIER_SHIFT       0x0001
#define AFXFAZZY_SHAREDMEM   "__yuratomo_afxfazzy_shared_memory"
#define WM_AFXFAXXY_START    WM_USER + 0x10000
#define WM_RELOAD            WM_AFXFAXXY_START + 0x0001
#define MAX_HILIGHT_NUM      3
#define MAX_HILIGHT_LINE     20

typedef struct _MatchInfo_T
{
	int start [MAX_HILIGHT_NUM];
	int length[MAX_HILIGHT_NUM];
	int lineLen;
	int num;
} MatchInfo_T;

// グローバル変数
int     pKeyCode = 89;
int     pModifier = 1;
HWND    _hAfxWnd = NULL;
HBRUSH  _brush_input = NULL;
HWND    _hWndInput = NULL;
HWND    _hWndList= NULL;
FARPROC _hWndProcInput;
char*   _buf = NULL;
int     _buf_len = 0;
char*   _work = NULL;
int     _work_len = 0;
char    _ini_path[MAX_PATH];
char    _dir_path[MAX_PATH];
wchar_t _dir_pathw[MAX_PATH];
wchar_t _ini_pathw[MAX_PATH];
char    _mnu_path[MAX_PATH];
char    _kill_prog[MAX_PATH];
int     _sleep_sendkey = 0;
int     _sleep_returnkey = 0;
BOOL    _send_enter = FALSE;
BOOL    _automation_exec = FALSE;
char    _title[256];
DWORD   _color_list_fg  = RGB(200,200,200);
DWORD   _color_list_bg  = RGB(0,0,0);
DWORD   _color_input_fg = RGB(200,200,200);
DWORD   _color_input_bg = RGB(0,0,0);
DWORD   _color_hilight[MAX_HILIGHT_NUM];
BOOL    _stay_mode = TRUE;
BOOL    _append_index = FALSE;
bool    _bLoadFromIni = false;
bool    _bHilight = true;
DWORD   _font_height = 8;
IDispatch* pAfxApp = NULL;

// プロトタイプ
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void SendKey();
void DoMask(HWND hWnd, HWND hWndList);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	DWORD r, g, b;

	//setlocale(LC_ALL, "japanese_japan.20932");
	setlocale( LC_ALL, "Japanese" );

	// iniファイル解析
	GetModuleFileName(hInstance, _ini_path, sizeof(_ini_path));
	int len = strlen(_ini_path);
	_ini_path[len-1] = 'i';
	_ini_path[len-2] = 'n';
	_ini_path[len-3] = 'i';
 	strcpy(_mnu_path, _ini_path);
	_mnu_path[len-1] = 'u';
	_mnu_path[len-2] = 'n';
	_mnu_path[len-3] = 'm';
	mbstowcs(_ini_pathw, _ini_path, sizeof(_ini_pathw));
	wchar_t* end = wcsrchr(_ini_pathw, L'\\');
	if (end != NULL) {
		memset(_dir_pathw, 0, sizeof(_dir_pathw));
		wcsncpy(_dir_pathw, _ini_pathw, end-_ini_pathw+1);
		wcstombs(_dir_path, _dir_pathw, sizeof(_dir_path));
	}

	// あふwのウィンドウ解決
	_hAfxWnd = ::FindWindow("TAfxWForm", NULL);
	if (_hAfxWnd == INVALID_HANDLE_VALUE) {
		_hAfxWnd = ::FindWindow("TAfxForm", NULL);
		if (_hAfxWnd == INVALID_HANDLE_VALUE) {
			return -1;
		}
	}

	char temp[8];
	::GetPrivateProfileString("Config", "title", "ふぁじ～めにゅ～", _title, sizeof(_title), _ini_path);
	::GetPrivateProfileString("Config", "output_path", _mnu_path, _mnu_path, sizeof(_mnu_path), _ini_path);
	::GetPrivateProfileString("Config", "kill_program", "afxfazzykill.vbs", _kill_prog, sizeof(_kill_prog), _ini_path);
	

	// すでにafxfazzyがある場合は、手前に表示する。非表示なら表示する。
	HANDLE hMutex = ::CreateMutex(NULL, TRUE, "__yuratomo_afxfazzy__");
	if (::GetLastError() == ERROR_ALREADY_EXISTS) {
		HWND hWndFazzy = ::FindWindow(NULL, _title);
		if (hWndFazzy != NULL) {
			if (::IsWindowVisible(hWndFazzy) == FALSE) {
				::ShowWindow(hWndFazzy, SW_SHOW);
			}
		}
		// 既存のafxfazzyにリロード要求をする
		int length = __argc+1;
		for (int idx=0; idx<__argc; idx++) {
			length += strlen(__argv[idx]);
		}
		HANDLE hShare = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, length, AFXFAZZY_SHAREDMEM);
		char *shmem = (char*)::MapViewOfFile(hShare, FILE_MAP_WRITE, 0, 0, length);
		char *p = shmem;
		for (int idx=1; idx<__argc; idx++) {
			strcpy(p, __argv[idx]);
			p += strlen(__argv[idx])+1;
		}
		//::SendMessage(hWndFazzy, WM_RELOAD, __argc-1, 0);
		::PostMessage(hWndFazzy, WM_RELOAD, __argc-1, 0);

		// afxfazzyを前面に表示する
		RECT rect, afxRect;
		::GetWindowRect(_hAfxWnd, &afxRect);
		::GetWindowRect(hWndFazzy, &rect);
		::MoveWindow(hWndFazzy, 
				(afxRect.left+afxRect.right)/2+(rect.left-rect.right)/2,
				(afxRect.top+afxRect.bottom)/2+(rect.top-rect.bottom)/2,
				rect.right-rect.left,
				rect.bottom-rect.top,
				FALSE);
		DWORD buf;
		DWORD pid;
		DWORD tid = ::GetWindowThreadProcessId(hWndFazzy, &pid);
		DWORD tidto = ::GetCurrentThreadId();
		::AttachThreadInput(tid, tidto, TRUE);
		::SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &buf, 0);
		::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, NULL, 0);
		::SetForegroundWindow( hWndFazzy );
		::BringWindowToTop( hWndFazzy );
		::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &buf, 0);
		::AttachThreadInput(tidto, tid, FALSE);

		::UnmapViewOfFile(shmem);
		::CloseHandle(hShare);
		
		return -1;
	}

	::GetPrivateProfileString("SendKey", "keycode", "1089", temp, sizeof(temp), _ini_path);
	sscanf(temp, "%1d%3d", &pModifier, &pKeyCode);
	_sleep_sendkey   = ::GetPrivateProfileInt("SendKey", "sendkey_sleep", 200, _ini_path);
	_send_enter      = ::GetPrivateProfileInt("SendKey", "send_returnkey", 0, _ini_path);
	_sleep_returnkey = ::GetPrivateProfileInt("SendKey", "send_returnkey_sleep", 200, _ini_path);
	_automation_exec = ::GetPrivateProfileInt("SendKey", "automation_exec", 0, _ini_path);
	_stay_mode       = ::GetPrivateProfileInt("Config",  "stay", 1, _ini_path);
	_bHilight        = ::GetPrivateProfileInt("Color",   "hilight_enable", 1, _ini_path);
	_font_height     = ::GetPrivateProfileInt("Font",    "height", 8, _ini_path);
	_append_index    = ::GetPrivateProfileInt("Config",  "menu_index", 0, _ini_path);

	// カラー指定読み込み
	::GetPrivateProfileString("Color", "list_fg", "", temp, sizeof(temp), _ini_path);
	if (strlen(temp) == 6) {
		sscanf(temp, "%02x%02x%02x", &r, &g, &b);
		_color_list_fg = RGB(r,g,b);
	}
	::GetPrivateProfileString("Color", "list_bg", "", temp, sizeof(temp), _ini_path);
	if (strlen(temp) == 6) {
		sscanf(temp, "%02x%02x%02x", &r, &g, &b);
		_color_list_bg = RGB(r,g,b);
	}
	::GetPrivateProfileString("Color", "input_fg", "", temp, sizeof(temp), _ini_path);
	if (strlen(temp) == 6) {
		sscanf(temp, "%02x%02x%02x", &r, &g, &b);
		_color_input_fg = RGB(r,g,b);
	}
	::GetPrivateProfileString("Color", "input_bg", "", temp, sizeof(temp), _ini_path);
	if (strlen(temp) == 6) {
		sscanf(temp, "%02x%02x%02x", &r, &g, &b);
		_color_input_bg = RGB(r,g,b);
	}

	if (_bHilight == true) {
		_color_hilight[0] = RGB(255, 0, 0);
		_color_hilight[1] = RGB(0, 255, 0);
		_color_hilight[2] = RGB(255, 255, 0);
		char key[32];
		for (int idx=0; idx<MAX_HILIGHT_NUM; idx++) {
			sprintf(key, "hilight%d", idx);
			::GetPrivateProfileString("Color", key, "", temp, sizeof(temp), _ini_path);
			if (strlen(temp) == 6) {
				sscanf(temp, "%02x%02x%02x", &r, &g, &b);
				_color_hilight[idx] = RGB(r,g,b);
			}
		}
	}
	
	// 背景色用ブラシ作成
	_brush_input = ::CreateSolidBrush(_color_input_bg);

	HINSTANCE hRich = LoadLibrary("RICHED32.DLL");

	// スレッドから通知される状態を表示するプログレス画面
	int ret = DialogBox(hInstance, (LPCSTR)IDD_AFXFAZZY_DIALOG, NULL, (DLGPROC)WindowProc);
	if (ret == -1) {
		char msg[256];
		sprintf(msg, "Create dialog error.(error code=%d)", GetLastError());
		MessageBox(NULL, msg, _title, MB_OK);
		::CloseHandle(hMutex);
		return -1;
	}

	FreeLibrary(hRich);

	if (_buf != NULL) {
		delete [] _buf;
	}
	_buf = NULL;
	_buf_len = 0;

	if (_work != NULL) {
		delete [] _work;
	}
	_work = NULL;
	_work_len = 0;

	if (_brush_input != NULL) {
		::DeleteObject(_brush_input);
	}

	::CloseHandle(hMutex);

	if (AfxInit(pAfxApp) == 0) {
		VARIANT x;
		x.vt = VT_BSTR;
		x.bstrVal = ::SysAllocString(L"afxfazzyを終了しました。");
		AfxExec(DISPATCH_METHOD, NULL, pAfxApp, L"MesPrint", 1, x);
		AfxCleanup(pAfxApp);
	}

	return 0;
}

void SendKey()
{
	Sleep(_sleep_sendkey);

	DWORD buf;
	DWORD pid;
	DWORD tid = ::GetWindowThreadProcessId(_hAfxWnd, &pid);
	DWORD tidto = ::GetCurrentThreadId();
	::AttachThreadInput(tid, tidto, TRUE);
	::SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &buf, 0);
    ::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, NULL, 0);

	::SetForegroundWindow( _hAfxWnd );
	::BringWindowToTop( _hAfxWnd );

	if (!_automation_exec) {
		if (pModifier & MODIFIER_ALT) {
			keybd_event(VK_MENU,0,KEYEVENTF_EXTENDEDKEY,0);
		}
		if (pModifier & MODIFIER_CONTROL) {
			keybd_event(VK_CONTROL,0,KEYEVENTF_EXTENDEDKEY,0);
		}
		if (pModifier & MODIFIER_SHIFT) {
			keybd_event(VK_LSHIFT,0,KEYEVENTF_EXTENDEDKEY,0);
		}
		keybd_event(pKeyCode,0,KEYEVENTF_EXTENDEDKEY,0);
		keybd_event(pKeyCode,0,KEYEVENTF_EXTENDEDKEY|KEYEVENTF_KEYUP,0);
		if (pModifier & MODIFIER_SHIFT) {
			keybd_event(VK_LSHIFT,0,KEYEVENTF_EXTENDEDKEY|KEYEVENTF_KEYUP,0);
		}
		if (pModifier & MODIFIER_CONTROL) {
			keybd_event(VK_CONTROL,0,KEYEVENTF_EXTENDEDKEY|KEYEVENTF_KEYUP,0);
		}
		if (pModifier & MODIFIER_ALT) {
			keybd_event(VK_MENU,0,KEYEVENTF_EXTENDEDKEY|KEYEVENTF_KEYUP,0);
		}
	}

	BOOL bExec = FALSE;
	char* ln = NULL;
	if (_send_enter == TRUE && (_work != NULL || _buf != NULL)) {
		if (_work != NULL) {
			ln = strchr(_work, '\n');
		} else {
			ln = strchr(_buf, '\n');
		}
		if ( (ln != NULL) && (strchr(ln+1, '\n') == NULL)) {
			if (_automation_exec) {
				char *d1, *d2, *d3;
				if (_work != NULL) {
					d1 = strchr(_work, '\"');
				} else {
					d1 = strchr(_buf, '\"');
				}
				if (d1 != NULL) {
					d2 = strchr(d1+1, '\"');
					if (d2 != NULL) {
						d3 = d2+1;
						while ( (*d3 == ' ') || (*d3 == '\t') ) {
							d3++;
						}

						int len = strlen(d3)+1;
						wchar_t* wbuf = new wchar_t[len];
						memset(wbuf, 0, 2*len);
						mbstowcs(wbuf, d3, 2*len);
						wchar_t* cr = wcsstr(wbuf, L"\n");
						if (cr != NULL) *cr = L'\0';

						if (AfxInit(pAfxApp) == 0) {
							VARIANT x;
							x.vt = VT_BSTR;
							x.bstrVal = ::SysAllocString(wbuf);
							AfxExec(DISPATCH_METHOD, NULL, pAfxApp, L"Exec", 1, x);
							AfxExec(DISPATCH_METHOD, NULL, pAfxApp, L"MesPrint", 1, x);
							AfxCleanup(pAfxApp);
						}
						delete [] wbuf;
						bExec = TRUE;
					}
				}
			} else {
				Sleep(_sleep_returnkey);
				keybd_event(VK_RETURN,0,0,0);
				keybd_event(VK_RETURN,0,KEYEVENTF_KEYUP,0);
			}
		}
	}

	if (_automation_exec && bExec == FALSE) {
		/*
		char command[MAX_PATH+10];
		sprintf(command, "&MENU %s", _mnu_path);
		int len = strlen(command)+1;
		wchar_t* wcommand = new wchar_t[len];
		memset(wcommand, 0, 2*len);
		mbstowcs(wcommand, command, 2*len);

		VARIANT x;
		x.vt = VT_BSTR;
		x.bstrVal = ::SysAllocString(wcommand);
		AfxInit(pAfxApp);
		AfxExec(DISPATCH_METHOD, NULL, pAfxApp, L"Exec", 1, x);
		AfxCleanup(pAfxApp);
		delete [] wcommand;
		*/
		if (pModifier & MODIFIER_ALT) {
			keybd_event(VK_MENU,0,KEYEVENTF_EXTENDEDKEY,0);
		}
		if (pModifier & MODIFIER_CONTROL) {
			keybd_event(VK_CONTROL,0,KEYEVENTF_EXTENDEDKEY,0);
		}
		if (pModifier & MODIFIER_SHIFT) {
			keybd_event(VK_LSHIFT,0,KEYEVENTF_EXTENDEDKEY,0);
		}
		keybd_event(pKeyCode,0,KEYEVENTF_EXTENDEDKEY,0);
		keybd_event(pKeyCode,0,KEYEVENTF_EXTENDEDKEY|KEYEVENTF_KEYUP,0);
		if (pModifier & MODIFIER_SHIFT) {
			keybd_event(VK_LSHIFT,0,KEYEVENTF_EXTENDEDKEY|KEYEVENTF_KEYUP,0);
		}
		if (pModifier & MODIFIER_CONTROL) {
			keybd_event(VK_CONTROL,0,KEYEVENTF_EXTENDEDKEY|KEYEVENTF_KEYUP,0);
		}
		if (pModifier & MODIFIER_ALT) {
			keybd_event(VK_MENU,0,KEYEVENTF_EXTENDEDKEY|KEYEVENTF_KEYUP,0);
		}
	}

	::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &buf, 0);
	::AttachThreadInput(tidto, tid, FALSE);
}

BOOL SetListTextColor(HWND hEdit, COLORREF color)
{
	CHARFORMAT cfm;
	memset(&cfm, 0, sizeof(CHARFORMAT));
	cfm.cbSize = sizeof(CHARFORMAT);
	cfm.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE | CFM_OFFSET;
	cfm.yHeight = _font_height*20;
	cfm.yOffset = 0;
	cfm.crTextColor = color;

	strcpy(cfm.szFaceName, "ＭＳ ゴシック");
	if (SendMessage(hEdit, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD, (LPARAM)&cfm) == 0) {
		return FALSE;
	}
	
	PARAFORMAT2 pfm;
	memset(&pfm, 0, sizeof(PARAFORMAT2));
	pfm.cbSize = sizeof(PARAFORMAT2);
	pfm.dwMask = PFM_LINESPACING;
	pfm.bLineSpacingRule  = 4;
	pfm.dyLineSpacing = cfm.yHeight + 2*20;

	if (SendMessage(hEdit, EM_SETPARAFORMAT, NULL, (LPARAM)&pfm) == 0) {
		return FALSE;
	}
	return TRUE; 
}

void LoadMenus(HWND hDlg, HWND hList, int start, int cnt, char** av)
{
	int len = 0;
	wchar_t wkey[4];
	char path2[MAX_PATH];
	wchar_t wpath[MAX_PATH];
	wchar_t wpath2[MAX_PATH];
	char title[32];
	char name[128];
	char buf[1024];
	char line[1056];
	char *d1, *d2, *d3;
	FILE* fp;
	wchar_t* ppath;
	BOOL bSpecKeyword = FALSE;

	if (cnt == start) {
		cnt = 999;
		if (_bLoadFromIni == true) {
			::SetWindowText(_hWndInput, "");
			::SetWindowText(_hWndList, "");
			DoMask(_hWndInput, _hWndList);
			::InvalidateRect(hList, NULL, TRUE);
			::UpdateWindow(hList);
			return;
		}
		_bLoadFromIni = true;
		::SetWindowText(_hWndInput, "");
	} else {
		// 2011.09.19 v0.2.3 起動時にキーワード指定
		if (start < cnt && strcmp(av[start], "-k") == 0) {
			_bLoadFromIni = true;
			memset(line, 0, sizeof(line));
			for (int idx=start+1; idx<cnt; idx++) {
				strcat(line, av[idx]);
				strcat(line, " ");
			}
			::SetWindowText(_hWndInput, line);
			len = strlen(line);
			SendMessage(_hWndInput, EM_SETSEL, len, len);
			len = 0;
			bSpecKeyword = TRUE;
		} else {
			_bLoadFromIni = false;
			::SetWindowText(_hWndInput, "");
		}
	}

	::SetWindowText(_hWndList, "");

	SendMessage(hList, WM_SETREDRAW, FALSE, 0);

	SetListTextColor(_hWndList, (COLORREF)_color_list_fg);

	for (int idx=start; idx<cnt; idx++) {
		if (_bLoadFromIni) {
			swprintf(wkey, L"%03d", idx-start);
			::GetPrivateProfileStringW(L"MenuFiles", wkey, L"", wpath, sizeof(wpath), _ini_pathw);
			if (wcscmp(wpath, L"") == 0) {
				break;
			}
			// 2011.09.19 v0.2.3 相対パス指定対応
			if (wpath[0] != L'-' && wpath[0] != L'\\' && wpath[1] != L':') {
				wsprintfW(wpath2, L"%s%s", _dir_pathw, wpath);
				ppath = wpath2;
			} else {
				ppath = wpath;
			}
		} else {
			mbstowcs(wpath, av[idx], sizeof(wpath));
			ppath = wpath;
		}
		if (wcsstr(ppath, L"-h") != NULL) {
			if (AfxInit(pAfxApp) == 0) {
				for (int idxd=0; idxd<2; idxd++) {
					wchar_t wbuf[32];
					wsprintfW(wbuf, L"%d", idxd);
					VARIANT x;
					x.vt = VT_BSTR;
					x.bstrVal = ::SysAllocString(wbuf);
					VARIANT result;
					AfxExec(DISPATCH_METHOD, &result, pAfxApp, L"HisDirCount", 1, x);
					int nHisDirCnt = result.intVal;
					for (int idxk=0; idxk<nHisDirCnt; idxk++) {
						VARIANT y;
						y.vt = VT_BSTR;
						wsprintfW(wbuf, L"%d", idxk);
						y.bstrVal = ::SysAllocString(wbuf);
						AfxExec(DISPATCH_METHOD, &result, pAfxApp, L"HisDir", 2, y, x);
						if (result.bstrVal == NULL) continue;
						wcstombs(path2, result.bstrVal, sizeof(path2));
						SendMessage(hList, EM_SETSEL, len, len);
						sprintf(buf,  "&CD %s", path2);
						sprintf(line, "\"history%d - %-64s\"\t%s\n", idxd, path2, buf);
						SendMessage(hList, EM_REPLACESEL, 0, (LPARAM)line);
						len += strlen(line) + 1;
					}
				}
				AfxCleanup(pAfxApp);
			}

		} else if (wcsstr(ppath, L"AFXW.HIS") != NULL) {
			wchar_t* dir_table[2] = { L"DIR", L"DIRR" };
			for (int idxd=0; idxd<2; idxd++) {
				wcstombs(title, dir_table[idxd], sizeof(title));
				for (int idxk=0; idxk<=35; idxk++) {
					swprintf(wkey, L"%02d", idxk);
					::GetPrivateProfileStringW(dir_table[idxd], wkey, L"", wpath2, sizeof(wpath2), ppath);
					wcstombs(path2, wpath2, sizeof(path2));
					SendMessage(hList, EM_SETSEL, len, len);
					sprintf(buf,  "&CD %s", path2);
					sprintf(line, "\"%s - %-64s\"\t%s\n", title, buf, buf);
					SendMessage(hList, EM_REPLACESEL, 0, (LPARAM)line);
					len += strlen(line) + 1;
				}
			}
		} else {
			fp = _wfopen(ppath, L"r");
			if (fp == NULL) {
				continue;
			}

			if (fgets(buf, sizeof(buf), fp) != NULL && _strnicmp(buf, "afx", 3) == 0) {
				memset(title, 0, sizeof(title));
				strncpy(title, &buf[3], strlen(&buf[3])-1);
				while (fgets(buf, sizeof(buf), fp) != NULL) {
					d1 = strchr(buf, '\"');
					if (d1 == NULL) continue;
					d2 = strchr(d1+1, '\"');
					if (d2 == NULL) continue;
					d3 = d2+1;
					while ( (*d3 == ' ') || (*d3 == '\t') ) {
						d3++;
					}

					memset(name, 0, sizeof(name));
					if (d2-d1 > sizeof(name)-1) {
						strncpy(name, d1+1, sizeof(name)-1);
					} else {
						strncpy(name, d1+1, d2-d1-1);
					}

					SendMessage(hList, EM_SETSEL, len, len);
					sprintf(line, "\"%s - %-64s\"\t%s", title, name, d3);
					SendMessage(hList, EM_REPLACESEL, 0, (LPARAM)line);
					len += strlen(line) + 1;
				}
			}
			fclose(fp);
		}
	}

	::SendMessage(hList, WM_SETREDRAW, TRUE, 0);
	::SendMessage(hList, EM_SCROLL, SB_TOP, 0);

	::InvalidateRect(hList, NULL, TRUE);
	::UpdateWindow(hList);

	if (_buf != NULL) {
		delete [] _buf;
	}
	_buf = new char[len + 1];
	_buf_len = len;

	SendMessage( hList,	WM_GETTEXT, _buf_len+1, (long)_buf);
	_buf_len = strlen(_buf);

	if (bSpecKeyword == TRUE) {
		DoMask(_hWndInput, _hWndList);
	}

	return;
}

/**
 * ファイル検索
 */
DWORD FindFile(LPCTSTR lpszPath)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA data;
	TCHAR szFind [ MAX_PATH ];

	DWORD len = 0;

	// ファイル検索
    sprintf( szFind, _T("%s\\%s"), lpszPath, "*");
	hFile = FindFirstFile(szFind, &data);
	if (hFile != INVALID_HANDLE_VALUE) {
		do {
			if( strcmp(data.cFileName, _T(".")) == 0
				|| strcmp(data.cFileName, _T("..")) == 0 ) continue;

			if( (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					== FILE_ATTRIBUTE_DIRECTORY ) {
				sprintf( szFind, _T("%s\\%s\\\n"), lpszPath, data.cFileName);
			} else {
				sprintf( szFind, _T("%s\\%s\n"), lpszPath, data.cFileName);
			}
			SendMessage(_hWndList, EM_SETSEL, len, len);
			SendMessage(_hWndList, EM_REPLACESEL, 0, (LPARAM)szFind);
			len += strlen(szFind)+2;
		} while (FindNextFile( hFile, &data));

		SendMessage(_hWndList, EM_SETSEL, 0, 0);

		FindClose( hFile );
	}

	return len;
}

void LoadFiles()
{
	TCHAR crDir[MAX_PATH + 1];
	/* xxx
	GetCurrentDirectory(MAX_PATH + 1 , crDir);
	*/
	AfxInit(pAfxApp);
	VARIANT x;
	x.vt = VT_BSTR;
	x.bstrVal = ::SysAllocString(L"$P");
	VARIANT result;
	HRESULT hr = AfxExec(DISPATCH_METHOD, &result, pAfxApp, L"Extract", 1, x);
	AfxCleanup(pAfxApp);
	if(FAILED(hr)) {
		return;
	}
	::SetWindowText(_hWndInput, "");
	::SetWindowText(_hWndList, "");

	wcstombs(crDir, result.bstrVal, sizeof(crDir));

	SendMessage(_hWndList, WM_SETREDRAW, FALSE, 0);
	DWORD len = FindFile(crDir);
	SendMessage(_hWndList, WM_SETREDRAW, TRUE, 0);
	::InvalidateRect(_hWndList, NULL, TRUE);
	::UpdateWindow(_hWndList);

	if (_buf != NULL) {
		delete [] _buf;
	}
	_buf = new char[len + 1];
	_buf_len = len;

	SendMessage( _hWndList,	WM_GETTEXT, _buf_len+1, (long)_buf);

}

void LoadKillTasks()
{
	::SetWindowText(_hWndInput, "");
	::SetWindowText(_hWndList, "");

	SendMessage(_hWndList, WM_SETREDRAW, FALSE, 0);

	DWORD len = 0;
	char szLine [1024];

	HANDLE hthSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hthSnapshot == INVALID_HANDLE_VALUE) {
		return;
	}
	
	PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
	BOOL bProcess = Process32First(hthSnapshot, &pe);
	for (; bProcess; bProcess = Process32Next(hthSnapshot, &pe)) {
		if (pe.th32ProcessID == 0) continue;
		sprintf(szLine, "\"kill %-32s [%5d]\" &SCRIPT \"%s%s\" %d\n", 
				pe.szExeFile, pe.th32ProcessID, _dir_path, _kill_prog, pe.th32ProcessID);
		SendMessage(_hWndList, EM_SETSEL, len, len);
		SendMessage(_hWndList, EM_REPLACESEL, 0, (LPARAM)szLine);
		len += strlen(szLine)+1;
	}
	SendMessage(_hWndList, EM_SETSEL, 0, 0);
	CloseHandle(hthSnapshot);

	SendMessage(_hWndList, WM_SETREDRAW, TRUE, 0);
	::SendMessage(_hWndList, EM_SCROLL, SB_TOP, 0);
	::InvalidateRect(_hWndList, NULL, TRUE);
	::UpdateWindow(_hWndList);

	if (_buf != NULL) {
		delete [] _buf;
	}
	_buf = new char[len + 1];
	_buf_len = len;

	if (_work != NULL) {
		delete [] _work;
	}
	_work = NULL;

	SendMessage( _hWndList,	WM_GETTEXT, _buf_len+1, (long)_buf);

}

void LoadEjectDrives()
{
	::SetWindowText(_hWndInput, "");
	::SetWindowText(_hWndList, "");

	SendMessage(_hWndList, WM_SETREDRAW, FALSE, 0);

	DWORD len = 0;
	char szDrive[3];
	char szLine [64];

	// ドライブのビットで取得
	DWORD dwDrive = GetLogicalDrives();
	for ( int nDrive = 0 ; nDrive < 26 ; nDrive++ ){
		if ( dwDrive & (1 << nDrive) ){
			sprintf(szDrive, "%c:", (nDrive + 'A') );
			if ( GetDriveType(szDrive) == DRIVE_REMOVABLE ){
				sprintf(szLine, "\"eject %s\" &EJECT %s\n", szDrive, szDrive);
				SendMessage(_hWndList, EM_SETSEL, len, len);
				SendMessage(_hWndList, EM_REPLACESEL, 0, (LPARAM)szLine);
				len += strlen(szLine)+1;
			}
		}
	}

	SendMessage(_hWndList, WM_SETREDRAW, TRUE, 0);
	::SendMessage(_hWndList, EM_SCROLL, SB_TOP, 0);
	::InvalidateRect(_hWndList, NULL, TRUE);
	::UpdateWindow(_hWndList);

	if (_buf != NULL) {
		delete [] _buf;
	}
	_buf = new char[len + 1];
	_buf_len = len;

	if (_work != NULL) {
		delete [] _work;
	}
	_work = NULL;

	SendMessage( _hWndList,	WM_GETTEXT, _buf_len+1, (long)_buf);

}

void tolower( char* p,  int len)
{
	if (p == NULL || len <= 0) {
		return;
	}
	for (int idx=0; idx<len; idx++) {
		if (p[idx] >= 0 && isalpha(p[idx]) && isupper((unsigned int)p[idx])) {
			p[idx] = tolower(p[idx]);
		}
	}
}


int strmatch ( const char *str, const char *ptn, MatchInfo_T& mi)
{
	char* hit = NULL;
	int len = strlen(ptn);
	char* buf = new char[len+1];
	strncpy(buf, ptn, len);
	buf[len] = '\0';
	tolower(buf, len);

	int tlen = strlen(str);
	char* target = new char[tlen+1];
	strncpy(target, str, tlen);
	target[tlen] = '\0';
	tolower(target, tlen);

	char* p = buf;
	int cnt = 1;
	do {
		p = strchr(p, ' ');
		if (p == NULL) {
			break;
		}
		*p = '\0';
		p++;
		cnt++;
	} while (p < buf+len && *(p+1) != '\0');

	mi.num = 0;
	BOOL bNotMatch = FALSE;
	p = buf;
	for (int idx=0; idx<cnt; idx++) {
		hit = strstr(target, p);
		if (hit == NULL) {
			bNotMatch = TRUE;
			break;
		}
		if (_bHilight && mi.num < MAX_HILIGHT_NUM) {
			mi.start [mi.num] = hit - target;
			mi.length[mi.num] = strlen(p);
			mi.num++;
		}
		p += strlen(p)+1;
	}

	delete [] target;
	delete [] buf;

	if (bNotMatch == TRUE) {
		return 0;
	}
	return 1;
}


void DoMask(HWND hWnd, HWND hWndList)
{
	char szMask[256];
	::GetWindowText(hWnd, szMask, sizeof(szMask));

	// 2011.09.19 v0.2.3 メニューパラメタの動的追加
	char* addpara = NULL;
	int addpara_len = 0;
	addpara = strchr(szMask, '+');
	if (addpara != NULL) {
		*addpara = '\0';
		addpara += 1;
		addpara_len = strlen(addpara);
	}

	int buflen = _buf_len;
	char* work = new char[buflen+1];
	strcpy(work, _buf);
	work[buflen] = '\0';

	// 改行 → \0
	int cnt = 0;
	char* p = work;
	do {
		char* ln = strchr(p, '\n');
		if (ln == NULL) {
			break;
		}
		*ln = '\0';
		p = ln + 1;
		cnt++;
	} while(p != NULL);

	// work → _work
	p = work;
	if (_work != NULL) {
		delete [] _work;
	}

	MatchInfo_T m;
	MatchInfo_T mtable[MAX_HILIGHT_LINE];
	int mtable_num = 0;
	int lineLen;
	_work = new char[buflen+cnt+1+addpara_len*cnt];
	memset(_work, 0, buflen+1);
	for (int idx=0; idx<cnt; idx++) {
		lineLen = strlen(p) + 1;
		if (strmatch(p, szMask, m) != NULL) {
			strcat(_work, p);
			// 2011.09.19 v0.2.3 メニューパラメタの動的追加
			if (addpara != NULL && addpara_len > 0) {
				strcat(_work, " ");
				strcat(_work, addpara);
			}
			strcat(_work, "\n");
			if (mtable_num < MAX_HILIGHT_LINE) {
				mtable[mtable_num] = m;
				mtable[mtable_num].lineLen = lineLen;
				mtable_num++;
			}
		}
		p += lineLen;
	}

	_work_len = strlen(_work);
	SendMessage(hWndList, WM_SETTEXT,    0, (LPARAM)_work);

	if (_bHilight) {
		int total = 0;
		for (int idx=0; idx<mtable_num; idx++) {
			MatchInfo_T& mi = mtable[idx];
			for (int idx2=0; idx2<mi.num && idx2<MAX_HILIGHT_NUM; idx2++) {
				::SendMessage(hWndList, EM_SETSEL, total+mi.start[idx2],  total+mi.start[idx2]+mi.length[idx2]);
				SetListTextColor(hWndList, _color_hilight[idx2]);
			}
			total += mi.lineLen;
		}
	}

	::SendMessage(hWndList, EM_SETSEL, 0, 0);
	SetListTextColor(_hWndList, (COLORREF)_color_list_fg);

	delete [] work;
}

void SaveMenu(HWND hDlg, HWND hList)
{
	FILE* fp;
	fp = fopen(_mnu_path, "w");
	if (fp == NULL) {
		return;
	}

	fprintf(fp, "afx%s\n", _title);

	int len = 0;
	char* buf = NULL;
	if (_work == NULL) {
		buf = _buf;
		len = _buf_len;
	} else {
		buf = _work;
		len = _work_len;
	}

	// 2011.09.07 v0.2.2 メニュー番号付加対応
	if (_append_index) {
		int idx = 0;
		char* end = NULL;
		char* tmp = new char[len + 1];
		memcpy(tmp, buf, len);
		tmp[len] = '\0';
		char* start = tmp;
		do {
			end = strchr(start, '\n');
			if (end != NULL) {
				*end = '\0';
			}
			if (strlen(start) != 0) {
				char symbol = ' ';
				if        (idx < 10 ) {
					symbol = '0' + idx - 0;
				} else if (idx < 10 + 26) {
					symbol = 'a' + idx - 10;
				} else if (idx < 10 + 26 + 26) {
					symbol = 'A' + idx - 10 - 26;
				}
				fprintf(fp, "\"%c %s\n", symbol, start+1);
			}

			start = end+1;
			idx++;
			if (idx >= 10 + 26 + 26) {
				idx = 0;
			}
		} while (end != NULL);
		delete [] tmp;
	} else {
		fwrite(buf, len, 1, fp);
	}
	
	fclose(fp);
}

LRESULT CALLBACK InputWindowProc(HWND hWnd, UINT message, WPARAM wp, LPARAM lp)
{
	int start, end;
	switch (message)
	{
		case WM_KEYDOWN:
			if (wp == VK_UP) {
				::SendMessage(_hWndList, EM_SCROLL, SB_LINEUP, 0);
			} else 
			if (wp == VK_DOWN) {
				::SendMessage(_hWndList, EM_SCROLL, SB_LINEDOWN, 0);
			} else 
			if (wp == VK_PRIOR) {
				::SendMessage(_hWndList, EM_SCROLL, SB_PAGEUP, 0);
			} else 
			if (wp == VK_NEXT) {
				::SendMessage(_hWndList, EM_SCROLL, SB_PAGEDOWN, 0);
			} else 
			if (wp == 'B') {
				if (GetKeyState(VK_LCONTROL) & 0x80) {
					::SendMessage(hWnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
					::SendMessage(hWnd, EM_SETSEL, start-1,  start-1);
					return 0;
				}
			} else 
			if (wp == 'F') {
				if (GetKeyState(VK_LCONTROL) & 0x80) {
					::SendMessage(hWnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
					::SendMessage(hWnd, EM_SETSEL, start+1,  start+1);
					return 0;
				}
			} else 
			if (wp == 'A') {
				if (GetKeyState(VK_LCONTROL) & 0x80) {
					wp = VK_HOME;
				}
			} else 
			if (wp == 'E') {
				if (GetKeyState(VK_LCONTROL) & 0x80) {
					wp = VK_END;
				}
			} else
			if (wp == 'N') {
				if (GetKeyState(VK_LCONTROL) & 0x80) {
					::SendMessage(_hWndList, EM_SCROLL, SB_LINEDOWN, 0);
					return 0;
				}
			} else 
			if (wp == 'P') {
				if (GetKeyState(VK_LCONTROL) & 0x80) {
					::SendMessage(_hWndList, EM_SCROLL, SB_LINEUP, 0);
					return 0;
				}
			}
			break;

		case WM_KEYUP:
			if (wp == VK_DOWN  || 
				wp == VK_UP    || 
				wp == VK_LEFT  || 
				wp == VK_RIGHT ||
				wp == VK_PRIOR ||
				wp == VK_NEXT ) {
				break;
			}
			if (GetKeyState(VK_LCONTROL) & 0x80) {
				if (wp == 'B' ||
					wp == 'F' ||
					wp == 'A' ||
					wp == 'E' ||
					wp == 'N' ||
					wp == 'P') {
					return 0;
				}
				break;
			}
			if (wp != VK_CONTROL) {
				DoMask(hWnd, _hWndList);
			}
			break;
	}

	return (CallWindowProc((WNDPROC)_hWndProcInput, hWnd, message, wp, lp));
}

LRESULT CALLBACK WindowProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	//HWND hWnd;
	//HDC hDC;
	RECT rect, afxRect;
	char command[256];
	HANDLE hShare;
	char *shmem;
	char *p;
	char**  dmyarg;
	int cnt;

	//HANDLE hEvent;
	switch (message)
	{
	case WM_INITDIALOG:
		// タイトル設定
		::SetWindowText(hDlg, _title);
		// あふの中央に表示
		::GetWindowRect(_hAfxWnd, &afxRect);
		::GetWindowRect(hDlg, &rect);
		::MoveWindow(hDlg, 
				(afxRect.left+afxRect.right)/2+(rect.left-rect.right)/2,
				(afxRect.top+afxRect.bottom)/2+(rect.top-rect.bottom)/2,
				rect.right-rect.left,
				rect.bottom-rect.top,
				FALSE);

		_hWndList  = GetDlgItem(hDlg, IDC_EDIT_LIST);
		_hWndInput = GetDlgItem(hDlg, IDC_EDIT_INPUT);

		// メニューのロード
		::SendMessage(_hWndList, EM_SETLIMITTEXT, 64*1024, 0);
		LoadMenus(hDlg, _hWndList, 1, __argc, __argv);

		// IDC_EDIT_INPUT
		_hWndProcInput = (FARPROC)GetWindowLong(_hWndInput, GWL_WNDPROC);
		SetWindowLong(_hWndInput, GWL_WNDPROC, (LONG)InputWindowProc);

		SendMessage( _hWndList, EM_SETBKGNDCOLOR, (WPARAM)0, (LPARAM)_color_list_bg);

		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			::GetWindowText(_hWndInput, command, sizeof(command));
			cnt = strlen(command);
			if (cnt == 0) {
				cnt = 1;
			}
			if (strncmp(command, "!!exit", cnt)==0) {
				EndDialog(hDlg, LOWORD(wParam));
			} else if (strncmp(command, "!!file", cnt)==0) {
				LoadFiles();
			} else if (strncmp(command, "!!reload", cnt)==0) {
				_bLoadFromIni = false;
				LoadMenus(hDlg, _hWndList, 1, __argc, __argv);
			} else if (strncmp(command, "!!kill", cnt)==0) {
				LoadKillTasks();
				_bLoadFromIni = false;
			} else if (strncmp(command, "!!eject", cnt)==0) {
				LoadEjectDrives();
				_bLoadFromIni = false;
			} else {
				SaveMenu(hDlg, _hWndList);
				if (_stay_mode) {
					::ShowWindow(hDlg, SW_HIDE);
					SendKey();
				} else {
					EndDialog(hDlg, LOWORD(wParam));
					SendKey();
				}
			}
			return TRUE;

		case IDCANCEL:
			if (_stay_mode) {
				::SetWindowText(_hWndInput, "");
				::ShowWindow(hDlg, SW_HIDE);
				DoMask(_hWndInput, _hWndList);
			} else {
				EndDialog(hDlg, LOWORD(wParam));
			}
			return TRUE;
		}
		break;

	case WM_CTLCOLOREDIT:
		if ((HWND)lParam == _hWndInput) {
			HDC hDC = (HDC)wParam;
			SetBkMode(hDC, TRANSPARENT);
			SetBkColor(hDC,_color_input_bg);
			SetTextColor(hDC, _color_input_fg);
			return (LRESULT)_brush_input;
		}
		break;

	case WM_RELOAD:
		hShare = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, FILE_MAP_READ, 0, wParam, AFXFAZZY_SHAREDMEM);
		shmem = (char*)::MapViewOfFile(hShare, FILE_MAP_READ, 0, 0, wParam);
		p = shmem;
		dmyarg = new char* [wParam];
		for (unsigned int idx=0; idx<wParam; idx++) {
			dmyarg[idx] = p;
			p += strlen(p)+1;
		}
		LoadMenus(hDlg, _hWndList, 0, wParam, dmyarg);
		delete [] dmyarg;

		::UnmapViewOfFile(shmem);
		::CloseHandle(hShare);
		break;
	
	}

	return FALSE;
}

