#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "afxcom.h"

IDispatch* pAfxApp = NULL;

/*
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
					 */
int wmain(int argc, wchar_t *argv[ ], wchar_t *envp[ ] )
{
	if (argc < 3) {
		printf("[usage]\n");
		printf("afxams.exe command param\n");
		printf("  ex)\n");
		printf("    afxams.exe MesPrint MesStr\n");
		printf("    afxams.exe Exec ComStr\n");
		printf("    afxams.exe Extract ComStr\n");
		printf("\n");
		return -1;
	}

	if (wcscmp(L"MesPrint", argv[1]) != 0 &&
	    wcscmp(L"Exec",     argv[1]) != 0 &&
	    wcscmp(L"Extract",  argv[1]) != 0 ) {
		return -1;
	}

	AfxInit(pAfxApp);

	int param_count = 0;
	for (int idx=2; idx<argc; idx++) {
		param_count += wcslen(argv[idx]);
		if (idx != argc-1) {
			param_count++;
		}
	}

	wchar_t* param = new wchar_t[param_count+1];
	memset(param, 0, param_count+1);
	for (int idx=2; idx<argc; idx++) {
		wcscat(param, argv[idx]);
		if (idx != argc-1) {
			wcscat(param, L" ");
		}
	}

	VARIANT x;
	x.vt = VT_BSTR;
	x.bstrVal = ::SysAllocString(param);

	VARIANT result;
	memset(&result, 0, sizeof(result));
	DWORD ret = AfxExec(DISPATCH_METHOD, &result, pAfxApp, (LPOLESTR)argv[1], 1, x);

	::SysFreeString(x.bstrVal);
	delete [] param;

	if (ret != 0) {
		return ret;
	}

	if (wcscmp(L"Extract", argv[1]) == 0 && result.bstrVal != NULL) {
		wprintf(result.bstrVal);
	}

	AfxCleanup(pAfxApp);

	return ret;
}

