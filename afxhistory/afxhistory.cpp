/**
 * あふwのファイル窓にフォルダ履歴を表示するプラグイン
 * 
 */
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <windows.h>
#include <wchar.h>
#include <shlwapi.h>
#include <shlobj.h>
#include "afxwapi.h"
#include "afxcom.h"
#include "version.h"

#define PLUGIN_NAME    L"afxhistory"
#define PLUGIN_AUTHOR  L"yuratomo"

typedef struct _HistoryItem {
	wchar_t title[API_MAX_PATH_LENGTH];
	char    cmd[API_MAX_PATH_LENGTH];
} HistoryItem;
typedef struct _PluginData {
	AfxwInfo afx;
	HistoryItem* menu;
	int count;
	int index;
} PluginData, *lpPluginData;

void _format_title(wchar_t* title);

IDispatch* pAfxApp = NULL;

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
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

	memset(pdata, 0, sizeof(PluginData));
	memcpy(&pdata->afx, afxwInfo, sizeof(AfxwInfo));

	wcscpy(openInfo->szBaseDir, L"");
	wcscpy(openInfo->szInitRelDir, L"");
	wcsncpy(openInfo->szTitle, szCommandLine, sizeof(openInfo->szTitle));

	char path2[MAX_PATH];
	VARIANT result;

	// 窓番号
	AfxExec(DISPATCH_METHOD, &result, pAfxApp, L"SrcWin", 0);
	int win = result.intVal;

	// 履歴数
	wchar_t wbuf[32];
	wsprintfW(wbuf, L"%d", win);
	VARIANT x;
	x.vt = VT_BSTR;
	x.bstrVal = ::SysAllocString(wbuf);
	AfxExec(DISPATCH_METHOD, &result, pAfxApp, L"HisDirCount", 1, x);
	int nHisDirCnt = result.intVal;
	pdata->menu = new HistoryItem[nHisDirCnt];

	// 履歴読み込み
	int idx=0;
	for (idx=0; idx<nHisDirCnt; idx++) {
		VARIANT y;
		y.vt = VT_BSTR;
		wsprintfW(wbuf, L"%d", idx);
		y.bstrVal = ::SysAllocString(wbuf);
		AfxExec(DISPATCH_METHOD, &result, pAfxApp, L"HisDir", 2, y, x);
		if (result.bstrVal == NULL) continue;
		wcstombs(path2, result.bstrVal, sizeof(path2));
		sprintf(pdata->menu[idx].cmd,  "&EXCD -P\"%s\"", path2);
		wcscpy(pdata->menu[idx].title, result.bstrVal);
		// タイトル整形
		//_format_title(pdata->menu[idx].title);
	}
	pdata->index = 0;
	pdata->count = idx;

	return (HAFX)pdata;
}

void _format_title(wchar_t* title)
{
	int en_cnt = 0;
	int len = (int)wcslen(title);
	for (int idx=len-1; idx>=0; idx--) {
		if (title[idx] == L'\\') {
			en_cnt++;
			if (en_cnt == 2) {
				title[idx+1] = L' ';
				break;
			}
		} else {
			title[idx+1] = title[idx];
		}
	}
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
	if (pdata->menu != NULL) {
		delete [] pdata->menu;
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

	if (pdata->index >= pdata->count) {
		return -1;
	}

	swprintf(lpItemInfo->szItemName, L"%02d %s", pdata->index, pdata->menu[pdata->index].title);
	lpItemInfo->ullItemSize = pdata->index;
	lpItemInfo->dwAttr = FILE_ATTRIBUTE_NORMAL;

	pdata->index++;
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
	int idx = -1;
	wchar_t buf[1024];

	const wchar_t* start = wcschr(szItemPath, L'/');
	if (start != NULL) {
		start++;
	} else {
		start = szItemPath;
	}

	swscanf(start, L"%d %s", &idx, buf);
	
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	if (idx < 0 || idx >= pdata->count) {
		return 0;
	}
	AfxExec(pAfxApp, pdata->menu[idx].cmd);
	return 1;
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



