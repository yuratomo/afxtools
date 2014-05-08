/**
 * あふwのファイル窓にGITのログやブランチを表示するプラグイン
 * 
 */
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <windows.h>
#include <wchar.h>
#include "afxwapi.h"
#include "afxcom.h"
#include "version.h"

#define PLUGIN_NAME    L"afxgit"
#define PLUGIN_AUTHOR  L"yuratomo"
#define MAX_LINE_SIZE       4096
#define MAX_BRANCH_ITEM_LEN 128
#define MAX_LOG_ITEM_LEN    1024

typedef enum _Mode {
	Mode_Log,
	Mode_Branch,
	Mode_Status,
} Mode;

typedef struct _BranchItem {
	wchar_t name[MAX_BRANCH_ITEM_LEN];
	int selected; // 1: 選択中
} BranchItem;

typedef struct _LogItem {
	DWORD id;
	FILETIME tm;
	wchar_t msg[MAX_LOG_ITEM_LEN];
} LogItem;

typedef struct _StatusItem {
	wchar_t path[MAX_LINE_SIZE];
} StatusItem;

typedef struct _PluginData {
	wchar_t wszBaseDir[API_MAX_PATH_LENGTH];
	AfxwInfo afx;
	Mode mode;
	int branch_count;
	int branch_index;
	BranchItem* branch_items;
	int log_count;
	int log_index;
	LogItem* log_items;
	int status_count;
	int status_index;
	StatusItem* status_items;
} PluginData, *lpPluginData;

IDispatch* pAfxApp = NULL;
wchar_t _ini_path[MAX_PATH];
DWORD   _item_max = 64;
HINSTANCE _instance;

BOOL CreateSubDirectory(wchar_t *szPath);
void TrimCarriageReturn(wchar_t* wszBaseDir);
void ReplaceCharactor(wchar_t* wszPath, wchar_t from, wchar_t to);

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
			_instance = hinstDLL;
            setlocale( LC_ALL, "Japanese" );
            AfxInit(pAfxApp);
            break;

        case DLL_PROCESS_DETACH:
            AfxCleanup(pAfxApp);
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;
    }
    return  TRUE;
}

/**
 * プラグイン情報を取得する。
 * @param[out] pluginInfo    プラグイン情報。
 */
void WINAPI ApiGetPluginInfo(lpApiInfo pluginInfo)
{
	if (pluginInfo == NULL) {
		return;
	}

	memset(pluginInfo, 0, sizeof(ApiInfo));

	// プラグイン名
	wcsncpy(pluginInfo->szPluginName, PLUGIN_NAME, API_MAX_PLUGIN_NAME_LENGTH-1);

	// 作者
	wcsncpy(pluginInfo->szAuthorName, PLUGIN_AUTHOR, API_MAX_AUTHOR_NAME_LENGTH-1);

	// プラグインバージョン
	pluginInfo->dwVersion = MAKE_VERSION(VERSION_MAJOR, VERSION_MINOR, VERSION_BUGFIX, VERSION_REVISION);
}

DWORD run(wchar_t* cmdline, const wchar_t* redirectTo)
{
    STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcessInfo;
    wchar_t Args[4096];

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(STARTUPINFO);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_HIDE;

    Args[0] = 0;
    wcscat(Args, cmdline);  
    wcscat(Args, L" > \""); 
    wcscat(Args, redirectTo);
    wcscat(Args, L"\" 2>&1"); 

    if (!CreateProcess( NULL, Args, NULL, NULL, FALSE,
        CREATE_NEW_CONSOLE, 
        NULL, 
        NULL,
        &StartupInfo,
        &ProcessInfo))
    {
        return GetLastError();
    }
	if (WaitForSingleObject(ProcessInfo.hProcess, INFINITE) == WAIT_OBJECT_0) {
		return 0;
	}

    return -1;
}

