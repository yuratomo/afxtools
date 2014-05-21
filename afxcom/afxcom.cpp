#include "afxcom.h"

#define MAX_LINE_SIZE 1024

#ifdef _AFXCMD
#define MYCLSCTX	CLSCTX_LOCAL_SERVER
#else
#define MYCLSCTX	CLSCTX_INPROC_SERVER
#endif

int AfxInit(IDispatch* &pAfxApp)
{
	if (pAfxApp != NULL) {
		return -3;
	}

	CoInitialize(NULL);

	CLSID clsid;
	HRESULT hr = CLSIDFromProgID(L"AFXW.Obj", &clsid);
	if(FAILED(hr)) {
		return -1;
	}

	hr = CoCreateInstance(clsid, NULL, MYCLSCTX,
			IID_IDispatch, (void **)&pAfxApp);
	if(FAILED(hr)) {
		return -2;
	}

	return 0;
}

HRESULT AfxExec(int autoType, VARIANT *pvResult, IDispatch *pDisp, LPOLESTR ptName, int cArgs...) 
{
	HRESULT hr = E_FAIL;
	if(!pDisp) {
		return hr;
	}

	va_list marker;
	va_start(marker, cArgs);
	DISPPARAMS dp = { NULL, NULL, 0, 0 };
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPID dispID;

	hr = pDisp->GetIDsOfNames(IID_NULL, &ptName, 1, LOCALE_USER_DEFAULT, &dispID);
	if(FAILED(hr)) {
		return hr;
	}

	VARIANT *pArgs = new VARIANT[cArgs+1];
	for(int i=0; i<cArgs; i++) {
		pArgs[i] = va_arg(marker, VARIANT);
	}

	dp.cArgs = cArgs;
	dp.rgvarg = pArgs;

	if(autoType & DISPATCH_PROPERTYPUT) {
		dp.cNamedArgs = 1;
		dp.rgdispidNamedArgs = &dispidNamed;
	}

	hr = pDisp->Invoke(dispID, IID_NULL, LOCALE_SYSTEM_DEFAULT, autoType, 
			&dp, pvResult, NULL, NULL);
	if(FAILED(hr)) {
		return hr;
	}
	va_end(marker);

	delete [] pArgs;

	return hr;
}

int AfxExec(IDispatch *pDisp, const char* line)
{
	size_t len = strlen(line)+1;
	wchar_t* wbuf = new wchar_t[len];
	memset(wbuf, 0, 2*len);
	mbstowcs(wbuf, line, 2*len);
	wchar_t* cr = wcsstr(wbuf, L"\r");
	if (cr != NULL) *cr = L'\0';

	VARIANT x;
	x.vt = VT_BSTR;
	x.bstrVal = ::SysAllocString(wbuf);
	HRESULT ret = AfxExec(DISPATCH_METHOD, NULL, pDisp, L"Exec", 1, x);
	::SysFreeString(x.bstrVal);
	delete [] wbuf;

	if(FAILED(ret)) {
		return 0;
	}

	return 1;
}

void AfxCleanup(IDispatch* &pAfxApp)
{
	if (pAfxApp == NULL) {
		return;
	}

	pAfxApp->Release();

	CoUninitialize();

	pAfxApp = NULL;
}

void AfxTrace(IDispatch *pDisp, const char* line)
{
	size_t len = strlen(line)+1;
	wchar_t* wbuf = new wchar_t[len];
	memset(wbuf, 0, 2*len);
	mbstowcs(wbuf, line, 2*len);
	wchar_t* cr = wcsstr(wbuf, L"\r");
	if (cr != NULL) *cr = L'\0';

	VARIANT x;
	x.vt = VT_BSTR;
	x.bstrVal = ::SysAllocString(wbuf);
	AfxExec(DISPATCH_METHOD, NULL, pDisp, L"MesPrint", 1, x);
	::SysFreeString(x.bstrVal);
	delete [] wbuf;

	return;
}

void AfxTraceV(IDispatch *pDisp, const char* fmt, ...)
{
	char line[MAX_LINE_SIZE];
	va_list arg;
	va_start(arg, fmt);
	vsprintf(line, fmt, arg);
	va_end(arg);
	AfxTrace(pDisp, line);
}

