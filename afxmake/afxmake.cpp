#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "afxcom.h"

#define PARAM_TYPE				0
#define PARAM_FILE				1
#define PARAM_END				2

#define TYPE_VC					0
#define TYPE_JAVA				1
#define TYPE_ANT				2

#define MODIFIER_ALT         0x0004
#define MODIFIER_CONTROL     0x0002
#define MODIFIER_SHIFT       0x0001

typedef struct _TokenInfo_T {
	char szNumberPostToken[8];
	char szNumberPreToken;
	char szWarningString[8];
	char szErrorString[8];
	BOOL bIncludeIllegalLine;
	char szStartFullPath[16];
} TokenInfo_T;
TokenInfo_T _token_info[] =
{
	{") : ", '(', "warning", "error", TRUE,  NULL},
	{": ",   ':', "warning", "",      TRUE,  NULL},
	{": ",   ':', "warning", "",      TRUE,  "[javac] "},
};

int _show_menu = 0;

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	char* pType = NULL;
	char* pFile = NULL;
	char  ini_path[MAX_PATH];
	char  edit_format[1024];
	char  pKeyCode = 0x37;
	int   pModifier = 1;

	// パラメタ解析
	if (__argc < 3) {
		return -2;
	}
	pType = __argv[1];
	pFile = __argv[2];
	if (pType == NULL || pFile == NULL) return -1;

	int type = -1;
	if (strcmp(pType, "java") == 0) {
		type = TYPE_JAVA;
	} else if (strcmp(pType, "vc") == 0) {
		type = TYPE_VC;
	} else if (strcmp(pType, "ant") == 0) {
		type = TYPE_ANT;
	}
	if (type == -1) {
		return -1;
	}

	// iniファイル解析
	char temp[8];
	GetModuleFileName(hInstance, ini_path, sizeof(ini_path));
	int len = strlen(ini_path);
	ini_path[len-1] = 'i';
	ini_path[len-2] = 'n';
	ini_path[len-3] = 'i';
	::GetPrivateProfileString("Config", "format", "&EDIT +%d \"%s\"", edit_format, sizeof(edit_format), ini_path);
	_show_menu = ::GetPrivateProfileInt("Config", "exec_mode", 0, ini_path);
	::GetPrivateProfileString("SendKey", "keycode", "1077", temp, sizeof(temp), ini_path);
	sscanf(temp, "%1d%3d", &pModifier, &pKeyCode);
	

	// 結果ファイル加工
	FILE* fi = fopen(pFile, "r");
	if (fi == NULL) {
		return -1;
	}
	FILE* fo = fopen(".afxmake", "w");
	if (fo == NULL) {
		fclose(fi);
		return -1;
	}
	FILE* fw = fopen(".afxmakew", "w");
	if (fw == NULL) {
		fclose(fi);
		return -1;
	}

	FILE* fp = fo;
	int idx = 0;
	char line[128];
	char buf[1024];
	int  last_num = 0;
	char last_path[MAX_PATH] = "";
	while (fgets(buf, sizeof(buf), fi)) {
		int len = strlen(buf);
		if (len > 0) {
			buf[len-1] = '\0';
		}
		if (idx == 0) {
			fprintf(fo, "afx %s\n", buf);
			fprintf(fw, "afx warning list\n");
			idx++;
			continue;
		}

		bool hit = false;
		char* tmp = strstr(buf, _token_info[type].szNumberPostToken);
		if (tmp != NULL) {
			*tmp = '\0';
			char* comment = tmp + strlen(_token_info[type].szNumberPostToken);
			tmp = strrchr(buf, _token_info[type].szNumberPreToken);
			if (tmp != NULL) {
				*tmp = '\0';
				char* num = tmp+1;
				char* path = buf;
				if (_token_info[type].szStartFullPath != NULL) {
					path = strstr(path, _token_info[type].szStartFullPath);
					if (path == NULL) {
						continue;
					}
					path += strlen(_token_info[type].szStartFullPath);
				}
				while (*path == ' ') {
					path++;
				}
				if (type == TYPE_VC) {
					if (*(path+1) == '>') {
						path+=2;
					}
				}
				if (strncmp(comment, _token_info[type].szWarningString, 
						strlen(_token_info[type].szWarningString)) == 0) {
					fp = fw;
				} else if (	strcmp(_token_info[type].szErrorString, "") == 0 ||
							strncmp(comment, _token_info[type].szErrorString, 
								strlen(_token_info[type].szErrorString)) == 0) {
					fp = fo;
				}

				int clen = strlen(comment);
				int cpos = 0;
				bool first = true;
				while (cpos < clen) {
					memset(line, 0, sizeof(line));
					strncpy(line, comment+cpos, sizeof(line)-1);
					if (first == true) {
						fprintf(fp, "\"%-4d %s\"	", idx, line);
						first = false;
					} else {
						fprintf(fp, "\"     %s\"	",      line);
					}
					fprintf(fp, edit_format, atoi(num), path);
					fprintf(fp, "\n");
					cpos += strlen(line);
				}
				last_num = atoi(num);
				strcpy(last_path, path);
				hit = true;
				idx++;
			}
		}
		if (hit == false && _token_info[type].bIncludeIllegalLine == TRUE) {
			if (strcmp(last_path, "") == 0) {
				continue;
			}
			char* comment = buf;
			if (_token_info[type].szStartFullPath != NULL) {
				comment = strstr(comment, _token_info[type].szStartFullPath);
				if (comment == NULL) {
					continue;
				}
				comment += strlen(_token_info[type].szStartFullPath);
			}
			int clen = strlen(comment);
			int cpos = 0;
			while (cpos < clen) {
				memset(line, 0, sizeof(line));
				strncpy(line, comment+cpos, sizeof(line)-1);
				fprintf(fp, "\"     %s\"	", line);
				fprintf(fp, edit_format, last_num, last_path);
				fprintf(fp, "\n");
				cpos += strlen(line);
			}
		}
	}
	if (idx > 1) {
		fprintf(fo, "\"warning list\"	&MENU .afxmakew\n");
		if (type == TYPE_VC) {
			fprintf(fo, buf);
		}
	}
	fprintf(fw, "\"go back\"	&MENU .afxmake\n");

	fclose(fi);
	fclose(fo);
	fclose(fw);

	if (_show_menu == 1) {
		IDispatch* pAfxApp = NULL;
		AfxInit(pAfxApp);
		VARIANT x;
		x.vt = VT_BSTR;
		x.bstrVal = ::SysAllocString(L"&MENU .afxmake");
		VARIANT result;
		AfxExec(DISPATCH_METHOD, &result, pAfxApp, L"Exec", 1, x);
		AfxCleanup(pAfxApp);
	} else if (_show_menu == 2) {
		// あふにキーコードを送る
		HWND hWndAfx = ::FindWindow("TAfxWForm", NULL);
		if (hWndAfx == INVALID_HANDLE_VALUE) {
			hWndAfx = ::FindWindow("TAfxForm", NULL);
			if (hWndAfx == INVALID_HANDLE_VALUE) {
				return -1;
			}
		}

		DWORD buf;
		DWORD pid;
		DWORD tid = ::GetWindowThreadProcessId(hWndAfx, &pid);
		DWORD tidto = ::GetCurrentThreadId();
		::AttachThreadInput(tid, tidto, TRUE);
		::SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &buf, 0);
		::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, NULL, 0);

		::SetForegroundWindow( hWndAfx );
		::BringWindowToTop( hWndAfx );

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

	return 0;
}

