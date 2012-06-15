// AfxFind.cpp : アプリケーション用クラスの定義を行います。
//
	
#include "stdafx.h"
#include "AfxFind.h"
#include "afxcom.h"
#include <locale.h>
#include <stdlib.h>
#include <windows.h>
#include <commctrl.h>
#include <string>

#define EVENT_FIND_THREAD		L"EventFindThread"
#define MODIFIER_ALT			0x0004
#define MODIFIER_CONTROL		0x0002
#define MODIFIER_SHIFT			0x0001
#define MAX_BUF_SIZE			MAX_PATH * 2

// グローバル変数
AfxFind*    AfxFind::m_instance = NULL;
wchar_t*    pFindString = NULL;
wchar_t*    pFindPath = NULL;
size_t      nFindPathLen = 0;
wchar_t*    pFile = NULL;
int         pKeyCode = 70;
int         pModifier = 0x00;
//int       pVariable = 9;
HBRUSH      _brush = NULL;
wchar_t     _contents[256];
HWND        _hAfxWnd = NULL;
bool        _bAbort = false;
wchar_t     _ini_path[MAX_PATH];
wchar_t     _exe_path[MAX_PATH];

// エラーテーブル
const wchar_t *g_szErrorTable[] = {
	L"ファイルのオープンに失敗しました。",
	L"パラメタを指定してください。",
	L"検索に失敗しました。",
};

// このコード モジュールに含まれる関数の宣言を転送します :
LRESULT CALLBACK	ProcFindDialog(HWND, UINT, WPARAM, LPARAM);
DWORD	WINAPI 		FindThread(LPVOID p);
void				SendKey();
BOOL				Jump(int adjust);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	setlocale( LC_ALL, "Japanese" );

	// ini file path
	GetModuleFileName(hInstance, _exe_path, sizeof(_exe_path));
	wcscpy(_ini_path, _exe_path);
	size_t len = wcslen(_ini_path);
	_ini_path[len-1] = L'i';
	_ini_path[len-2] = L'n';
	_ini_path[len-3] = L'i';

	// find string
	pFindString = lpCmdLine;

	// find base directory
	wchar_t curdir[MAX_PATH];
	::GetCurrentDirectory(sizeof(curdir), curdir);
	pFindPath = curdir;
	nFindPathLen = wcslen(pFindPath);

	// result file
	wchar_t result[MAX_PATH];
	::GetPrivateProfileString(L"Config", L"result", L"", result, sizeof(result), _ini_path);
	if (result[1] != L':' || result[0] != L'\\') {
		wchar_t temp[MAX_PATH];
		wcsncpy(temp, _exe_path, MAX_PATH-1);
		wchar_t* en = wcsrchr(temp, L'\\');
		*(en+1) = L'\0';
		wcscat(temp, result);
		wcscpy(result, temp);
	}
	pFile = result;

	// send key code
	wchar_t keycode[32];
	::GetPrivateProfileString(L"Config", L"keycode", L"", keycode, sizeof(keycode), _ini_path);
	if (wcslen(keycode) == 4) {
		swscanf(keycode, L"%1d%3d", &pModifier, &pKeyCode);
	} else {
		pKeyCode = _wtoi(keycode);
	}

	if (pFile == NULL || pFindPath == NULL || pFindString == NULL) return -1;

	_hAfxWnd = ::FindWindow(L"TAfxWForm", NULL);
	if (_hAfxWnd == INVALID_HANDLE_VALUE) {
		_hAfxWnd = ::FindWindow(L"TAfxForm", NULL);
		if (_hAfxWnd == INVALID_HANDLE_VALUE) {
			return -1;
		}
	}

	// 背景色用ブラシ作成
	_brush = ::CreateSolidBrush(RGB(0,0,0));

	// 処理はスレッドで行う。
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, EVENT_FIND_THREAD);
	HANDLE hThreadFind = (HANDLE)CreateThread(NULL, 0, FindThread, NULL, 0, NULL);

	// スレッドから通知される状態を表示するプログレス画面
	INT_PTR ret = DialogBox(hInstance, (LPCWSTR)IDD_AFXFIND_DIALOG, NULL, (DLGPROC)ProcFindDialog);
	if (ret == -1) {
		wchar_t msg[256];
		swprintf(msg, L"Create dialog error.(error code=%d)", GetLastError());
		MessageBox(NULL, msg, L"afxfind", MB_OK);
		return -1;
	}

	SendKey();

	AfxFind::GetInstance()->DeleteInstance();
	if (hThreadFind != NULL) {
		CloseHandle(hThreadFind);
		hThreadFind = NULL;
	}

	::WaitForSingleObject(hThreadFind, INFINITE);

	if (_brush != NULL) {
		::DeleteObject(_brush);
	}

	return 0;
}

