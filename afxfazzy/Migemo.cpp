#include <windows.h>
#include <tchar.h>
#include "Migemo.h"


CMigemo::CMigemo()
{
	migemo_open = NULL;
	migemo_close = NULL;
	migemo_query = NULL;
	migemo_release = NULL;
	migemo_is_enable = NULL;
	migemo_setproc_char2int = NULL;
	migemo_setproc_int2char = NULL;
}

CMigemo::~CMigemo()
{
	if (m_hDLL != NULL) {
		UnloadDLL();
	}
}

bool CMigemo::LoadDLL(const TCHAR *path)
{
	m_hDLL = LoadLibrary(path);

	if (m_hDLL != NULL) {
		migemo_open = (MIGEMO_OPEN)GetProcAddress(m_hDLL, "migemo_open");
		migemo_close = (MIGEMO_CLOSE)GetProcAddress(m_hDLL, "migemo_close");
		migemo_query = (MIGEMO_QUERY)GetProcAddress(m_hDLL, "migemo_query");
		migemo_release = (MIGEMO_RELEASE)GetProcAddress(m_hDLL, "migemo_release");
		migemo_is_enable = (MIGEMO_IS_ENABLE)GetProcAddress(m_hDLL, "migemo_is_enable");
		migemo_setproc_char2int = (MIGEMO_SETPROC_CHAR2INT)GetProcAddress(m_hDLL, "migemo_setproc_char2int");
		migemo_setproc_int2char = (MIGEMO_SETPROC_INT2CHAR)GetProcAddress(m_hDLL, "migemo_setproc_int2char");
	} else {
		MessageBox(NULL, _T("migemo.dll ‚ªŒ©‚Â‚©‚è‚Ü‚¹‚ñB"), _T("ƒGƒ‰["), MB_OK);
	}

	return m_hDLL != NULL;
}


bool CMigemo::Open(const TCHAR *dict_path)
{
	if (migemo_open != NULL) {
#ifdef UNICODE
		char path[MAX_PATH];
		wcstombs(path, dict_path, sizeof(path));
		m_migemo = migemo_open(path);
#else
		m_migemo = migemo_open(dict_path);
#endif

		migemo_setproc_int2char(m_migemo, pcre_int2char);
	}

	return m_migemo != NULL;
}

void CMigemo::Close()
{
	if (migemo_close != NULL) {
		if (migemo_is_enable(m_migemo)) {
			migemo_close(m_migemo);
		}
	}
}

void CMigemo::UnloadDLL()
{
	if (m_hDLL != NULL) {
		Close();
		FreeLibrary(m_hDLL);
		m_hDLL = NULL;
	}
}

void CMigemo::Query(const TCHAR *query, TCHAR **regex)
{
	if (migemo_query != NULL) {
		u_char *result;
#ifdef UNICODE
		int query_len = wcstombs(NULL, query, wcslen(query)*sizeof(TCHAR)+1);
		char *tmp_query = new char[query_len+1];
		wcstombs(tmp_query, query, wcslen(query)*sizeof(TCHAR)+1);

		result = migemo_query(m_migemo, (u_char *)tmp_query);
		delete [] tmp_query;

		int result_len = strlen((char *)result);
		*regex = new WCHAR[result_len*sizeof(WCHAR)+1];
		mbstowcs(*regex, (char *)result, result_len*sizeof(WCHAR)+1);
#else
		result = migemo_query(m_migemo, (u_char *)query);
		int result_len = strlen((char *)result);
		*regex = new TCHAR[result_len*sizeof(TCHAR)+1];
		strcpy(*regex, (char *)result);
#endif
		migemo_release(m_migemo, result);
	}
}

bool CMigemo::IsEnable()
{
	if (migemo_is_enable != NULL) {
		return migemo_is_enable(m_migemo) != 0;
	}

	return false;
}

static int __cdecl pcre_int2char(unsigned int in, unsigned char* out)
{
	if (in >= 0x100) {
		if (out) {
			out[0] = (unsigned char)((in >> 8) & 0xFF);
			out[1] = (unsigned char)(in & 0xFF);
		}
		return 2;
	} else {
		int len = 0;
		switch (in) {
			case '\\':
			case '.': case '*': case '^': case '$': case '/':
			case '[': case ']':
			case '(': case ')': case '{': case '}': case '|':
			case '+': case '?':
			if (out)
				out[len] = '\\';
			++len;
			default:
			if (out)
				out[len] = (unsigned char)(in & 0xFF);
			++len;
			break;
		}
		return len;
	}
}