/**
 * プラグインオープンする。
 * @param[in]  szCommandLine プラグイン側に渡すコマンドライン。
 * @param[in]  afxwInfo      あふwの情報。
 * @param[out] openInfo      オープン情報。
 * @retval     0以外         プラグインで自由に使えるハンドル。その他APIのhandleとして使用する。
 * @retval     NULL(0)       オープン失敗。
 */
HAFX WINAPI ApiOpen(LPCWSTR szCommandLine, const lpAfxwInfo afxwInfo, lpApiOpenInfo openInfo)
{
	lpPluginData pdata = (lpPluginData)GlobalAlloc(GMEM_FIXED, sizeof(PluginData));
	if (pdata == NULL) {
		return 0;
	}

	wchar_t wcmd[MAX_PATH+32];
	wchar_t dir[MAX_PATH];
	wchar_t path[MAX_PATH];
	memset(dir, 0, sizeof(dir));
	memset(path, 0, sizeof(path));
	GetTempPath(MAX_PATH, dir);
	GetTempFileName(dir, L"afx", 0, path);

	memset(pdata, 0, sizeof(PluginData));
	memcpy(&pdata->afx, afxwInfo, sizeof(AfxwInfo));

	GetModuleFileName(_instance, _ini_path, sizeof(_ini_path));
	size_t len = wcslen(_ini_path);
	_ini_path[len-1] = L'i';
	_ini_path[len-2] = L'n';
	_ini_path[len-3] = L'i';
	::GetPrivateProfileString(L"Config", L"title", L"ぎっと", openInfo->szTitle, sizeof(openInfo->szTitle), _ini_path);
	_item_max = ::GetPrivateProfileInt(L"Config", L"item_max", 64, _ini_path);
	if (_item_max < 16) {
		_item_max = 16;
	}

	wcscpy(openInfo->szBaseDir, L"");
	wcscpy(openInfo->szInitRelDir, L"");

	int ret;
	if (wcsncmp(szCommandLine, L"branch", 6) == 0) {
	// BRANCH
		pdata->mode = Mode_Branch;
		ret = run(L"cmd /c git branch", path);
		wcsncpy(pdata->wszBaseDir, &szCommandLine[7], API_MAX_PATH_LENGTH);
	} else if (wcsncmp(szCommandLine, L"log", 3) == 0) {
	// LOG
		pdata->mode = Mode_Log;
		swprintf(wcmd, L"cmd /c git log --pretty=format:%s -50 \"%s\"", L"\"%at %h %s\"", &szCommandLine[4]);
		ret = run(wcmd, path);
		//ret = run(L"cmd /c git log --pretty=format:\"%at %h %s\" -50", path);
		wcsncpy(pdata->wszBaseDir, &szCommandLine[4], API_MAX_PATH_LENGTH);
	} else if (wcsncmp(szCommandLine, L"status", 6) == 0) {
	// STATUS
		pdata->mode = Mode_Status;
		ret = run(L"cmd /c git status -s", path);
		wcsncpy(pdata->wszBaseDir, &szCommandLine[7], API_MAX_PATH_LENGTH);
	} else {
		GlobalFree(pdata);
		return NULL;
	}

	if (ret != 0) {
		GlobalFree(pdata);
		pdata = NULL;
	} else {
		// load
		int num = 0;
		FILE* fp = _wfopen(path, L"r, ccs=utf-8");
		if (fp == NULL) {
			GlobalFree(pdata);
			pdata = NULL;
		} else {
			wchar_t temp[MAX_LINE_SIZE];
			while (fgetws(temp, MAX_LINE_SIZE, fp) != NULL) {
				num++;
			}
			if (num != 0) {
				// BRANCH
				if (pdata->mode == Mode_Branch) {
					pdata->branch_items = new BranchItem[num];
					pdata->branch_count = num;
				} else 
				// LOG
				if (pdata->mode == Mode_Log) {
					pdata->log_items = new LogItem[num];
					pdata->log_count = num;
				} else
				// STATUS
				if (pdata->mode == Mode_Status) {
					pdata->status_items = new StatusItem[num];
					pdata->status_count = num;
				}

				int idx = 0;
				fseek(fp, 0, SEEK_SET);
				while (fgetws(temp, MAX_LINE_SIZE, fp) != NULL) {

					// BRANCH
					if (pdata->mode == Mode_Branch) {
						if (wcslen(temp) > 3) {
							if (temp[0] == L'*') {
								pdata->branch_items[idx].selected = 1;
							} else {
								pdata->branch_items[idx].selected = 0;
							}
							wcsncpy(pdata->branch_items[idx].name, &temp[0], MAX_BRANCH_ITEM_LEN-1);
							idx++;
						}
					} else

					// LOG
					if (pdata->mode == Mode_Log) {
						if (wcslen(temp) > 1) {
							int tm;
							memset(pdata->log_items[idx].msg, 0, MAX_LOG_ITEM_LEN);
							swscanf(temp, L"%10d %07x %s",
								    &tm,
									&pdata->log_items[idx].id,
									pdata->log_items[idx].msg);
							LONGLONG ll = Int32x32To64(tm, 10000000) + 116444736000000000;
							pdata->log_items[idx].tm.dwHighDateTime = (ll >> 32) & 0xFFFFFFFF;
							pdata->log_items[idx].tm.dwLowDateTime  =  ll        & 0xFFFFFFFF;
							idx++;	
						}
					} else

					// STATUS
					if (pdata->mode == Mode_Status) {
						if (wcslen(temp) > 1) {
							//int tm;
							wcscpy(pdata->status_items[idx].path, temp);
							TrimCarriageReturn(pdata->status_items[idx].path);
							idx++;	
						}
					}
				}
			}
			pdata->branch_index = 0;
			pdata->log_index = 0;
		}
	}

	DeleteFile(path);

	return (HAFX)pdata;
}

