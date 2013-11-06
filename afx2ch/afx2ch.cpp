#include <windows.h>
#include <Wininet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "afxcom.h"

#define AFX2CH_TITLE "afx2ch"

char g_curDir[MAX_PATH];
char g_path[MAX_PATH];
char g_type[32];
char g_url[1024];

void   replaceStr(char* ptr, char* from, char to);
LPBYTE getHttpDocument(char * lpszURL, void callback(char*));
void   callback1(char* test);
void   callback2(char* test);
void   callback3(char* test);

/**
 * メイン関数
 * @param[in] hInstance
 * @param[in] hPrevInstance
 * @param[in] lpCmdLine
 * @param[in] nCmdShow
 * @return 
 */
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	setlocale( LC_ALL, "Japanese" );
	if (lpCmdLine != NULL && strcmp(lpCmdLine, "") == 0) {
		::MessageBox(NULL, "[USAGE]\nafx2ch xxx.2ch\n\n", AFX2CH_TITLE, MB_OK);
		::MessageBox(NULL, lpCmdLine, AFX2CH_TITLE, MB_OK);
		return -1;
	}

	::GetCurrentDirectory(sizeof(g_curDir), g_curDir);
	sprintf(g_path, lpCmdLine);
	::GetPrivateProfileString("Config", "type", "", g_type, sizeof(g_type), g_path);
	::GetPrivateProfileString("Config", "url",  "", g_url,  sizeof(g_url),  g_path);
	if (strcmp(g_type, "") == 0 || strcmp(g_url, "") == 0) {
		::MessageBox(NULL, "invalid 2ch file", AFX2CH_TITLE, MB_OK);
		return -2;
	}

	IDispatch* pAfxApp;
	AfxInit(pAfxApp);

	char    mes[1024];
	wchar_t wmes[1024];
	sprintf(mes, "[afx2ch] Update -%s- %s", g_type, g_path);
	mbstowcs(wmes, mes, sizeof(wmes));

	VARIANT x;
	x.vt = VT_BSTR;
	x.bstrVal = ::SysAllocString(wmes);
	AfxExec(DISPATCH_METHOD, NULL, pAfxApp, L"MesPrint", 1, x);

	if (strcmp(g_type, "root") == 0) {
		getHttpDocument(g_url, callback1);
	} else if(strcmp(g_type, "bbs") == 0) { 
		getHttpDocument(g_url, callback2);
	} else if(strcmp(g_type, "thread") == 0) { 
		getHttpDocument(g_url, callback3);
	} else {
		::MessageBox(NULL, "illegal type", AFX2CH_TITLE, MB_OK);
	}

	sprintf(mes, "[afx2ch] End    -%s- %s", g_type, g_path);
	mbstowcs(wmes, mes, sizeof(wmes));
	x.bstrVal = ::SysAllocString(wmes);
	AfxExec(DISPATCH_METHOD, NULL, pAfxApp, L"MesPrint", 1, x);

	AfxCleanup(pAfxApp);
	return 0;
}

/* 指定URLからデータを読み込んでバイナリデータを返す */
LPBYTE getHttpDocument(char * lpszURL, void callback(char*))
{

    /* 読み込みバッファサイズ設定 */
    int iBufSize = 4096;

    /* 取得データ格納バッファサイズ設定 */
    //int iDocSize = iBufSize * 2;
	int iDocSize = 1024*1024;

    /* 取得データ格納バッファの現在位置初期化 */
    int iDocIndex = 0;

#if 0
    /* プロセスヒープのハンドル取得 */
    HANDLE hHeap = GetProcessHeap();

    /* バッファ確保 */
    LPBYTE lpbyBuf = (LPBYTE)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, iBufSize);
    LPBYTE lpbyDoc = (LPBYTE)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, iDocSize);
#else
    LPBYTE lpbyBuf = (LPBYTE)malloc(iBufSize);
    LPBYTE lpbyDoc = (LPBYTE)malloc(iDocSize);
