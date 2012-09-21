/**
 * 1行に1つのファイルパスが書かれたファイルを
 * あふwのファイル窓に表示するプラグイン
 * 
 */
#include <string.h>
#include <stdio.h>
#include "afxwapi.h"
#include "version.h"
#include <wchar.h>

#define PLUGIN_NAME    L"afxflist"
#define PLUGIN_AUTHOR  L"yuratomo"

void TrimCarriageReturn(wchar_t* wszBaseDir);
void ReplaceCharactor(wchar_t* wszPath, wchar_t from, wchar_t to);

typedef struct _PluginData {
	AfxwInfo afx;
	wchar_t wszBaseDir[API_MAX_PATH_LENGTH];
	FILE* fp;
} PluginData, *lpPluginData;

//void _ShowLastError();

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

	pdata->fp = _wfopen(szCommandLine, L"r, ccs=utf-8");
	if (pdata->fp == NULL) {
		GlobalFree(pdata);
		return 0;
	}

	wcsncpy(openInfo->szBaseDir, L"", API_MAX_PATH_LENGTH);

	// 先頭に#付きでパスが書いてあれば、基準ディレクトリにする
	wchar_t temp[API_MAX_PATH_LENGTH];
	if (fgetws(temp, API_MAX_PATH_LENGTH, pdata->fp) != NULL) {
		if (temp[0] == L'#' && wcslen(temp) > 1) {
			wcsncpy(openInfo->szBaseDir, &temp[1], API_MAX_PATH_LENGTH);
		} else {
			fseek(pdata->fp, 0, SEEK_SET);
		}
	}

	wcsncpy(pdata->wszBaseDir, openInfo->szBaseDir, API_MAX_PATH_LENGTH);
	TrimCarriageReturn(pdata->wszBaseDir);
	ReplaceCharactor(pdata->wszBaseDir, L'\\', L'/');
	if (wcscmp(pdata->wszBaseDir, L"") != 0) {
		wcsncpy(openInfo->szBaseDir, pdata->wszBaseDir, API_MAX_PATH_LENGTH);
	}

	// 初期相対ディレクトリはルート("/")
	wcscpy(openInfo->szInitRelDir, L"/");
	wcscpy(openInfo->szTitle, L"検索結果");

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

	fclose(pdata->fp);
	pdata->fp = NULL;

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
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	//テキストモードだとftellがうまく拾えないので、先頭にもどしてから１行読み飛ばすようにする
	fseek(pdata->fp, 0, SEEK_SET);
	if (fgetws(lpItemInfo->szItemName, API_MAX_PATH_LENGTH, pdata->fp) == NULL) {
		return -1;
	}

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

	wchar_t path[API_MAX_PATH_LENGTH];
	while (1) {
		if (fgetws(lpItemInfo->szItemName, API_MAX_PATH_LENGTH, pdata->fp) == NULL) {
			if (feof(pdata->fp)) {
				return 0;
			} else {
				return -1;
			}
		}
		size_t len = wcslen(lpItemInfo->szItemName);
		if (len > 0 && lpItemInfo->szItemName[len-1] == L'\n') {
			lpItemInfo->szItemName[len-1] = L'\0';
		}

		wsprintf(path, L"%s/%s", pdata->wszBaseDir, lpItemInfo->szItemName);
		WIN32_FILE_ATTRIBUTE_DATA attrData;
		BOOL bRet = GetFileAttributesEx(path, GetFileExInfoStandard, &attrData);
		if (bRet == FALSE) {
			continue;
		}
		lpItemInfo->dwAttr = attrData.dwFileAttributes;
		lpItemInfo->ullItemSize = (((ULONGLONG)attrData.nFileSizeHigh) << 32) | (ULONGLONG)attrData.nFileSizeLow;
		lpItemInfo->ullTimestamp = attrData.ftLastWriteTime;
		break;
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

#if 0
	if (pdata->afx.MesPrint != NULL) {
		wchar_t szTest[512];
		_swprintf(szTest, L"execute %s.",szItemPath);
		pdata->afx.MesPrint(szTest);
	}
#endif

	/*
	if (ShellExecute(m_hWnd, L"open", szItemPath, NULL, NULL, SW_SHOWNORMAL) <= 32) {
		return 0;
	}
	*/
	int ret = 0;
#if 0
	if (pdata->afx.Exec != NULL) {
		wchar_t szCmd[512];
		_swprintf(szCmd, L"&EXCD -P\"%s/%s\"", pdata->wszBaseDir, szItemPath);
		BOOL bRet = pdata->afx.Exec(szCmd);
		if (bRet) {
			ret = 1;
		} else {
			ret = 0;
		}
	}
#endif

	return ret;
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
int  WINAPI ApiCopy(HAFX handle, LPCWSTR szFromItem, LPCWSTR szToPath, LPPROGRESS_ROUTINE lpPrgRoutine)
{
	return ApiCopyTo(handle, szFromItem, szToPath, lpPrgRoutine);
}
int  WINAPI ApiCopyTo(HAFX handle, LPCWSTR szFromItem, LPCWSTR szToPath, LPPROGRESS_ROUTINE lpPrgRoutine)
{
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	wchar_t pathFrom[API_MAX_PATH_LENGTH];
	wchar_t pathTo[API_MAX_PATH_LENGTH];
	wsprintf(pathFrom, L"%s/%s", pdata->wszBaseDir, szFromItem);
	const wchar_t* pFromName = wcsrchr(szFromItem, L'\\');
	if (pFromName == NULL) {
		pFromName = szFromItem;
	} else {
		pFromName++;
	}
	wsprintf(pathTo,   L"%s/%s", szToPath, pFromName);

	BOOL bCancel = FALSE;
	int ret = 0;
	BOOL bRet = ::CopyFileEx(pathFrom, pathTo, lpPrgRoutine, NULL, &bCancel, COPY_FILE_RESTARTABLE);
	if (bRet) {
		ret = 1;
	}

	return ret;
}

/**
 * あふwが内部的に利用するコピー処理。
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

	wchar_t pathTo[API_MAX_PATH_LENGTH];
	wsprintf(pathTo,   L"%s\\%s", pdata->wszBaseDir, szFromItem);

	// バッファサイズチェック
	size_t len = wcslen(pathTo);
	if (len > dwOutPathSize) {
		return 0;
	}

	// afxflistはファイルコピーしないで、ローカルパスを返す。
	wcsncpy(szOutputPath, pathTo, dwOutPathSize - 1);
	return 1;
}

/**
 * アイテムを削除する。
 * あふwで削除をするときに呼び出される。
 * @param[in]  handle        ApiOpenで開いたハンドル。
 * @param[in]  szItemPath    削除するアイテムのフルパス。
 * @retval     1             成功
 * @retval     0             エラー
 */
int  WINAPI ApiDelete(HAFX handle, LPCWSTR szItemPath)
{
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	wchar_t path[API_MAX_PATH_LENGTH];
	wsprintf(path, L"%s\\%s", pdata->wszBaseDir, szItemPath);

	int ret = 0;
	BOOL bRet = ::DeleteFile(path);
	if (bRet) {
		ret = 1;
	}

	return ret;
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
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	// あふw側からプラグイン側への移動は不可
	if (direction == ApiDirection_A2P) {
		return 0;
	}

	wchar_t path[API_MAX_PATH_LENGTH];
	wsprintf(path, L"%s/%s", pdata->wszBaseDir, szFromPath);

	int ret = 0;
	BOOL bRet = ::MoveFile(path, szToPath);
	if (bRet) {
		ret = 1;
	}

	return ret;
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
	lpPluginData pdata = (lpPluginData)handle;
	if (pdata == NULL) {
		return 0;
	}

	wchar_t pathFrom[API_MAX_PATH_LENGTH];
	wsprintf(pathFrom, L"%s/%s", pdata->wszBaseDir, szFromPath);

	wchar_t pathTo[API_MAX_PATH_LENGTH];
	wcscpy(pathTo, pathFrom);
	wchar_t* en = wcsrchr(pathTo, L'/');
	if (en == NULL) {
		return 0;
	}
	*(en+1) = L'\0';
	wcscat(pathTo, szToName);

	int ret = !_wrename(pathFrom, pathTo);

	return ret;
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
	return 2;
}

/*
void _ShowLastError()
{
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
	  FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	  NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
	  (LPTSTR) &lpMsgBuf, 0, NULL);
	MessageBox(NULL, (const wchar_t*)lpMsgBuf, NULL, MB_OK);
	LocalFree(lpMsgBuf);
}
*/

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