/**
 * プラグインクローズする。
 * @param[in]  handle        ApiOpenで開いたハンドル。
 * @retval     1             成功
 * @retval     0             エラー
 */
int  WINAPI ApiClose(HAFX handle)
{
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}
	if (pdata != NULL) {
		GlobalFree(pdata);
	}
	return 1;
}

/**
 * szDirPathで指定された仮想ディレクトリパスの先頭アイテムを取得する。
 * @param[in]  handle        ApiOpenで開いたハンドル。
 * @param[in]  szDirPath     取得したいディレクトリフルパス。
 * @param[out] lpItemInfo    先頭アイテムの情報。
 * @retval     1             アイテムを見つけた場合
 * @retval     0             エラー
 * @retval     -1            検索終了
 */
int  WINAPI ApiFindFirst(HAFX handle, LPCWSTR szDirPath, /*LPCWSTR szWildName,*/ lpApiItemInfo lpItemInfo)
{
	return ApiFindNext(handle, lpItemInfo);

}

/**
 * ApiFindFirstで取得したアイテム以降のアイテムを取得する。
 * @param[in]  handle        ApiOpenで開いたハンドル。
 * @param[out] lpItemInfo    先頭アイテムの情報。
 * @retval     1             アイテムを見つけた場合
 * @retval     0             エラー
 * @retval     -1            検索終了
 */