#endif

    /* インターネットハンドル作成 */
    HINTERNET hInet = InternetOpen("afx2ch", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

    /* URLオープン */
    HINTERNET hFile = InternetOpenUrl(hInet, lpszURL, NULL, 0, INTERNET_FLAG_RELOAD, 0);

    DWORD dwLoadSize = 0;

    /* データ読み込み */
    while (InternetReadFile(hFile, lpbyBuf, iBufSize, &dwLoadSize) && dwLoadSize > 0) {

        /* データ格納バッファのサイズが足りなくなったら再確保 */
        if (iDocIndex + (int)dwLoadSize >= iDocSize) {

            iDocSize *= 2;

#if 0
            lpbyDoc = (LPBYTE)HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, lpbyDoc, iDocSize);
#else
            lpbyDoc = (LPBYTE)realloc(lpbyDoc, iDocSize);
			memset(lpbyDoc + iDocIndex, 0, iDocSize - iDocIndex);
#endif

        }

        /* 読み込みバッファのデータをデータ格納バッファに転送 */
#if 0
        CopyMemory(lpbyDoc + iDocIndex, lpbyBuf, dwLoadSize);
#else
		memcpy(lpbyDoc + iDocIndex, lpbyBuf, dwLoadSize);
#endif

        /* データ格納バッファの現在位置更新 */
        iDocIndex += dwLoadSize;

    }

	if (callback != NULL) {
		callback((char*)lpbyDoc);
	}

    /* インターネット関連ハンドル解放 */
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInet);

    /* 読み込みバッファ解放 */
#if 0
    HeapFree(hHeap, 0, lpbyBuf);
    HeapFree(hHeap, 0, lpbyDoc);
#else
	free(lpbyBuf);
	free(lpbyDoc);
#endif

    /* データを格納したバッファを返す */
    return lpbyDoc;

 }

void callback1(char* test)
{
	BOOL bRet;
	char path[MAX_PATH];
	char ourl[256];
	char* ptr = test;
	while (ptr != NULL) {
		char* HREF = strstr(ptr, "<A HREF=");
		char* href = strstr(ptr, "<a href=");
		if (href == 0 && HREF == 0) {
			break;
		} else if (href > 0 && HREF > 0) {
			if ( href > HREF) {
				href = HREF;
			}
		} else if (href == 0) {
			href = HREF;
		} else if (HREF == 0) {
		}
		ptr = href;
		char* url = href + 8;
		char* toji = strchr(ptr, '>');
		char* space = strchr(url, ' ');
		if (toji == 0) {
			ptr++;
			continue;
		}
		if (space != 0 && space < toji) {
			toji = space;
		}
		*toji = '\0';
		ptr = toji + 1;
		char* end = strchr(ptr, '<');
		if (end == 0) {
			continue;
		}
		*end = '\0';
		bRet = ::CreateDirectory(ptr, NULL);
		if (bRet == TRUE || GetLastError() == ERROR_ALREADY_EXISTS) {
			sprintf(path, "%s\\%s\\@update.2ch", g_curDir, ptr);
			sprintf(ourl, "%ssubback.html", url);
			::WritePrivateProfileString("Config", "type", "bbs", path);
			::WritePrivateProfileString("Config", "url",  ourl,  path);
		} else {
			//TRACE("CreateDirectory(%s) error", ptr);
		}
		ptr = end+1;
	}
}

