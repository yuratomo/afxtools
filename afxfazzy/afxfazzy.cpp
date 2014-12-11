// afxfazzy.cpp : アプリケーション用クラスの定義を行います。
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <richedit.h>
#include <Tlhelp32.h>
#include <gdiplus.h>
#include "resource.h"
#include <vector>
#include <regex>
#include "afxcom.h"
#include "Migemo.h"

#define MODIFIER_ALT         0x0004
#define MODIFIER_CONTROL     0x0002
#define MODIFIER_SHIFT       0x0001
#define AFXFAZZY_SHAREDMEM   L"__yuratomo_afxfazzy_shared_memory"
#define WM_AFXFAXXY_START    WM_USER + 0x10000
#define WM_RELOAD            WM_AFXFAXXY_START + 0x0001
#define MAX_HILIGHT_NUM      3
#define MAX_HILIGHT_LINE     20
#define RICHEDIT_DLL         L"MSFTEDIT.DLL"
#define MIGEMO_START_STR_LEN 2  // Migemo 検索を開始する文字数
#define LINESPACING_POINT    2

typedef struct _MatchInfo_T
{
	int start [MAX_HILIGHT_NUM];
	int length[MAX_HILIGHT_NUM];
	int lineLen;
	int num;
} MatchInfo_T;

enum EAction
{
	E_SaveMenu = 0,
	E_Run = 1,
};

// グローバル変数
int     pKeyCode = 89;
int     pModifier = 1;
HWND    _hAfxWnd = NULL;
HBRUSH  _brush_input = NULL;
HWND    _hWndInput = NULL;
HWND    _hWndList= NULL;
FARPROC _hWndProcInput;
WCHAR*  _buf = NULL;
int     _buf_len = 0;
WCHAR*  _work = NULL;
int     _work_len = 0;
WCHAR   _ini_path[MAX_PATH];
WCHAR   _dir_path[MAX_PATH];
WCHAR   _mnu_path[MAX_PATH];
WCHAR   _migemo_path[MAX_PATH];
WCHAR   _migemo_dict_path[MAX_PATH];
WCHAR   _kill_prog[MAX_PATH];
int     _sleep_sendkey = 0;
int     _sleep_returnkey = 0;
BOOL    _send_enter = FALSE;
BOOL    _automation_exec = FALSE;
WCHAR   _title[256];
WCHAR   _title_with_migemo[256];
DWORD   _color_list_fg  = RGB(200,200,200);
DWORD   _color_list_bg  = RGB(0,0,0);
DWORD   _color_input_fg = RGB(200,200,200);
DWORD   _color_input_bg = RGB(0,0,0);
DWORD   _color_hilight_line_bg = RGB(7, 54, 66);
DWORD   _color_hilight[MAX_HILIGHT_NUM];
BOOL    _stay_mode = TRUE;
BOOL    _default_migemo = FALSE;
BOOL    _migemo_mode = FALSE;
BOOL    _append_index = FALSE;
bool    _bLoadFromIni = false;
bool    _bHilight = true;
DWORD   _font_height = 8;
int     _page_scroll_step = 20;
bool    _bNoEnChange = false;
EAction _defaultAction = E_SaveMenu;
IDispatch* pAfxApp = NULL;
CMigemo *_pMigemo = NULL;

// プロトタイプ
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void SendKey();
void DoMask(HWND hWnd, HWND hWndList);
void FormatLine(WCHAR *title, WCHAR *name, WCHAR *command, WCHAR *line);
int strmatchRegex(const WCHAR *str, LPVOID pat, MatchInfo_T& mi);
typedef int (*STRMATCH_FUNC)(const WCHAR *str, LPVOID pat, MatchInfo_T& mi);
std::vector<std::wregex *> GetMigemoRegex(WCHAR *szMask);
void ReinitMigemoMode();
void LoadMigemo();
void UnloadMigemo();
void RemoveLastLineFeed(HWND hList);
BOOL MoveHighlightLine(int amount);
bool IsOneLine(WCHAR *lines);
void GetCommand(WCHAR *line, WCHAR *out, int len);
bool RunInternalCommand(HWND hDlg, WPARAM wParam, WCHAR *command);
void SetCurrentLine();
int CalcPageScrollStep(int fontHeight);
BOOL SetWindowTextNoNotify(HWND hWnd, WCHAR *lpString);