int  WINAPI ApiFindNext(HAFX handle, lpApiItemInfo lpItemInfo)
{
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}
	if (lpItemInfo == NULL) {
		return -1;
	}

	memset(lpItemInfo->szItemName, 0, API_MAX_PATH_LENGTH);

	// BRANCH
	if (pdata->mode == Mode_Branch) {
		if (pdata->branch_index >= pdata->branch_count) {
			return -1;
		}

		swprintf(lpItemInfo->szItemName, API_MAX_PATH_LENGTH, L"%s", pdata->branch_items[pdata->branch_index].name);
		lpItemInfo->ullItemSize = 0; // pdata->branch_items[pdata->branch_index].selected;
		lpItemInfo->dwAttr = FILE_ATTRIBUTE_NORMAL;
		pdata->branch_index++;

	} else

	// LOG
	if (pdata->mode == Mode_Log) {
		if (pdata->log_index >= pdata->log_count) {
			return -1;
		}

		swprintf(lpItemInfo->szItemName, _item_max, L"%07x %s",
				pdata->log_items[pdata->log_index].id,
				pdata->log_items[pdata->log_index].msg);
		/*
		swprintf(lpItemInfo->szItemName, API_MAX_PATH_LENGTH, L"%s", pdata->log_items[pdata->log_index].msg);
		*/
		lpItemInfo->ullItemSize = pdata->log_index;
		lpItemInfo->dwAttr = FILE_ATTRIBUTE_NORMAL;
		lpItemInfo->ullTimestamp = pdata->log_items[pdata->log_index].tm;
		pdata->log_index++;
	} else

	// STATUS
	if (pdata->mode == Mode_Status) {
		if (pdata->status_index >= pdata->status_count) {
			return -1;
		}

		wcscpy(lpItemInfo->szItemName, pdata->status_items[pdata->status_index].path);

		wchar_t path[API_MAX_PATH_LENGTH];
		wsprintf(path, L"%s\\%s", pdata->wszBaseDir, &pdata->status_items[pdata->status_index].path[3]);
		//ReplaceCharactor(path, L'/', L'\\');

		WIN32_FILE_ATTRIBUTE_DATA attrData;
		BOOL bRet = GetFileAttributesEx(path, GetFileExInfoStandard, &attrData);
		if (bRet) {
			if ( (attrData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
				lpItemInfo->dwAttr = FILE_ATTRIBUTE_SYSTEM;
				wcscat(lpItemInfo->szItemName, L"/");
			} else {
				lpItemInfo->dwAttr = attrData.dwFileAttributes;
			}
			lpItemInfo->ullItemSize = (((ULONGLONG)attrData.nFileSizeHigh) << 32) | (ULONGLONG)attrData.nFileSizeLow;
			lpItemInfo->ullTimestamp = attrData.ftLastWriteTime;
		} else {
			lpItemInfo->ullItemSize = pdata->status_index;
			lpItemInfo->dwAttr = FILE_ATTRIBUTE_NORMAL;
		}
		pdata->status_index++;
	} else {
		return -1;
	}

	return 1;
}

/**
 * アイテムを拡張子判別実行する。
 * あふwでENTERやSHIFT-ENTERを押したときに呼び出される。
 * @param[in]  handle        ApiOpenで開いたハンドル。
 * @param[in]  szItemPath    アイテムのフルパス。
 * @retval     2             あふw側に処理を任せる。（ApiCopyでテンポラリにコピーしてから実行)
 * @retval     1             成功
 * @retval     0             エラー
 */