void callback2(char* test)
{
	BOOL bRet;
	char path[MAX_PATH];
	char ourl[256];

	// base hrefでベースを解決
	char* base_href = strstr(test, "<base href=");
	char* BASE_HREF = strstr(test, "<BASE HREF=");
	if (base_href == 0 && BASE_HREF == 0) {
		return;
	} else if (base_href > 0 && BASE_HREF > 0) {
		if ( base_href > BASE_HREF) {
			base_href = BASE_HREF;
		}
	} else if (base_href == 0) {
		base_href = BASE_HREF;
	} else if (BASE_HREF == 0) {
	}
	char* base_url = base_href + 12;
	char* base_toji = strchr(base_url, '\"');
	if (base_toji == 0) {
		return;
	}
	*base_toji = '\0';

	char* ptr = base_toji+1;
	while (ptr != NULL) {
		char* HREF = strstr(ptr, "<A HREF=");
		char* href = strstr(ptr, "<a href=");
		if (href == 0 && HREF == 0) {
			break;
		} else if (href > 0 && HREF > 0) {
			if ( href > HREF) {
				href = HREF;
			}
		} else if (href == 0) {
			href = HREF;
		} else if (HREF == 0) {
		}
		ptr = href;
		char* url = href + 8 + 1; // "があるので+1
		char* toji = strstr(url, "\"");
		if (toji == 0) {
			continue;
		}
		*toji = '\0';
		ptr = strstr(toji+1, ": ");
		if (ptr == 0) {
			continue;
		}
		ptr += 2;
		char* end = strstr(ptr, " (");
		if (end == 0) {
			end = strstr(ptr, "<");
			if (end == 0) {
				continue;
			}
		}
		*end = '\0';
		sprintf(path, "%s\\%s.2ch", g_curDir, ptr);
		sprintf(ourl, "%s%s/", base_url, url);
		::WritePrivateProfileString("Config", "type", "thread", path);
		::WritePrivateProfileString("Config", "url",  ourl,  path);
		ptr = end+1;
	}
}

void callback3(char* test)
{
	FILE* fp = fopen(g_path, "w");
	if (fp == NULL) {
		return;
	}

	::WritePrivateProfileString("Config", "type", "thread", g_path);
	::WritePrivateProfileString("Config", "url",  g_url,  g_path);

	fseek(fp, 0, SEEK_END);
	fprintf(fp, "\n--\n");

	char* start;
	char* end;
	char* ptr = test;
	while (ptr != NULL) {
		// <dt> ～ <
		start = strstr(ptr, "<dt>");
		if (start == NULL) {
			break;
		}
		end = strstr(start+1, "<");
		if (end == NULL) {
			break;
		}
		*end = '\0';
		ptr = end + 1;
		fprintf(fp, start+4);

		// <b> ～ </b>
		start = strstr(ptr, "<b>");
		if (start == NULL) {
			break;
		}
		end = strstr(start+1, "</b>");
		if (end == NULL) {
			break;
		}
		*end = '\0';
		ptr = end + 1;
		fprintf(fp, start+3);

		// ：～ <dd>
		start = strstr(ptr, "：");
		if (start == NULL) {
			break;
		}
		end = strstr(start+1, "<dd>");
		if (end == NULL) {
			break;
		}
		*end = '\0';
		ptr = end + 4;
		fprintf(fp, start);
		fprintf(fp, "\n");

		// <dd> ～ \n
		start = ptr;
		end = strstr(ptr, "\n");
		if (end == NULL) {
			break;
		}
		*end = '\0';
		replaceStr(ptr, "<br>",   '\n');
		replaceStr(ptr, "<BR>",   '\n');
		replaceStr(ptr, "&lt;",   '<');
		replaceStr(ptr, "&gt;",   '>');
		replaceStr(ptr, "&nbsp;", ' ');
		replaceStr(ptr, "&amp;",  '&');
		replaceStr(ptr, "&yen;",  '\\');
		replaceStr(ptr, "&quot;", '\'');
		//replaceStr(ptr, "&#9745;",'□');

		fwrite(start, strlen(start), 1, fp);
		fprintf(fp, "\n");
		ptr = end + 1;
	}

	fclose(fp);
}

void replaceStr(char* ptr, char* from, char to)
{
	char* fstr;
	char* tmp = ptr;
	while (tmp != NULL) {
		fstr = strstr(tmp, from);
		if (fstr == NULL) {
			break;
		}
		*fstr = to;
		strcpy(fstr+1, fstr+strlen(from))+1;
		tmp = fstr+1;
		if (*tmp == '\0') {
			break;
		}
	}
}