void SendKey()
{
	Sleep(100);

	DWORD buf;
	DWORD pid;
	DWORD tid = ::GetWindowThreadProcessId(_hAfxWnd, &pid);
	DWORD tidto = ::GetCurrentThreadId();
	::AttachThreadInput(tid, tidto, TRUE);
	::SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &buf, 0);
    ::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, NULL, 0);

	::SetForegroundWindow( _hAfxWnd );
	::BringWindowToTop( _hAfxWnd );

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

    ::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &buf, 0);
	::AttachThreadInput(tidto, tid, FALSE);
}

/**
 * インスタンスの取得
 */
AfxFind* AfxFind::GetInstance()
{
	if (m_instance == NULL) {
		m_instance = new AfxFind();
	}
	return m_instance;
}

/**
 * インスタンスの破棄
 */
void AfxFind::DeleteInstance()
{
	if (m_instance != NULL) {
		delete m_instance;
		m_instance = NULL;
	}
}

/**
 * コンストラクタ
 */
AfxFind::AfxFind()
{
	m_hWndProgress = (HWND)-1;
}

/**
 * 検索処理スレッド
 */
DWORD WINAPI FindThread(LPVOID p)
{
	AfxFind* finder = AfxFind::GetInstance();

	finder->FindThreadCallBack();

	return 0;
}

/**
 * あふメニュー出力
 */
void AfxFind::OutputMenu(wchar_t* lpszPath)
{
	//if (m_ofp == NULL) return;
	//fwprintf(m_ofp, L"%hs\n", lpszPath);
	wchar_t x[MAX_PATH];
	swprintf(x, L"%s\n", lpszPath);
	fwrite(x, wcslen(x) * sizeof(wchar_t), 1, m_ofp);
}

/**
 * ファイル検索
 */
bool AfxFind::FindFile(LPCTSTR lpszPath, LPCTSTR lpszFile, HWND hWnd)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA data;
    TCHAR szPath[ MAX_PATH ];
    TCHAR szNextPath[ MAX_PATH ];
	TCHAR szFind [ MAX_PATH ];
	if (_bAbort == true) {
		return false;
	}

	::SendMessage(hWnd, WM_CURRENT_FINDDIR, (WPARAM)lpszPath, NULL);

	if (lpszFile == NULL) {
		return false;
	}
	if (wcslen(lpszFile) <= 0 || wcslen(lpszFile) >= MAX_PATH) {
		return false;
	}

	// ファイル検索
	swprintf( szFind, L"%s\\%s", lpszPath, lpszFile);
	hFile = FindFirstFile(szFind, &data);
	if (hFile != INVALID_HANDLE_VALUE) {
		do {
			if (wcslen(lpszPath) > nFindPathLen) {
				swprintf( szFind, L"%s\\%s", &lpszPath[nFindPathLen+1], data.cFileName);
				OutputMenu(szFind);
				::SendMessage(hWnd, WM_FINDDIR, (WPARAM)szFind, NULL);
			} else {
				OutputMenu(data.cFileName);
				::SendMessage(hWnd, WM_FINDDIR, (WPARAM)data.cFileName, NULL);
			}
		}
		while (FindNextFile( hFile, &data));

		FindClose( hFile );
		hFile = INVALID_HANDLE_VALUE;
	}

	// サブディレクトリ検索
	swprintf( szPath, L"%s\\*", lpszPath);
	hFile = FindFirstFile(szPath, &data);
	if (hFile != INVALID_HANDLE_VALUE) {
		do {
			if( (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					!= FILE_ATTRIBUTE_DIRECTORY ) continue;

			//printf("find %s ...\n", lpszPath);

			if( wcscmp(data.cFileName, L".") == 0
					|| wcscmp(data.cFileName, L"..") == 0 ) continue;

			swprintf( szNextPath, L"%s\\%s", lpszPath, data.cFileName);

			if (!FindFile(szNextPath, lpszFile, hWnd)) {
				return false;
			}
		}
		while (FindNextFile( hFile, &data));

		FindClose( hFile );
	}

	return true;
}