int  WINAPI ApiExecute(HAFX handle, LPCWSTR szItemPath)
{
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	wchar_t dir[MAX_PATH];
	wchar_t path[MAX_PATH];
	memset(dir, 0, sizeof(dir));
	memset(path, 0, sizeof(path));
	GetTempPath(MAX_PATH, dir);
	GetTempFileName(dir, L"afx", 0, path);

	if (pdata->mode == Mode_Branch) {
		wchar_t cmd[MAX_LINE_SIZE];
		swprintf(cmd, MAX_LINE_SIZE, L"cmd /c git checkout %s", &szItemPath[3]);
		int ret = run(cmd, path);

		FILE* fp = _wfopen(path, L"r");
		if (fp != NULL) {
			wchar_t temp[MAX_LINE_SIZE];
			while (fgetws(temp, MAX_LINE_SIZE, fp) != NULL) {
				swprintf(cmd, MAX_LINE_SIZE, L"&ECHO %s", temp);
				char msg[MAX_LINE_SIZE];
				wcstombs(msg, cmd, sizeof(msg));
				AfxExec(pAfxApp, msg);
			}
			fclose(fp);
		}

		AfxExec(pAfxApp, "&RELOAD");
		DeleteFile(path);
	} else

	if (pdata->mode == Mode_Log) {
		int id;
		wchar_t cmd[1024];
		swscanf(&szItemPath[1], L"%07x %s", &id, cmd);
		swprintf(cmd, sizeof(cmd), L"cmd /c git show %07x \"%s\"", id, pdata->wszBaseDir);
		int ret = run(cmd, path);

		char cpath[MAX_PATH];
		wcstombs(cpath, path, sizeof(cpath));

		char viewcmd[MAX_PATH];
		sprintf(viewcmd, "&VIEW %s", cpath);
		AfxExec(pAfxApp, viewcmd);
		DeleteFile(path);
	} else

	if (pdata->mode == Mode_Status) {
		wchar_t wcmd[MAX_PATH+32];
		swprintf(wcmd, L"&EXCD -P\"%s\\%s\"", pdata->wszBaseDir, &szItemPath[4]);

		char cmd[MAX_PATH+32];
		wcstombs(cmd, wcmd, sizeof(cmd));

		AfxExec(pAfxApp, cmd);
	}

	return 1;
}

/**
 * アイテムを拡張子判別実行する。
 * あふwでENTERやSHIFT-ENTERを押したときに呼び出される。
 * @param[in]  handle        ApiOpenで開いたハンドル。
 * @param[in]  szItemPath    アイテムのフルパス。
 * @retval     2             あふw側に処理を任せる。（ApiCopyでテンポラリにコピーしてから実行)
 * @retval     1             成功
 * @retval     0             エラー
 */
int  WINAPI ApiExecute2(HAFX handle, LPCWSTR szItemPath)
{
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	if (pdata->mode == Mode_Status) {
		wchar_t cmd[1024];
		swprintf(cmd, sizeof(cmd), L"TortoiseGitProc.exe /command:diff /path:\"%s\\%s\"",
				pdata->wszBaseDir, &szItemPath[4]);
		if (run(cmd, L"NUL") == 0) {
			return 1;
		}
	}

	return 0;
}

/**
 * アイテムをコピーする。
 * あふwでコピー処理をするときに呼び出される。
 * @param[in]  handle        ApiOpenで開いたハンドル。
 * @param[in]  szFromItem    コピー元のアイテム名。
 * @param[in]  szToPath      コピー先のフォルダ。
 * @param[in]  lpPrgRoutine  コールバック関数(CopyFileExと同様)
 * @retval     1             成功
 * @retval     0             エラー
 */
int  WINAPI ApiCopyTo(HAFX handle, LPCWSTR szFromItem, LPCWSTR szToPath, LPPROGRESS_ROUTINE lpPrgRoutine)
{
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	if (pdata->mode == Mode_Status) {
		wchar_t from[MAX_PATH];
		swprintf(from, MAX_PATH-1, L"%s\\%s", pdata->wszBaseDir, &szFromItem[3]);
		wchar_t to[MAX_PATH];
		swprintf(to, MAX_PATH-1, L"%s%s", szToPath, &szFromItem[3]);
		
		// サブディレクトリ
		if (CreateSubDirectory(to) == FALSE) {
			return 0;
		}

		// コピー
		size_t len = wcslen(szFromItem);
		if (szFromItem[len-1] != L'/') {
			if (::CopyFile(from, to, FALSE) == 0) {
				return 0;
			}
		}
		return 1;
	}

	return 0;
}

