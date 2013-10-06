#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <share.h>
#include <locale.h>
#include "afxcom.h"

#define MAX_LINE_SIZE 1024
IDispatch* pAfxApp = NULL;

static void Message2(const char* line)
{
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
{
	int retval = 0;
	char line[MAX_LINE_SIZE];
	char ini_path[MAX_PATH];
	char mnu_path[MAX_PATH];

	setlocale( LC_ALL, "Japanese" );
	
	if (__argc != 3) {
		Message2("[usage]");
		Message2("  afxpopd.exe -n menu-number");
		return -1;
	}

	AfxInit(pAfxApp);

	GetModuleFileName(NULL, ini_path, sizeof(ini_path));
	strcpy(mnu_path, ini_path);
	int len = strlen(ini_path);
	if (len+2 >= MAX_PATH) {
		return -1;
	}
	ini_path[len-7] = 'u';
	ini_path[len-6] = 's';
	ini_path[len-5] = 'h';
	ini_path[len-4] = 'd';
	ini_path[len-3] = '.';
	ini_path[len-2] = 'i';
	ini_path[len-1] = 'n';
	ini_path[len  ] = 'i';
	ini_path[len+1] = '\0';
	mnu_path[len-7] = 'u';
	mnu_path[len-6] = 's';
	mnu_path[len-5] = 'h';
	mnu_path[len-4] = 'd';
	mnu_path[len-3] = '.';
	mnu_path[len-2] = 'm';
	mnu_path[len-1] = 'n';
	mnu_path[len  ] = 'u';
	mnu_path[len+1] = '\0';

	char tmp[1024];
	::GetPrivateProfileString("Config", "OutputPath", "", tmp, sizeof(tmp), ini_path);
	if (strcmp(tmp, "") != 0) {
		strncpy(mnu_path, tmp, sizeof(mnu_path)-1);
	}
	int max_num = 10;
	int tmpCount = ::GetPrivateProfileInt("Config", "MaxCount", 0, ini_path);
	if (tmpCount > 0) {
		max_num = tmpCount;
	}

	FILE* ifp = fopen(mnu_path, "r");
	if (ifp == NULL) {
		return -1;
	}

	int menu_index = atoi(__argv[2]);

	for (int idx=0; idx<max_num-1; idx++) {
		if (fgets(line, sizeof(line)-1, ifp) == NULL) {
			break;
		}
		if (idx == 0) {
			continue;
		}
		if (idx == menu_index) {
			wchar_t wbuf[1024];
			char *d1, *d2, *d3;
			d1 = strchr(line, '\"');
			if (d1 == NULL) continue;
			d2 = strchr(d1+1, '\"');
			if (d2 == NULL) continue;
			d3 = d2+1;
			while ( (*d3 == ' ') || (*d3 == '\t') ) {
				d3++;
			}
			memset(wbuf, 0, sizeof(wbuf));
			d1 = strchr(d3, '\n');
			if (d1 != NULL) {
				*d1 = '\0';
			}
			mbstowcs(wbuf, d3, sizeof(wbuf));
			VARIANT x;
			x.vt = VT_BSTR;
			x.bstrVal = ::SysAllocString(wbuf);
			AfxExec(DISPATCH_METHOD, NULL, pAfxApp, L"MesPrint", 1, x);
			AfxExec(DISPATCH_METHOD, NULL, pAfxApp, L"Exec", 1, x);
			break;
		}
	}

	fclose(ifp);
	AfxCleanup(pAfxApp);

	return retval;
}

