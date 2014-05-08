/**
 * あふwのファイル窓にドライブ一覧を表示するプラグイン
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

#define PLUGIN_NAME    L"afxdrive"
#define PLUGIN_AUTHOR  L"yuratomo"

typedef struct _PluginData {
	AfxwInfo afx;
	DWORD dwDrives;
	int index;
	int sindex;
} PluginData, *lpPluginData;

// 特殊ディレクトリ定義
static const wchar_t* _SpecialDirName [] = {
	L"0) My Document",
	L"1) Desktop",
    L"2) System32",
    L"3) Program Files",
};
static int _SpecialDirTable [] = {
	CSIDL_PERSONAL,            // My Document
	CSIDL_DESKTOPDIRECTORY,    // Desktop
    -1,                        // System32
    -2,                        // Program Files
};
static int _SpecialDirTableCount = sizeof(_SpecialDirTable) / sizeof(int);


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

	pdata->dwDrives = GetLogicalDrives();
	pdata->index = 0;
	pdata->sindex = 0;

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

	for ( ; pdata->index<26 ; pdata->index++ ){
		if ( pdata->dwDrives & (1 << pdata->index) ){
			break;
		}
	}

	if (pdata->index >= 26) {
		if (pdata->sindex < _SpecialDirTableCount) {
			swprintf(lpItemInfo->szItemName, L"%s", _SpecialDirName[pdata->sindex]);
			lpItemInfo->ullItemSize = 0;
			lpItemInfo->dwAttr = FILE_ATTRIBUTE_NORMAL;
			pdata->sindex++;

			return 1;
		}
		return -1;
	}

	wchar_t drive[8];
	swprintf(drive, L"%c:\\", (pdata->index + TEXT('A')) );

	wchar_t type[64];
	memset(type, 0, sizeof(type));
	int drv_type = GetDriveType(drive);
	switch(drv_type) {
	case DRIVE_REMOVABLE:
		swprintf(type, L"REMOVABLE");
		break;
	case DRIVE_FIXED:
		swprintf(type, L"HDD");
		break;
	case DRIVE_REMOTE:
		swprintf(type, L"REMOTE");
		break;
	case DRIVE_CDROM:
		swprintf(type, L"CD-ROM");
		break;
	case DRIVE_RAMDISK:
		swprintf(type, L"RAM");
		break;
	}

	wchar_t VolumeName[1000];
	wchar_t SystemName[1000];
	wcscpy(VolumeName, L"");
	wcscpy(SystemName, L"");

	// FDD対策
	//   - REMOVABLEでA,Bドライブは対象外
	if (drv_type != DRIVE_REMOVABLE || pdata->index > 1) {
		ULARGE_INTEGER Ava, Total, Free;
		if (GetDiskFreeSpaceEx(drive, &Ava, &Total, &Free)) {
			lpItemInfo->ullItemSize = Total.QuadPart;
		}

		DWORD SerialNumber;
		DWORD FileNameLength;
		DWORD Flags;
		BOOL ret = GetVolumeInformation(
			drive,
			VolumeName,
			sizeof(VolumeName),
			&SerialNumber,
			&FileNameLength,
			&Flags,
			SystemName,
			sizeof(SystemName));
		if (ret == FALSE) {
			memset(SystemName, 0, sizeof(SystemName));
			memset(VolumeName, 0, sizeof(VolumeName));
		}
	} else {
		lpItemInfo->ullItemSize = 0;
	}

	swprintf(lpItemInfo->szItemName, L"%C) %s/%s/%s", 
			(pdata->index + TEXT('A')), VolumeName, type, SystemName);
	//lpItemInfo->ullTimestamp = 0;
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
	char cmd[MAX_PATH+32];
    wchar_t label = szItemPath[0];
    const wchar_t* p = wcschr(szItemPath, L')');
    if (p != NULL) {
        label = *(--p);
    }

    if ( (label >= L'A' && label <= L'Z' ) ||
            (label >= L'a' && label <= L'z' ) ) {
        sprintf(cmd, "&EXCD -P\"%C:\\\"", label);
        AfxExec(pAfxApp, cmd);
        return 1;
    }

    int idx = label - L'0';
    if (idx >= 0 && idx <= _SpecialDirTableCount) {
        char path[MAX_PATH+1];
        wchar_t wpath[MAX_PATH+1];
        if (_SpecialDirTable[idx] == -1) {
            GetSystemDirectory(wpath, sizeof(wpath)/sizeof(wpath[0]));
        } else if (_SpecialDirTable[idx] == -2) {
            wcscpy(wpath, L"$V\"ProgramFiles\"");
        } else {
            LPITEMIDLIST PidList;
            SHGetSpecialFolderLocation(NULL, _SpecialDirTable[idx], &PidList);
            SHGetPathFromIDList(PidList, wpath);
            CoTaskMemFree(PidList);
        }

        wcstombs(path, wpath, sizeof(path));
        sprintf(cmd, "&EXCD -P\"%s\\\"", path);
        AfxExec(pAfxApp, cmd);
        return 1;
    }
	return 0;
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
	return ApiExecute(handle, szItemPath);
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
	wchar_t label = szFromItem[0];
	const wchar_t* p = wcschr(szFromItem, L')');
	if (p != NULL) {
		label = *(--p);
	}

	wchar_t path[MAX_PATH+1];
	swprintf(path, L"%s%s", szToPath, PLUGIN_NAME);

	if ( (label >= L'A' && label <= L'Z' ) ||
			(label >= L'a' && label <= L'z' ) ) {

		FILE* fp = _wfopen(path, L"w");
		if (fp == NULL) {
			return 1;
		}

		wchar_t drive[8];
		swprintf(drive, L"%c:\\", label );

		ULARGE_INTEGER Ava, Total, Free;
		GetDiskFreeSpaceEx(drive, &Ava, &Total, &Free);
		fwprintf(fp, L"<< %C drive >>\n", label);
		fwprintf(fp, L"Free Space:  %8d M\n", Free.QuadPart / 1024 / 1024 );
		fwprintf(fp, L"Disk Bytes:  %8d M\n", Total.QuadPart / 1024 / 1024 );
		fclose(fp);
		wcsncpy(szOutputPath, path, dwOutPathSize - 1);
		return 1;
	}

	int idx = label - L'0';
	if (idx >= 0 && idx <= _SpecialDirTableCount) {
        wchar_t wpath[MAX_PATH+1];
        if (_SpecialDirTable[idx] == -1) {
            GetSystemDirectory(wpath, sizeof(wpath)/sizeof(wpath[0]));
        } else if (_SpecialDirTable[idx] == -2) {
            wcscpy(wpath, L"$V\"ProgramFiles\"");
        } else {
            LPITEMIDLIST PidList;
            SHGetSpecialFolderLocation(NULL, _SpecialDirTable[idx], &PidList);
            SHGetPathFromIDList(PidList, wpath);
            CoTaskMemFree(PidList);
        }
		FILE* fp = _wfopen(path, L"w");
		if (fp == NULL) {
			return 1;
		}
		fwprintf(fp, L"Path:\n%s\n", wpath);
		fclose(fp);
		wcsncpy(szOutputPath, path, dwOutPathSize - 1);
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



