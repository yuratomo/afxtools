#pragma once

typedef int (*MIGEMO_PROC_CHAR2INT)(const unsigned char*, unsigned int*);
typedef int (*MIGEMO_PROC_INT2CHAR)(unsigned int, unsigned char*);

typedef struct _migemo migemo;

typedef migemo* (__stdcall *MIGEMO_OPEN)(const char* dict);
typedef void (__stdcall *MIGEMO_CLOSE)(migemo* object);
typedef unsigned char* (__stdcall *MIGEMO_QUERY)(migemo* object, const unsigned char* query);
typedef void (__stdcall *MIGEMO_RELEASE)(migemo* object, unsigned char* string);

typedef int (__stdcall *MIGEMO_SET_OPERATOR)(migemo* object, int index, const unsigned char* op);
typedef const unsigned char* (__stdcall *MIGEMO_GET_OPERATOR)(migemo* object,	int index);

typedef int (__stdcall *MIGEMO_LOAD)(migemo* obj, int dict_id, const char* dict_file);
typedef int (__stdcall *MIGEMO_IS_ENABLE)(migemo* obj);

typedef void (__stdcall *MIGEMO_SETPROC_CHAR2INT)(migemo* object, MIGEMO_PROC_CHAR2INT proc);
typedef void (__stdcall *MIGEMO_SETPROC_INT2CHAR)(migemo* object, MIGEMO_PROC_INT2CHAR proc);

static int __cdecl pcre_int2char(unsigned int in, unsigned char* out);
static int __cdecl pcre_char2int(const unsigned char*, unsigned int*);

class CMigemo
{
public:
	CMigemo();
	~CMigemo();
	bool LoadDLL(const TCHAR* path);
	bool Open(const TCHAR* dict_path);
	void Close();
	void UnloadDLL();
	void Query (const TCHAR *query, TCHAR **regex);
	bool IsEnable();

private:
	migemo *m_migemo;
	HMODULE m_hDLL;

	MIGEMO_OPEN migemo_open;
	MIGEMO_CLOSE migemo_close;
	MIGEMO_QUERY migemo_query;
	MIGEMO_RELEASE migemo_release;
	MIGEMO_IS_ENABLE migemo_is_enable;
	MIGEMO_SETPROC_CHAR2INT migemo_setproc_char2int;
	MIGEMO_SETPROC_INT2CHAR migemo_setproc_int2char;
};