int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPWSTR    lpCmdLine,
                     int       nCmdShow)
{
	DWORD r, g, b;

	//setlocale(LC_ALL, L"japanese_japan.20932");
	_wsetlocale( LC_ALL, L"Japanese" );

	// iniファイル解析
	GetModuleFileName(hInstance, _ini_path, sizeof(_ini_path)/sizeof(_ini_path[0]));
	int len = wcslen(_ini_path);
	_ini_path[len-1] = L'i';
	_ini_path[len-2] = L'n';
	_ini_path[len-3] = L'i';
 	wcscpy(_mnu_path, _ini_path);
	_mnu_path[len-1] = L'u';
	_mnu_path[len-2] = L'n';
	_mnu_path[len-3] = L'm';
	WCHAR* end = wcsrchr(_ini_path, L'\\');
	if (end != NULL) {
		memset(_dir_path, 0, sizeof(_dir_path));
		wcsncpy(_dir_path, _ini_path, end-_ini_path+1);
	}

	// あふwのウィンドウ解決
	_hAfxWnd = ::FindWindow(L"TAfxWForm", NULL);
	if (_hAfxWnd == INVALID_HANDLE_VALUE) {
		_hAfxWnd = ::FindWindow(L"TAfxForm", NULL);
		if (_hAfxWnd == INVALID_HANDLE_VALUE) {
			return -1;
		}
	}

	WCHAR temp[8];
	::GetPrivateProfileString(L"Config", L"title", L"ふぁじ～めにゅ～", _title, sizeof(_title)/sizeof(_title[0]), _ini_path);
	::GetPrivateProfileString(L"Config", L"title_migemo", L"ふぁじ～めにゅ～(migemo)", _title_with_migemo, sizeof(_title_with_migemo)/sizeof(_title_with_migemo[0]), _ini_path);
	::GetPrivateProfileString(L"Config", L"output_path", _mnu_path, _mnu_path, sizeof(_mnu_path)/sizeof(_mnu_path[0]), _ini_path);
	::GetPrivateProfileString(L"Config", L"kill_program", L"afxfazzykill.vbs", _kill_prog, sizeof(_kill_prog)/sizeof(_kill_prog[0]), _ini_path);
	::GetPrivateProfileString(L"Config", L"migemo_path", L"", _migemo_path, sizeof(_migemo_path)/sizeof(_migemo_path[0]), _ini_path);
	::GetPrivateProfileString(L"Config", L"migemo_dict_path", L"", _migemo_dict_path, sizeof(_migemo_dict_path)/sizeof(_migemo_dict_path[0]), _ini_path);

	// すでにafxfazzyがある場合は、手前に表示する。非表示なら表示する。
	HANDLE hMutex = ::CreateMutex(NULL, TRUE, L"__yuratomo_afxfazzy__");
	if (::GetLastError() == ERROR_ALREADY_EXISTS) {
		HWND hWndFazzy = ::FindWindow(NULL, _title);
		if (hWndFazzy == NULL) {
			hWndFazzy = ::FindWindow(NULL, _title_with_migemo);
		}
		if (hWndFazzy != NULL) {
			if (::IsWindowVisible(hWndFazzy) == FALSE) {
				::ShowWindow(hWndFazzy, SW_SHOW);
			}
		}
		// 既存のafxfazzyにリロード要求をする
		int length = __argc+1;
		for (int idx=0; idx<__argc; idx++) {
			length += wcslen(__wargv[idx]);
		}
		HANDLE hShare = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, length, AFXFAZZY_SHAREDMEM);
		WCHAR *shmem = (WCHAR*)::MapViewOfFile(hShare, FILE_MAP_WRITE, 0, 0, length);
		WCHAR *p = shmem;
		for (int idx=1; idx<__argc; idx++) {
			wcscpy(p, __wargv[idx]);
			p += wcslen(__wargv[idx])+1;
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

	::GetPrivateProfileString(L"SendKey", L"keycode", L"1089", temp, sizeof(temp)/sizeof(temp[0]), _ini_path);
	swscanf(temp, L"%1d%3d", &pModifier, &pKeyCode);
	_sleep_sendkey   = ::GetPrivateProfileInt(L"SendKey", L"sendkey_sleep", 200, _ini_path);
	_send_enter      = ::GetPrivateProfileInt(L"SendKey", L"send_returnkey", 0, _ini_path);
	_sleep_returnkey = ::GetPrivateProfileInt(L"SendKey", L"send_returnkey_sleep", 200, _ini_path);
	_automation_exec = ::GetPrivateProfileInt(L"SendKey", L"automation_exec", 0, _ini_path);
	_stay_mode       = ::GetPrivateProfileInt(L"Config",  L"stay", 1, _ini_path);
	_default_migemo  = ::GetPrivateProfileInt(L"Config",  L"migemode", 0, _ini_path);
	_migemo_mode     = _default_migemo;
	_bHilight        = ::GetPrivateProfileInt(L"Color",   L"hilight_enable", 1, _ini_path);
	_font_height     = ::GetPrivateProfileInt(L"Font",    L"height", 8, _ini_path);
	_append_index    = ::GetPrivateProfileInt(L"Config",  L"menu_index", 0, _ini_path);
	_defaultAction   = (EAction)::GetPrivateProfileInt(L"Config",  L"default_action", E_SaveMenu, _ini_path);

	// カラー指定読み込み
	::GetPrivateProfileString(L"Color", L"list_fg", L"", temp, sizeof(temp)/sizeof(temp[0]), _ini_path);
	if (wcslen(temp) == 6) {
		swscanf(temp, L"%02x%02x%02x", &r, &g, &b);
		_color_list_fg = RGB(r,g,b);
	}
	::GetPrivateProfileString(L"Color", L"list_bg", L"", temp, sizeof(temp)/sizeof(temp[0]), _ini_path);
	if (wcslen(temp) == 6) {
		swscanf(temp, L"%02x%02x%02x", &r, &g, &b);
		_color_list_bg = RGB(r,g,b);
	}
	::GetPrivateProfileString(L"Color", L"input_fg", L"", temp, sizeof(temp)/sizeof(temp[0]), _ini_path);
	if (wcslen(temp) == 6) {
		swscanf(temp, L"%02x%02x%02x", &r, &g, &b);
		_color_input_fg = RGB(r,g,b);
	}
	::GetPrivateProfileString(L"Color", L"input_bg", L"", temp, sizeof(temp)/sizeof(temp[0]), _ini_path);
	if (wcslen(temp) == 6) {
		swscanf(temp, L"%02x%02x%02x", &r, &g, &b);
		_color_input_bg = RGB(r,g,b);
	}
	::GetPrivateProfileString(L"Color", L"hilight_line_bg", L"", temp, sizeof(temp)/sizeof(temp[0]), _ini_path);
	if (wcslen(temp) == 6) {
		swscanf(temp, L"%02x%02x%02x", &r, &g, &b);
		_color_hilight_line_bg = RGB(r,g,b);
	}

	if (_bHilight == true) {
		_color_hilight[0] = RGB(255, 0, 0);
		_color_hilight[1] = RGB(0, 255, 0);
		_color_hilight[2] = RGB(255, 255, 0);
		WCHAR key[32];
		for (int idx=0; idx<MAX_HILIGHT_NUM; idx++) {
			swprintf(key, L"hilight%d", idx);
			::GetPrivateProfileString(L"Color", key, L"", temp, sizeof(temp)/sizeof(temp[0]), _ini_path);
			if (wcslen(temp) == 6) {
				swscanf(temp, L"%02x%02x%02x", &r, &g, &b);
				_color_hilight[idx] = RGB(r,g,b);
			}
		}
	}
	
	// 背景色用ブラシ作成
	_brush_input = ::CreateSolidBrush(_color_input_bg);

	HINSTANCE hRich = LoadLibrary(RICHEDIT_DLL);

	// スレッドから通知される状態を表示するプログレス画面
	int ret = DialogBox(hInstance, (LPCWSTR)IDD_AFXFAZZY_DIALOG, NULL, (DLGPROC)WindowProc);
	if (ret == -1) {
		WCHAR msg[256];
		swprintf(msg, L"Create dialog error.(error code=%d)", GetLastError());
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
	WCHAR* ln = NULL;
	if (_send_enter == TRUE && (_work != NULL || _buf != NULL)) {
		if (_work != NULL) {
			ln = wcschr(_work, L'\n');
		} else {
			ln = wcschr(_buf, L'\n');
		}
		if ( (ln != NULL) && (wcschr(ln+1, L'\n') == NULL)) {
			if (_automation_exec) {
				WCHAR *d1, *d2, *d3;
				if (_work != NULL) {
					d1 = wcschr(_work, L'\"');
				} else {
					d1 = wcschr(_buf, L'\"');
				}
				if (d1 != NULL) {
					d2 = wcschr(d1+1, L'\"');
					if (d2 != NULL) {
						d3 = d2+1;
						while ( (*d3 == L' ') || (*d3 == L'\t') ) {
							d3++;
						}

						WCHAR* cr = wcsstr(d3, L"\n");
						if (cr != NULL) *cr = L'\0';

						if (AfxInit(pAfxApp) == 0) {
							VARIANT x;
							x.vt = VT_BSTR;
							x.bstrVal = ::SysAllocString(d3);
							AfxExec(DISPATCH_METHOD, NULL, pAfxApp, L"Exec", 1, x);
							AfxExec(DISPATCH_METHOD, NULL, pAfxApp, L"MesPrint", 1, x);
							AfxCleanup(pAfxApp);
						}
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
		WCHAR command[MAX_PATH+10];
		swprintf(command, L"&MENU %s", _mnu_path);
		int len = wcslen(command)+1;
		wWCHAR_t* wcommand = new wWCHAR_t[len];
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

BOOL SetListTextDefaultColor(HWND hEdit, COLORREF color)
{
	CHARFORMAT2 cfm;
	memset(&cfm, 0, sizeof(CHARFORMAT2));
	cfm.cbSize = sizeof(CHARFORMAT2);
	if (SendMessage(hEdit, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cfm) == 0) {
		return FALSE;
	}

	cfm.dwMask |= CFM_COLOR | CFM_FACE | CFM_SIZE | CFM_OFFSET;
	cfm.dwEffects &= ~CFE_AUTOCOLOR;
	cfm.yHeight = _font_height*20;
	cfm.yOffset = 0;
	cfm.crTextColor = color;

	wcscpy(cfm.szFaceName, L"ＭＳ ゴシック");
	if (SendMessage(hEdit, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cfm) == 0) {
		return FALSE;
	}
	
	PARAFORMAT2 pfm;
	memset(&pfm, 0, sizeof(PARAFORMAT2));
	pfm.cbSize = sizeof(PARAFORMAT2);
	pfm.dwMask = PFM_LINESPACING;
	pfm.bLineSpacingRule  = 4;
	pfm.dyLineSpacing = cfm.yHeight + LINESPACING_POINT*20;

	if (SendMessage(hEdit, EM_SETPARAFORMAT, NULL, (LPARAM)&pfm) == 0) {
		return FALSE;
	}
	return TRUE; 
}

BOOL SetListTextColor(HWND hEdit, COLORREF color)
{
	CHARFORMAT cfm;
	memset(&cfm, 0, sizeof(CHARFORMAT));
	cfm.cbSize = sizeof(CHARFORMAT);
	cfm.dwMask = CFM_COLOR;
	cfm.crTextColor = color;

	if (SendMessage(hEdit, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD, (LPARAM)&cfm) == 0) {
		return FALSE;
	}
	
	return TRUE; 
}

void LoadMenus(HWND hDlg, HWND hList, int start, int cnt, WCHAR** av)
{
	int len = 0;
	WCHAR wkey[4];
	WCHAR path2[MAX_PATH];
	WCHAR wpath[MAX_PATH];
	WCHAR wpath2[MAX_PATH];
	WCHAR title[32];
	WCHAR name[128];
	WCHAR buf[1024];
	WCHAR line[1056];
	WCHAR *d1, *d2, *d3;
	FILE* fp;
	WCHAR* ppath;
	BOOL bSpecKeyword = FALSE;

	if (cnt == start) {
		cnt = 999;
		if (_bLoadFromIni == true) {
			SetWindowTextNoNotify(_hWndInput, L"");
			::SetWindowText(_hWndList, L"");
			DoMask(_hWndInput, _hWndList);
			::InvalidateRect(hList, NULL, TRUE);
			::UpdateWindow(hList);
			return;
		}
		_bLoadFromIni = true;
		SetWindowTextNoNotify(_hWndInput, L"");
	} else {
		// 2011.09.19 v0.2.3 起動時にキーワード指定
		if (start < cnt && wcscmp(av[start], L"-k") == 0) {
			_bLoadFromIni = true;
			memset(line, 0, sizeof(line));
			for (int idx=start+1; idx<cnt; idx++) {
				wcscat(line, av[idx]);
				wcscat(line, L" ");
			}
			SetWindowTextNoNotify(_hWndInput, line);
			len = wcslen(line);
			SendMessage(_hWndInput, EM_SETSEL, len, len);
			len = 0;
			bSpecKeyword = TRUE;
		} else {
			_bLoadFromIni = false;
			SetWindowTextNoNotify(_hWndInput, L"");
		}
	}

	::SetWindowText(_hWndList, L"");

	SendMessage(hList, WM_SETREDRAW, FALSE, 0);

	SetListTextDefaultColor(_hWndList, (COLORREF)_color_list_fg);

	for (int idx=start; idx<cnt; idx++) {
		if (_bLoadFromIni) {
			swprintf(wkey, L"%03d", idx-start);
			::GetPrivateProfileString(L"MenuFiles", wkey, L"", wpath, sizeof(wpath)/sizeof(wpath[0]), _ini_path);
			if (wcscmp(wpath, L"") == 0) {
				break;
			}
			// 2011.09.19 v0.2.3 相対パス指定対応
			if (wpath[0] != L'-' && wpath[0] != L'\\' && wpath[1] != L':') {
				swprintf(wpath2, L"%s%s", _dir_path, wpath);
				ppath = wpath2;
			} else {
				ppath = wpath;
			}
		} else {
			wcsncpy(wpath, av[idx], sizeof(wpath)/sizeof(wpath[0]));
			ppath = wpath;
		}
		if (wcsstr(ppath, L"-h") != NULL) {
			if (AfxInit(pAfxApp) == 0) {
				for (int idxd=0; idxd<2; idxd++) {
					WCHAR wbuf[32];
					swprintf(wbuf, L"%d", idxd);
					VARIANT x;
					x.vt = VT_BSTR;
					x.bstrVal = ::SysAllocString(wbuf);
					VARIANT result;
					AfxExec(DISPATCH_METHOD, &result, pAfxApp, L"HisDirCount", 1, x);
					int nHisDirCnt = result.intVal;
					swprintf(title, L"history%d", idxd);
					for (int idxk=0; idxk<nHisDirCnt; idxk++) {
						VARIANT y;
						y.vt = VT_BSTR;
						swprintf(wbuf, L"%d", idxk);
						y.bstrVal = ::SysAllocString(wbuf);
						AfxExec(DISPATCH_METHOD, &result, pAfxApp, L"HisDir", 2, y, x);
						if (result.bstrVal == NULL) continue;
						wcsncpy(path2, result.bstrVal, sizeof(path2)/sizeof(path2[0]));
						SendMessage(hList, EM_SETSEL, len, len);
						swprintf(buf,  L"&CD %s", path2);
						FormatLine(title, path2, buf, line);
						SendMessage(hList, EM_REPLACESEL, 0, (LPARAM)line);
						len += wcslen(line);
					}
				}
				AfxCleanup(pAfxApp);
			}

		} else if (wcsstr(ppath, L"AFXW.HIS") != NULL) {
			WCHAR* dir_table[2] = { L"DIR", L"DIRR" };
			for (int idxd=0; idxd<2; idxd++) {
				wcsncpy(title, dir_table[idxd], sizeof(title)/sizeof(title[0]));
				for (int idxk=0; idxk<=35; idxk++) {
					swprintf(wkey, L"%02d", idxk);
					::GetPrivateProfileStringW(dir_table[idxd], wkey, L"", wpath2, sizeof(wpath2)/sizeof(wpath2[0]), ppath);
					wcsncpy(path2, wpath2, sizeof(path2)/sizeof(path2[0]));
					SendMessage(hList, EM_SETSEL, len, len);
					swprintf(buf,  L"&CD %s", path2);
					FormatLine(title, buf, buf, line);
					SendMessage(hList, EM_REPLACESEL, 0, (LPARAM)line);
					len += wcslen(line);
				}
			}
		} else {
			fp = _wfopen(ppath, L"r");
			if (fp == NULL) {
				continue;
			}

			if (fgetws(buf, sizeof(buf)/sizeof(buf[0]), fp) != NULL && wcsnicmp(buf, L"afx", 3) == 0) {
				memset(title, 0, sizeof(title));
				wcsncpy(title, &buf[3], wcslen(&buf[3])-1);
				while (fgetws(buf, sizeof(buf)/sizeof(buf[0]), fp) != NULL) {
					d1 = wcschr(buf, L'\"');
					if (d1 == NULL) continue;
					d2 = wcschr(d1+1, L'\"');
					if (d2 == NULL) continue;
					d3 = d2+1;
					while ( (*d3 == L' ') || (*d3 == L'\t') ) {
						d3++;
					}

					memset(name, 0, sizeof(name));
					if (d2-d1 > sizeof(name)/sizeof(name[0])-1) {
						wcsncpy(name, d1+1, sizeof(name)/sizeof(name[0])-1);
					} else {
						wcsncpy(name, d1+1, d2-d1-1);
					}

					SendMessage(hList, EM_SETSEL, len, len);
					d3[wcslen(d3)-1] = L'\0';
					FormatLine(title, name, d3, line);
					SendMessage(hList, EM_REPLACESEL, 0, (LPARAM)line);
					len += wcslen(line);
				}
			}
			fclose(fp);
		}
	}

	::SendMessage(hList, WM_SETREDRAW, TRUE, 0);
	::SendMessage(hList, EM_SETSEL, 0, 0);
	MoveHighlightLine(0);

	::InvalidateRect(hList, NULL, TRUE);
	::UpdateWindow(hList);

	if (_buf != NULL) {
		delete [] _buf;
	}
	_buf = new WCHAR[len + 1];
	_buf_len = len;

	SendMessage( hList,	WM_GETTEXT, _buf_len+1, (long)_buf);
	RemoveLastLineFeed(hList);
	::SendMessage(hList, EM_SETSEL, 0, 0);
	_buf_len = wcslen(_buf);

	if (bSpecKeyword == TRUE) {
		DoMask(_hWndInput, _hWndList);
	}

	return;
}

/**
 * ファイル検索
 */
DWORD FindFile(LPCWSTR lpszPath)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA data;
	WCHAR szFind [ MAX_PATH ];

	DWORD len = 0;

	// ファイル検索
    swprintf( szFind, L"%s\\%s", lpszPath, L"*");
	hFile = FindFirstFile(szFind, &data);
	if (hFile != INVALID_HANDLE_VALUE) {
		do {
			if( wcscmp(data.cFileName, L".") == 0
				|| wcscmp(data.cFileName, L"..") == 0 ) continue;

			if( (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					== FILE_ATTRIBUTE_DIRECTORY ) {
				swprintf( szFind, L"%s\\%s\\\n", lpszPath, data.cFileName);
			} else {
				swprintf( szFind, L"%s\\%s\n", lpszPath, data.cFileName);
			}
			SendMessage(_hWndList, EM_SETSEL, len, len);
			SendMessage(_hWndList, EM_REPLACESEL, 0, (LPARAM)szFind);
			len += wcslen(szFind)+2;
		} while (FindNextFile( hFile, &data));

		SendMessage(_hWndList, EM_SETSEL, 0, 0);

		FindClose( hFile );
	}

	return len;
}

void LoadFiles()
{
	WCHAR crDir[MAX_PATH + 1];
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
	SetWindowTextNoNotify(_hWndInput, L"");
	::SetWindowText(_hWndList, L"");

	wcsncpy(crDir, result.bstrVal, sizeof(crDir)/sizeof(crDir[0]));

	SendMessage(_hWndList, WM_SETREDRAW, FALSE, 0);
	DWORD len = FindFile(crDir);
	SendMessage(_hWndList, WM_SETREDRAW, TRUE, 0);
	::InvalidateRect(_hWndList, NULL, TRUE);
	::UpdateWindow(_hWndList);

	if (_buf != NULL) {
		delete [] _buf;
	}
	_buf = new WCHAR[len + 1];
	_buf_len = len;

	SendMessage( _hWndList,	WM_GETTEXT, _buf_len+1, (long)_buf);

}

void LoadKillTasks()
{
	SetWindowTextNoNotify(_hWndInput, L"");
	::SetWindowText(_hWndList, L"");

	SendMessage(_hWndList, WM_SETREDRAW, FALSE, 0);

	DWORD len = 0;
	WCHAR szLine [1024];

	HANDLE hthSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hthSnapshot == INVALID_HANDLE_VALUE) {
		return;
	}
	
	PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
	BOOL bProcess = Process32First(hthSnapshot, &pe);
	for (; bProcess; bProcess = Process32Next(hthSnapshot, &pe)) {
		if (pe.th32ProcessID == 0) continue;
		swprintf(szLine, L"\"kill %-32s [%5d]\" cscript \"%s%s\" %d\n", 
				pe.szExeFile, pe.th32ProcessID, _dir_path, _kill_prog, pe.th32ProcessID);
		SendMessage(_hWndList, EM_SETSEL, len, len);
		SendMessage(_hWndList, EM_REPLACESEL, 0, (LPARAM)szLine);
		len += wcslen(szLine);
	}
	SendMessage(_hWndList, EM_SETSEL, 0, 0);
	CloseHandle(hthSnapshot);

	RemoveLastLineFeed(_hWndList);

	SendMessage(_hWndList, WM_SETREDRAW, TRUE, 0);
	::SendMessage(_hWndList, EM_SETSEL, 0, 0);
	MoveHighlightLine(0);
	::InvalidateRect(_hWndList, NULL, TRUE);
	::UpdateWindow(_hWndList);

	if (_buf != NULL) {
		delete [] _buf;
	}
	_buf = new WCHAR[len + 1];
	_buf_len = len;

	if (_work != NULL) {
		delete [] _work;
	}
	_work = NULL;

	SendMessage( _hWndList,	WM_GETTEXT, _buf_len+1, (long)_buf);

}

void LoadEjectDrives()
{
	SetWindowTextNoNotify(_hWndInput, L"");
	::SetWindowText(_hWndList, L"");

	SendMessage(_hWndList, WM_SETREDRAW, FALSE, 0);

	DWORD len = 0;
	WCHAR szDrive[3];
	WCHAR szLine [64];

	// ドライブのビットで取得
	DWORD dwDrive = GetLogicalDrives();
	for ( int nDrive = 0 ; nDrive < 26 ; nDrive++ ){
		if ( dwDrive & (1 << nDrive) ){
			swprintf(szDrive, L"%c:", (nDrive + L'A') );
			if ( GetDriveType(szDrive) == DRIVE_REMOVABLE ){
				swprintf(szLine, L"\"eject %s\" &EJECT %s\n", szDrive, szDrive);
				SendMessage(_hWndList, EM_SETSEL, len, len);
				SendMessage(_hWndList, EM_REPLACESEL, 0, (LPARAM)szLine);
				len += wcslen(szLine);
			}
		}
	}

	RemoveLastLineFeed(_hWndList);

	SendMessage(_hWndList, WM_SETREDRAW, TRUE, 0);
	::SendMessage(_hWndList, EM_SETSEL, 0, 0);
	MoveHighlightLine(0);
	::InvalidateRect(_hWndList, NULL, TRUE);
	::UpdateWindow(_hWndList);

	if (_buf != NULL) {
		delete [] _buf;
	}
	_buf = new WCHAR[len + 1];
	_buf_len = len;

	if (_work != NULL) {
		delete [] _work;
	}
	_work = NULL;

	SendMessage( _hWndList,	WM_GETTEXT, _buf_len+1, (long)_buf);

}

void tolower( WCHAR* p,  int len)
{
	if (p == NULL || len <= 0) {
		return;
	}
	for (int idx=0; idx<len; idx++) {
		if (p[idx] >= 0 && iswalpha(p[idx]) && iswupper((unsigned int)p[idx])) {
			p[idx] = towlower(p[idx]);
		}
	}
}


int strmatch ( const WCHAR *str, LPVOID pat, MatchInfo_T& mi)
{
	WCHAR *ptn = (WCHAR *)pat;
	WCHAR* hit = NULL;
	int len = wcslen(ptn);
	WCHAR* buf = new WCHAR[len+1];
	wcsncpy(buf, ptn, len);
	buf[len] = L'\0';
	tolower(buf, len);

	int tlen = wcslen(str);
	WCHAR* target = new WCHAR[tlen+1];
	wcsncpy(target, str, tlen);
	target[tlen] = L'\0';
	tolower(target, tlen);

	WCHAR* p = buf;
	int cnt = 1;
	do {
		p = wcschr(p, L' ');
		if (p == NULL) {
			break;
		}
		*p = L'\0';
		p++;
		cnt++;
	} while (p < buf+len && *(p+1) != L'\0');

	mi.num = 0;
	BOOL bNotMatch = FALSE;
	p = buf;
	for (int idx=0; idx<cnt; idx++) {
		hit = wcsstr(target, p);
		if (hit == NULL) {
			bNotMatch = TRUE;
			break;
		}
		if (_bHilight && mi.num < MAX_HILIGHT_NUM) {
			mi.start [mi.num] = hit - target;
			mi.length[mi.num] = wcslen(p);
			mi.num++;
		}
		p += wcslen(p)+1;
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
	WCHAR szMask[256];
	::GetWindowText(hWnd, szMask, sizeof(szMask)/sizeof(szMask[0]));

	// 2011.09.19 v0.2.3 メニューパラメタの動的追加
	WCHAR* addpara = NULL;
	int addpara_len = 0;
	addpara = wcschr(szMask, L'+');
	if (addpara != NULL) {
		*addpara = L'\0';
		addpara += 1;
		addpara_len = wcslen(addpara);
	}

	int buflen = _buf_len;
	WCHAR* work = new WCHAR[buflen+1];
	wcscpy(work, _buf);
	work[buflen] = L'\0';

	// 改行 → \0
	int cnt = 0;
	WCHAR* p = work;
	do {
		WCHAR* ln = wcschr(p, L'\n');
		if (ln == NULL) {
			break;
		}
		*ln = L'\0';
		if (*(ln-1) == L'\r') {
			*(ln-1) = L'\0';
		}
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
	_work = new WCHAR[buflen+cnt+1+addpara_len*cnt];
	memset(_work, 0, (buflen+cnt+1+addpara_len*cnt)*sizeof(WCHAR));

	std::vector<std::wregex *> regex;
	STRMATCH_FUNC matchFunc = NULL;
	LPVOID ptn = NULL;

	if (_migemo_mode) {
		regex = GetMigemoRegex(szMask);
		matchFunc = strmatchRegex;
		ptn = &regex;
	} else {
		matchFunc = strmatch;
		ptn = szMask;
	}

	for (int idx=0; idx<cnt; idx++) {
		lineLen = wcslen(p) + 1;
		if (matchFunc(p, ptn, m) != NULL) {
			wcscat(_work, p);
			// 2011.09.19 v0.2.3 メニューパラメタの動的追加
			if (addpara != NULL && addpara_len > 0) {
				wcscat(_work, L" ");
				wcscat(_work, addpara);
			}
			wcscat(_work, L"\n");
			if (mtable_num < MAX_HILIGHT_LINE) {
				mtable[mtable_num] = m;
				if (addpara_len > 0) {
					mtable[mtable_num].lineLen = lineLen + addpara_len + 1;
				} else {
					mtable[mtable_num].lineLen = lineLen;
				}
				mtable_num++;
			}
		}
		p += lineLen;
		if (*p == L'\0') {
			p++;
		}
	}

	_work_len = wcslen(_work);
	SendMessage(hWndList, WM_SETTEXT,    0, (LPARAM)_work);

	for (std::vector<std::wregex *>::iterator it = regex.begin(); it != regex.end(); it++) {
		delete *it;
	}
	_work_len = wcslen(_work);
	SendMessage(hWndList, WM_SETTEXT,    0, (LPARAM)_work);
	RemoveLastLineFeed(hWndList);
	_work_len = wcslen(_work);

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
	MoveHighlightLine(0);

	delete [] work;
}

void SaveMenu(HWND hDlg, HWND hList)
{
	FILE* fp;
	fp = _wfopen(_mnu_path, L"w");
	if (fp == NULL) {
		return;
	}

	fwprintf(fp, L"afx%s\n", _title);

	int len = 0;
	WCHAR* buf = NULL;
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
		WCHAR* end = NULL;
		WCHAR* tmp = new WCHAR[len + 1];
		memcpy(tmp, buf, len);
		tmp[len] = L'\0';
		WCHAR* start = tmp;
		do {
			end = wcschr(start, L'\n');
			if (end != NULL) {
				*end = L'\0';
			}
			if (wcslen(start) != 0) {
				WCHAR symbol = L' ';
				if        (idx < 10 ) {
					symbol = L'0' + idx - 0;
				} else if (idx < 10 + 26) {
					symbol = L'a' + idx - 10;
				} else if (idx < 10 + 26 + 26) {
					symbol = L'A' + idx - 10 - 26;
				}
				fwprintf(fp, L"\"%c %s\n", symbol, start+1);
			}

			start = end+1;
			idx++;
			if (idx >= 10 + 26 + 26) {
				idx = 0;
			}
		} while (end != NULL);
		delete [] tmp;
	} else {
		fwprintf(fp, L"%s", buf);
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
				return MoveHighlightLine(-1);
			} else 
			if (wp == VK_DOWN) {
				return MoveHighlightLine(1);
			} else 
			if (wp == VK_PRIOR) {
				return MoveHighlightLine(-_page_scroll_step);
			} else 
			if (wp == VK_NEXT) {
				return MoveHighlightLine(_page_scroll_step);
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
					return MoveHighlightLine(1);
				}
			} else 
			if (wp == 'P') {
				if (GetKeyState(VK_LCONTROL) & 0x80) {
					return MoveHighlightLine(-1);
				}
			}
			if (wp == 'M') {
				if (GetKeyState(VK_CONTROL) & 0x80) {
					if (!_migemo_mode){
						LoadMigemo();
						DoMask(_hWndInput, _hWndList);
					}
					return TRUE;
				}
			}
			if (wp == 'S') {
				if (GetKeyState(VK_CONTROL) & 0x80) {
					if (_migemo_mode){
						UnloadMigemo();
						DoMask(_hWndInput, _hWndList);
					}
					return TRUE;
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
			break;
	}

	return (CallWindowProc((WNDPROC)_hWndProcInput, hWnd, message, wp, lp));
}

LRESULT CALLBACK WindowProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	//HWND hWnd;
	//HDC hDC;
	RECT rect, afxRect;
	WCHAR command[256];
	HANDLE hShare;
	WCHAR *shmem;
	WCHAR *p;
	WCHAR**  dmyarg;

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

		_page_scroll_step = CalcPageScrollStep(_font_height);

		// メニューのロード
		::SendMessage(_hWndList, EM_SETLIMITTEXT, 64*1024, 0);
		LoadMenus(hDlg, _hWndList, 1, __argc, __wargv);

		// IDC_EDIT_INPUT
		_hWndProcInput = (FARPROC)GetWindowLong(_hWndInput, GWL_WNDPROC);
		SetWindowLong(_hWndInput, GWL_WNDPROC, (LONG)InputWindowProc);

		SendMessage( _hWndList, EM_SETBKGNDCOLOR, (WPARAM)0, (LPARAM)_color_list_bg);
		::SendMessage(_hWndList, EM_SETSEL, 0, 0);
		MoveHighlightLine(0);
		if (_default_migemo) {
			LoadMigemo();
		}

		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			::GetWindowText(_hWndInput, command, sizeof(command)/sizeof(command[0]));
			if (!RunInternalCommand(hDlg, wParam, command)) {
				BOOL ctrlDown = (GetKeyState(VK_CONTROL) & 0x80);
				if ((_defaultAction == E_SaveMenu  && !ctrlDown)
					|| (_defaultAction != E_SaveMenu && ctrlDown)) {
					SaveMenu(hDlg, _hWndList);
				} else {
					SetCurrentLine();
				}
				if (IsOneLine(_work)) {
					WCHAR currentCommand[256];
					GetCommand(_work, currentCommand, sizeof(currentCommand)/sizeof(currentCommand[0]));
					if (RunInternalCommand(hDlg, wParam, currentCommand)) {
						return TRUE;
					}
				}
				if (_stay_mode) {
					::ShowWindow(hDlg, SW_HIDE);
					SendKey();
					ReinitMigemoMode();
				} else {
					EndDialog(hDlg, LOWORD(wParam));
					SendKey();
				}
			}
			return TRUE;

		case IDCANCEL:
			if (_stay_mode) {
				SetWindowTextNoNotify(_hWndInput, L"");
				::SendMessage(hDlg, WM_SETREDRAW, FALSE, 0);
				DoMask(_hWndInput, _hWndList);
				::SendMessage(hDlg, WM_SETREDRAW, TRUE, 0);
				::ShowWindow(hDlg, SW_HIDE);
				ReinitMigemoMode();
			} else {
				EndDialog(hDlg, LOWORD(wParam));
			}
			return TRUE;

		case IDC_EDIT_INPUT:
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				if (!_bNoEnChange) {
					DoMask(_hWndInput, _hWndList);
				}
				break;
			}
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
		shmem = (WCHAR*)::MapViewOfFile(hShare, FILE_MAP_READ, 0, 0, wParam);
		p = shmem;
		dmyarg = new WCHAR* [wParam];
		for (unsigned int idx=0; idx<wParam; idx++) {
			dmyarg[idx] = p;
			p += wcslen(p)+1;
		}
		LoadMenus(hDlg, _hWndList, 0, wParam, dmyarg);
		delete [] dmyarg;

		::UnmapViewOfFile(shmem);
		::CloseHandle(hShare);
		break;
	
	case WM_DESTROY:
		if (_pMigemo != NULL) {
			UnloadMigemo();
		}
		break;
	}

	return FALSE;
}

void FormatLine(WCHAR *title, WCHAR *name, WCHAR *command, WCHAR *line)
{
	char out_name[65];
	memset(out_name, ' ', 65);
	int name_len = wcstombs(out_name, name, sizeof(out_name)-1);
	if (name_len < 64) {
		out_name[name_len] = ' ';
		out_name[sizeof(out_name) - 1] = '\0';

		swprintf(line, L"\"%s - %S\"\t%s\n", title, out_name, command);
	} else {
		swprintf(line, L"\"%s - %-64s\"\t%s\n", title, name, command);
	}
}

int strmatchRegex(const WCHAR *str, LPVOID pat, MatchInfo_T& mi)
{
	std::vector<std::wregex *> *ptn = (std::vector<std::wregex *> *)pat;
	int len = wcslen(str);
	WCHAR* target = new WCHAR[len+1];
	wcsncpy(target, str, len);
	target[len] = L'\0';
	tolower(target, len);
	int target_len = wcslen(target);

	mi.num = 0;

	BOOL bNotMatch = FALSE;

	for (std::vector<std::wregex *>::iterator it = ptn->begin(); it != ptn->end(); it++) {
		std::wregex *regex = *it;

		std::wcregex_token_iterator first(target, target + target_len, *regex);
		std::wcregex_token_iterator last;

		bool in = false;
		while (first != last) {
			if (_bHilight && mi.num < MAX_HILIGHT_NUM) {
				mi.start [mi.num] = std::distance(target, (WCHAR *)first->first);
				mi.length[mi.num] = first->length();
				mi.num++;
			}
			first++;
			in = true;
		}
		if (!in) {
			bNotMatch = TRUE;
			break;
		}
	}
	delete [] target;

	if (bNotMatch == TRUE) {
		return 0;
	}
	return 1;
}

std::vector<std::wregex *> GetMigemoRegex(WCHAR *szMask)
{
	int lineLen = 0;
	int len = wcslen(szMask);
	WCHAR* buf = new WCHAR[len+1];
	wcsncpy(buf, szMask, len);
	buf[len] = L'\0';
	tolower(buf, len);

	WCHAR* p = buf;
	int cnt = 1;
	do {
		p = wcschr(p, L' ');
		if (p == NULL) {
			break;
		}
		*p = L'\0';
		p++;
		cnt++;
	} while (p < buf+len && *(p+1) != L'\0');

	p = buf;
	WCHAR *regex = NULL;

	std::vector<std::wregex *> ptn;

	for (int idx=0; idx<cnt; idx++) {
		if (wcslen(p) < MIGEMO_START_STR_LEN) {
			p += wcslen(p)+1;
			continue;
		} else {
			_pMigemo->Query(p, &regex);
		}
		ptn.push_back(new std::wregex(regex));
		delete regex;
		p += wcslen(p)+1;
	}
	delete [] buf;

	return ptn;
}

void ReinitMigemoMode()
{
	if (!_default_migemo) {
		UnloadMigemo();
	} else if (!_migemo_mode) {
		LoadMigemo();
	}
	_migemo_mode = _default_migemo;
}

void LoadMigemo()
{
	if (_pMigemo == NULL) {
		_pMigemo = new CMigemo();
		if(!_pMigemo->LoadDLL(_migemo_path)) {
			_pMigemo = NULL;
			return;
		}
		if(!_pMigemo->Open(_migemo_dict_path)) {
			_pMigemo = NULL;
			return;
		}
		_migemo_mode = TRUE;

		HWND hDlg = ::GetParent(_hWndInput);
		::SetWindowText(hDlg, _title_with_migemo);
	}
}

void UnloadMigemo()
{
	if (_pMigemo != NULL) {
		_migemo_mode = FALSE;
		delete _pMigemo;
		_pMigemo = NULL;

		HWND hDlg = ::GetParent(_hWndInput);
		::SetWindowText(hDlg, _title);
	}
}

void RemoveLastLineFeed(HWND hList)
{
	WCHAR last_char[3] = {0};
	DWORD start, end;
	::SendMessage(hList, EM_SETSEL, -1, -1);
	::SendMessage(hList, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
	::SendMessage(hList, EM_SETSEL, end-1, end);
	::SendMessage(hList, EM_GETSELTEXT, 0, (LPARAM)last_char);

	if (last_char[0] == L'\r' || last_char[0] == L'\n') {
		::SendMessage(hList, EM_REPLACESEL, 0, (LPARAM)L"\0");
	}
}

bool IsOneLine(WCHAR *lines)
{
	WCHAR *line = wcschr(lines, L'\n');
	if (line != NULL) {
		line++;
		line = wcschr(line, L'\n');
	}

	return line == NULL;
}

void GetCommand(WCHAR *line, WCHAR *out, int len)
{
	WCHAR *command;
	command = wcsrchr(line, L'"');
	if (command == NULL) {
		return;
	}

	command++;
	while ( (*command == L' ') || (*command == L'\t') ) {
		command++;
	}
	wcsncpy(out, command, len);
	out[wcslen(out)-1] = L'\0';
}

bool RunInternalCommand(HWND hDlg, WPARAM wParam, WCHAR *command)
{
	int cnt = wcslen(command);
	if (cnt == 0) {
		cnt = 1;
	}
	if (wcsncmp(command, L"!!exit", cnt)==0) {
		EndDialog(hDlg, LOWORD(wParam));
	} else if (wcsncmp(command, L"!!file", cnt)==0) {
		LoadFiles();
	} else if (wcsncmp(command, L"!!reload", cnt)==0) {
		_bLoadFromIni = false;
		LoadMenus(hDlg, _hWndList, 1, __argc, __wargv);
	} else if (wcsncmp(command, L"!!kill", cnt)==0) {
		LoadKillTasks();
		_bLoadFromIni = false;
	} else if (wcsncmp(command, L"!!eject", cnt)==0) {
		LoadEjectDrives();
		_bLoadFromIni = false;
	} else {
		return false;
	}
	return true;
}

void SetCurrentLine()
{
	int cur_line_no = ::SendMessage(_hWndList, EM_LINEFROMCHAR, -1, 0);
	int cur_line_index = ::SendMessage(_hWndList, EM_LINEINDEX, cur_line_no, 0);
	int cur_line_length = ::SendMessage(_hWndList, EM_LINELENGTH, cur_line_index, 0);
	::SendMessage(_hWndList, EM_SETSEL, cur_line_index, cur_line_index+cur_line_length);

	if (_work != NULL) {
		delete [] _work;
	}
	_work = new WCHAR[cur_line_length + 2];
	_work_len = cur_line_length + 2;

	::SendMessage(_hWndList, EM_GETSELTEXT, 0, (LPARAM)_work);
	_work[cur_line_length] = L'\n';
	_work[cur_line_length + 1] = L'\0';
}

BOOL MoveHighlightLine(int amount)
{
	int line_count = ::SendMessage(_hWndList, EM_GETLINECOUNT, 0, 0) - 1;
	int cur_line_no = ::SendMessage(_hWndList, EM_LINEFROMCHAR, -1, 0);
	int cur_line_index = ::SendMessage(_hWndList, EM_LINEINDEX, cur_line_no, 0);
	int cur_line_length = ::SendMessage(_hWndList, EM_LINELENGTH, cur_line_index, 0);
	if (line_count == cur_line_no && amount == 1) {
		amount = 0 - cur_line_no;
	}
	if (line_count < cur_line_no + amount) {
		amount = line_count - cur_line_no;
	}
	if (cur_line_no > 0 && cur_line_no + amount < 0) {
		amount = 0 - cur_line_no;
	}
	if (cur_line_no == 0 && amount == -1) {
		amount = line_count - cur_line_no;
	}
	int next_line_index = ::SendMessage(_hWndList, EM_LINEINDEX, cur_line_no + amount, 0);
	int next_line_length = ::SendMessage(_hWndList, EM_LINELENGTH, next_line_index, 0);

	CHARFORMAT2 cfm;
	memset(&cfm, 0, sizeof(CHARFORMAT2));
	cfm.cbSize = sizeof(CHARFORMAT2);
	cfm.dwMask = CFM_BACKCOLOR;
	cfm.crBackColor = _color_list_bg;
	::SendMessage(_hWndList, EM_SETSEL, cur_line_index, cur_line_index + cur_line_length);
	if (SendMessage(_hWndList, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD, (LPARAM)&cfm) == 0) {
		return FALSE;
	}
	cfm.crBackColor = _color_hilight_line_bg;
	::SendMessage(_hWndList, EM_SETSEL, next_line_index, next_line_index + next_line_length);

	if (SendMessage(_hWndList, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD, (LPARAM)&cfm) == 0) {
		return FALSE;
	}

	::SendMessage(_hWndList, EM_SETSEL, next_line_index, next_line_index);
	::SetFocus(_hWndList);
	::SendMessage(_hWndList, EM_SCROLLCARET, 0, 0);
	::SetFocus(_hWndInput);

	return TRUE;
}

int CalcPageScrollStep(int fontHeight)
{
	HDC hdc = ::GetDC(_hWndList);
	RECT rect;
	::GetClientRect(_hWndList, &rect);
	int scaleY = ::GetDeviceCaps(hdc, LOGPIXELSY);
	int twip = 1440 / scaleY;
	return (rect.bottom - rect.top) * twip / ((fontHeight + LINESPACING_POINT) * 20) - 1;
}

BOOL SetWindowTextNoNotify(HWND hWnd, WCHAR *lpString)
{
	BOOL ret;
	_bNoEnChange = true;
	ret = SetWindowText(hWnd, lpString);
	_bNoEnChange = false;

	return ret;
}

