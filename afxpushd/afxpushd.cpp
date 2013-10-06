#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <share.h>
#include <locale.h>
#include "afxcom.h"

#define MAX_LINE_SIZE 1024
IDispatch* pAfxApp = NULL;

typedef enum {
	MODE_FIFO,
	MODE_CYCLE,
	MODE_NUMBER,
} Mode_E;

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

static void Message(const char* fmt, ...)
{
	char line[MAX_LINE_SIZE];
	va_list arg;
	va_start(arg, fmt);
	vsprintf(line, fmt, arg);
	va_end(arg);
	Message2(line);
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
	char mn__path[MAX_PATH];

	setlocale( LC_ALL, "Japanese" );

	if (__argc != 3 && __argc != 5 && __argc != 7) {
		Message2("[usage]");
		Message2("  afxpushd.exe [-n menu-number] left-dir-path right-dir-path [left-mask right-mask]");
		return -1;
	}

	AfxInit(pAfxApp);

	GetModuleFileName(NULL, ini_path, sizeof(ini_path));
	strcpy(mnu_path, ini_path);
	strcpy(mn__path, ini_path);
	int len = strlen(ini_path);
	ini_path[len-3] = 'i';
	ini_path[len-2] = 'n';
	ini_path[len-1] = 'i';
	mnu_path[len-3] = 'm';
	mnu_path[len-2] = 'n';
	mnu_path[len-1] = 'u';
	mn__path[len-3] = 'm';
	mn__path[len-2] = 'n';
	mn__path[len-1] = '_';

	Mode_E mode = MODE_FIFO;
	int max_num = 10;
	int menu_index = 1;
	char title[32] = "PUSHD";
	char sep[32] = ",";
	char tmp[1024];
	::GetPrivateProfileString("Config", "OutputPath", "", tmp, sizeof(tmp), ini_path);
	if (strcmp(tmp, "") != 0) {
		strncpy(mnu_path, tmp, sizeof(mnu_path)-1);
	}
	::GetPrivateProfileString("Config", "title", "", tmp, sizeof(tmp), ini_path);
	if (strcmp(tmp, "") != 0) {
		strncpy(title, tmp, sizeof(title)-1);
	}
	::GetPrivateProfileString("Config", "separator", "", tmp, sizeof(tmp), ini_path);
	if (strcmp(tmp, "") != 0) {
		strncpy(sep, tmp, sizeof(sep)-1);
	}
	int tmpCount = ::GetPrivateProfileInt("Config", "MaxCount", 0, ini_path);
	if (tmpCount > 0) {
		max_num = tmpCount;
	}
	::GetPrivateProfileString("Config", "mode", "FIFO", tmp, sizeof(tmp), ini_path);
	int cycle_start = 1;
	if (strcmp(tmp, "CYCLE") == 0) {
		mode = MODE_CYCLE;
		tmpCount = ::GetPrivateProfileInt("Config", "CycleStart", 1, ini_path);
		if (tmpCount > 0 && tmpCount <= max_num) {
			cycle_start = tmpCount;
		}
		tmpCount = ::GetPrivateProfileInt("Config", "CycleIndex", cycle_start, ini_path);
		if (tmpCount > 0 && tmpCount <= max_num) {
			menu_index = tmpCount;
		}
		if (menu_index < cycle_start) {
			menu_index = cycle_start;
		}
	}

	BOOL exist_mnu = FALSE;
	FILE* ofp;
	FILE* ifp = fopen(mnu_path, "r");
	if (ifp == NULL) {
		ofp = fopen(mnu_path, "w");
	} else {
		ofp = fopen(mn__path, "w");
		exist_mnu = TRUE;
	}
	if (ofp == NULL) {
		fclose(ifp);
		return -1;
	}

	int arg_base = 0;
	if (strcmp(__argv[1], "-n")==0) {
		arg_base = 2;
		menu_index = atoi(__argv[2]);
		mode = MODE_NUMBER;
	}

	fprintf(ofp, "AFX%s\n", title);
	char curline[MAX_LINE_SIZE];
	if (__argc-arg_base> 3) {
		sprintf(curline, "\"%-2d %s%s %s %s%s\" &EXCD -L\"%s\" -R\"%s\" -*L\"%s\" -*R\"%s\" \n", 
				menu_index,
				__argv[arg_base+1], __argv[arg_base+3], sep, __argv[arg_base+2], __argv[arg_base+4],
				__argv[arg_base+1], __argv[arg_base+2],      __argv[arg_base+3], __argv[arg_base+4]);
	} else {
		sprintf(curline, "\"%-2d %s %s %s\" &EXCD -L\"%s\" -R\"%s\" \n",
				menu_index,
				__argv[arg_base+1], sep, __argv[arg_base+2], __argv[arg_base+1], __argv[arg_base+2]);
	}

	if (exist_mnu) {
		for (int idx=0; idx<max_num+1; idx++) {
			if (fgets(line, sizeof(line)-1, ifp) == NULL) {
				if (idx > menu_index) {
					break;
				}
				strcpy(line, "\n");
			}
			if (idx == 0) {
				continue;
			}
			if (idx == menu_index) {
				fputs(curline, ofp);
			} else {
				if (mode == MODE_FIFO) {
					if (strcmp(line+2, curline+2) != 0) {
						char num[8];
						sprintf(num, "%-2d", idx+1);
						strncpy(line+1, num, 2);
						fputs(line, ofp);
					} else {
						idx--; // ÉXÉãÅ[
					}
				}
				else if (mode == MODE_CYCLE || mode == MODE_NUMBER) {
					fputs(line, ofp);
				}
			}
		}
		// afxpushd.mn_ Å® afxpushd.mnu
		fclose(ifp);
		fclose(ofp);
		DeleteFile(mnu_path);
		rename(mn__path, mnu_path);
	} else {
		if (arg_base == 2) {
			for (int idx=0; idx<menu_index-1; idx++) {
				fputs("\n", ofp);
			}
			fputs(curline, ofp);
		}
		fclose(ofp);
	}

	char target_num[32];
	sprintf(target_num, "%d", menu_index);
	if (__argc-arg_base > 3) {
		Message("[afxpushd] push %s ( %s%s %s %s%s )", 
			target_num, __argv[arg_base+1], __argv[arg_base+3], sep, __argv[arg_base+2], __argv[arg_base+4]);
	} else {
		Message("[afxpushd] push %s ( %s %s %s )", 
			target_num, __argv[arg_base+1], sep, __argv[arg_base+2]);
	}

	AfxCleanup(pAfxApp);

	if (mode == MODE_CYCLE) {
		tmpCount = menu_index + 1;
		if (tmpCount > max_num) {
			tmpCount = cycle_start;
		}
		sprintf(tmp, "%d", tmpCount);
		::WritePrivateProfileString("Config", "CycleIndex", tmp, ini_path);
	}

	return retval;
}