/**
 * FindThreadから呼ばれる処理
 * @return 0
 */
int AfxFind::FindThreadCallBack()
{
	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, EVENT_FIND_THREAD);
	WaitForSingleObject(hEvent, INFINITE);
	CloseHandle(hEvent);

	_bAbort = false;

	try {
		if ((m_ofp = _wfopen(pFile, L"w, ccs=utf-8")) == NULL) {
			throw AFX_ERR_OPEN_FILE;
		}
		//fwprintf(m_ofp, L"#%hs\n", pFindPath);
		wchar_t x[MAX_PATH];
		swprintf(x, L"#%s\n", pFindPath);
		fwrite(x, wcslen(x) * sizeof(wchar_t), 1, m_ofp);

		// 検索開始
		if (!FindFile(pFindPath, pFindString, GetProgressWindowsHandle())) {
			//throw AFX_ERR_FIND;
		}

		//::SendMessage(GetProgressWindowsHandle(), WM_CLOSE, NULL, NULL);
	} catch (AFX_ERR_T nError) {
		int nErrorNum;
		// エラー通知
		if (nError < sizeof(g_szErrorTable)) {
			nErrorNum = nError;
		} else {
			nErrorNum = sizeof(g_szErrorTable)-1;
		}
		::MessageBox(NULL, g_szErrorTable[nError], L"エラー", MB_OK);
	} catch (...) {
		::MessageBox(NULL, g_szErrorTable[sizeof(g_szErrorTable)-1], L"エラー", MB_OK);
	}

	::SendMessage(GetProgressWindowsHandle(), WM_COMMAND, IDOK, NULL);

	if (m_ofp != NULL) {
		fclose(m_ofp);
		m_ofp = NULL;
	}

	return 0;
}

/**
 * プログレス表示画面のハンドル設定
 * @param[IN] hWnd プログレス表示画面のウィンドウハンドル
 */
void AfxFind::SetProgressWindowsHandle(HWND hWnd)
{
	m_hWndProgress = hWnd;
}

/**
 * プログレス表示画面のハンドル取得
 * @retval プログレス表示画面のウィンドウハンドル
 */
HWND AfxFind::GetProgressWindowsHandle()
{
	return m_hWndProgress;
}

LRESULT CALLBACK ProcFindDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	AfxFind* afxfind = AfxFind::GetInstance();
	HWND hWnd;
	HDC hDC;
	RECT rect, afxRect;

	HANDLE hEvent;
	switch (message)
	{
	case WM_INITDIALOG:
		// 準備完了のためメイン処理と同期
		afxfind->SetProgressWindowsHandle(hDlg);

		// あふの中央に表示
		::GetWindowRect(_hAfxWnd, &afxRect);
		::GetWindowRect(hDlg, &rect);
		::MoveWindow(hDlg, 
				(afxRect.left+afxRect.right)/2+(rect.left-rect.right)/2,
				(afxRect.top+afxRect.bottom)/2+(rect.top-rect.bottom)/2,
				rect.right-rect.left,
				rect.bottom-rect.top,
				FALSE);
		
		// スレッドを再開させる
		hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, EVENT_FIND_THREAD);
		SetEvent(hEvent);
		CloseHandle(hEvent);

		return TRUE;

	case WM_CURRENT_FINDDIR:
		SetDlgItemText(hDlg, IDC_STATUS_BAR, (wchar_t*)wParam);
		return TRUE;

	case WM_FINDDIR:
		hWnd = GetDlgItem(hDlg, IDC_EDIT_LOG);
		SendMessage(hWnd, EM_SETSEL, 0, 0);
		swprintf(_contents, L"%s\r\n", wParam);
		SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM)_contents);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;

		case IDCANCEL:
			_bAbort = true;
			//EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;

	case WM_CTLCOLORSTATIC:
		if ((HWND)lParam == GetDlgItem(hDlg, IDC_EDIT_LOG)) {
			hDC = (HDC)wParam;
			SetBkColor( hDC, RGB( 0, 0, 0)) ;
			SetTextColor(hDC, RGB(200,200,200));
			return (LRESULT)_brush;
		}
		break;
		
	}

	return FALSE;
}