/**
 * @brief あふwが内部的に利用するコピー処理。
 * 主にプラグイン内のファイルを開く場合に、AFXWTMP以下にコピーするために使われる。
 * ただし、実際にコピーするかどうかはプラグイン次第で、あふwはszOutputPathで指定されたファイルを開く。
 *
 * @param[in]  handle        ApiOpenで開いたハンドル。
 * @param[in]  szFromItem    プラグイン内のアイテム名
 * @param[in]  szToPath      コピー先のフォルダ(c:\Program Files\AFXWTMP.0\hoge.txt等が指定される)
 * @param[out] szOutputPath  プラグインが実際にコピーしたファイルパス
 *                           szToPathにコピーする場合はszToPathをszOutputPathにコピーしてあふwに返す。
 *                           szToPathにコピーしない場合、あふwに開いてほしいファイルのパスを指定する。
 * @param[in]  dwOutPathSize szOutputPathのバッファサイズ
 * @param[in]  lpPrgRoutine  コールバック関数(CopyFileExと同様)
 * @retval     1             成功
 * @retval     0             エラー
 */
int  WINAPI ApiIntCopyTo(HAFX handle, LPCWSTR szFromItem, LPCWSTR szToPath, LPWSTR szOutputPath, DWORD dwOutPathSize, LPPROGRESS_ROUTINE lpPrgRoutine)
{
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	if (pdata->mode == Mode_Log) {
		int id;
		wchar_t cmd[1024];
		swscanf(szFromItem, L"%07x %s", &id, cmd);
		swprintf(cmd, sizeof(cmd), L"cmd /c git show %07x", id);

		wchar_t path[API_MAX_PATH_LENGTH];
		swprintf(path, API_MAX_PATH_LENGTH - 1, L"%s%s", szToPath, PLUGIN_NAME);

		int ret = run(cmd, path);
		wcsncpy(szOutputPath, path, dwOutPathSize - 1);
		return ret;
	} 
	else if (pdata->mode == Mode_Status) {
		swprintf(szOutputPath, dwOutPathSize - 1, L"%s\\%s", pdata->wszBaseDir, &szFromItem[3]);
		return 1;
	}

	return 0;
}

/**
 * アイテムを削除する。
 * あふwで削除をするときに呼び出される。
 * @param[in]  handle        ApiOpenで開いたハンドル。
 * @param[in]  szItemPath    削除するアイテムのフルパス。
 * @retval     1             成功
 * @retval     0             エラー
 */
int  WINAPI ApiDelete(HAFX handle, LPCWSTR szFromItem)
{
	/*
	char cmd[MAX_PATH+32];

    wchar_t label = szFromItem[0];
    const wchar_t* p = wcsrchr(szFromItem, L')');
    if (p != NULL) {
        label = *(--p);
    }

    if ( (label >= L'A' && label <= L'Z' ) ||
            (label >= L'a' && label <= L'z' ) ) {
        sprintf(cmd, "&EJECT %C", label);
        AfxExec(pAfxApp, cmd);
        return 1;
    }
	*/
	return 0;
}

//------------------------------ 以下ポストローンチ --------------------------------

/**
 * あふw側からプラグインにアイテムをコピーする。
 * あふwでコピー処理をするときに呼び出される。
 * @param[in]  handle        ApiOpenで開いたハンドル。
 * @param[in]  szFromItem    コピー元のアイテム名。
 * @param[in]  szToPath      コピー先のフォルダ。
 * @param[in]  lpPrgRoutine  コールバック関数(CopyFileExと同様)
 * @retval     1             成功
 * @retval     0             エラー
 */
int  WINAPI ApiCopyFrom(HAFX handle, LPCWSTR szFromItem, LPCWSTR szToPath, LPPROGRESS_ROUTINE lpPrgRoutine)
{
	return 0;
}

/**
 * アイテムを移動する。
 * あふwで移動処理をするときに呼び出される。
 * @param[in]  handle        ApiOpenで開いたハンドル。
 * @param[in]  szFromPath    移動元のフルパス。
 * @param[in]  szToPath      移動先のフルパス。
 * @param[in]  direction     ApiDirection_A2Pはあふwからプラグイン側に移動する場合。
 *                           ApiDirection_P2Aはプラグイン側からあふwに移動する場合。
 * @retval     1             成功
 * @retval     0             エラー
 */
int  WINAPI ApiMove(HAFX handle, LPCWSTR szFromPath, LPCWSTR szToPath, ApiDirection direction)
{
	return 0;
}

/**
 * アイテムをリネームする。
 * あふwでリネームをするときに呼び出される。
 * @param[in]  handle        ApiOpenで開いたハンドル。
 * @param[in]  szFromPath    リネーム前のフルパス。
 * @param[in]  szToName      リネーム後のファイル名。
 * @retval     1             成功
 * @retval     0             エラー
 */
int  WINAPI ApiRename(HAFX handle, LPCWSTR szFromPath, LPCWSTR szToName)
{
	return 0;
}

/**
 * ディレクトリを作成する。
 * あふwでディレクトリを作成をするときに呼び出される。
 * @param[in]  handle        ApiOpenで開いたハンドル。
 * @param[in]  szItemPath    作成するディレクトリのフルパス。
 * @retval     1             成功
 * @retval     0             エラー
 */
int  WINAPI ApiCreateDirectory(HAFX handle, LPCWSTR szItemPath)
{
	return 0;
}

/**
 * ディレクトリを削除する。
 * あふwでディレクトリを削除をするときに呼び出される。
 * @param[in]  handle        ApiOpenで開いたハンドル。
 * @param[in]  szItemPath    削除するディレクトリのフルパス。
 * @retval     1             成功
 * @retval     0             エラー
 */
int  WINAPI ApiRemoveDirectory(HAFX handle, LPCWSTR szItemPath)
{
	return 0;
}

/**
 * コンテキストメニュー表示
 * @param[in]  handle        ApiOpenで開いたハンドル。
 * @param[in]  hWnd          あふwのウィンドウハンドル。
 * @param[in]  x             メニュー表示X座標。
 * @param[in]  y             メニュー表示Y座標。
 * @param[in]  szItemPath    アイテムのフルパス。
 * @retval     2             あふw側に処理を任せる。（ApiCopyでテンポラリにコピーしてからコンテキストメニュー?)
 * @retval     1             成功
 * @retval     0             エラー
 */
int  WINAPI ApiShowContextMenu(HAFX handle, const HWND hWnd, DWORD x, DWORD y, LPCWSTR szItemPath)
{
	return 0;
}

//------------------------------ 内部関数 --------------------------------

BOOL CreateSubDirectory(wchar_t *szPath)
{
    wchar_t szMakePath[MAX_PATH+1];

    if (wcslen(szPath) > MAX_PATH) {
        return FALSE;
    }

    for (unsigned int i = 0; i < wcslen(szPath); i++) {
        szMakePath[i] = szPath[i];
        if (szPath[i] == L'\\' || szPath[i] == L'/') {
            szMakePath[i+1] = L'\0';
            if ( GetFileAttributes(szMakePath) == 0xFFFFFFFF ) {
                if (!CreateDirectory(szMakePath, NULL) ) {
                    return FALSE;
                }
            }
        }
    }

    return TRUE;
}

void TrimCarriageReturn(wchar_t* wszBaseDir)
{
	size_t len = wcslen(wszBaseDir);
	if (wszBaseDir[len-1] == L'\n') {
		if (wszBaseDir[len-2] == L'\\' || wszBaseDir[len-2] == L'/') {
			wszBaseDir[len-2] = L'\0';
		} else {
			wszBaseDir[len-1] = L'\0';
		}
	} else {
		if (wszBaseDir[len-1] == L'\\' || wszBaseDir[len-1] == L'/') {
			wszBaseDir[len-1] = L'\0';
		}
	}
}

void ReplaceCharactor(wchar_t* wszPath, wchar_t from, wchar_t to)
{
	size_t len = wcslen(wszPath);
	for (size_t idx=0; idx<len; idx++) {
		if (wszPath[idx] == from) {
			wszPath[idx] = to;
		}
	}
}
