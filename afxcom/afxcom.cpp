#include "afxcom.h"

#define MAX_LINE_SIZE 1024

int AfxInit(IDispatch* &pAfxApp)
{
	if (pAfxApp != NULL) {
		return -3;
	}

	// Initialize COM for this thread...
	CoInitialize(NULL);

	// Get CLSID for Word.Application...
	CLSID clsid;
	HRESULT hr = CLSIDFromProgID(L"afxw.obj", &clsid);
	if(FAILED(hr)) {
		//::MessageBox(NULL, "CLSIDFromProgID() failed", "Error", 0x10010);
		return -1;
	}

	// Start Word and get IDispatch...
	;
	hr = CoCreateInstance(clsid, NULL, CLSCTX_LOCAL_SERVER, 
			IID_IDispatch, (void **)&pAfxApp);
	if(FAILED(hr)) {
		//::MessageBox(NULL, "Word not registered properly", "Error", 0x10010);
		return -2;
	}

	return 0;
}

// 
// AfxExec() - Automation helper function...
// 
HRESULT AfxExec(int autoType, VARIANT *pvResult, IDispatch *pDisp, LPOLESTR ptName, int cArgs...) 
{
	HRESULT hr = E_FAIL;
	if(!pDisp) {
		return hr;
	}

	// Begin variable-argument list...
	va_list marker;
	va_start(marker, cArgs);
	// Variables used...
	DISPPARAMS dp = { NULL, NULL, 0, 0 };
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	DISPID dispID;
	//char buf[200];
	//char szName[200];

	// Convert down to ANSI
	//WideCharToMultiByte(CP_ACP, 0, ptName, -1, szName, 256, NULL, NULL);

	// Get DISPID for name passed...
	hr = pDisp->GetIDsOfNames(IID_NULL, &ptName, 1, LOCALE_USER_DEFAULT, &dispID);
	if(FAILED(hr)) {
		return hr;
	}

	// Allocate memory for arguments...
	VARIANT *pArgs = new VARIANT[cArgs+1];
	// Extract arguments...
	for(int i=0; i<cArgs; i++) {
		pArgs[i] = va_arg(marker, VARIANT);
	}

	// Build DISPPARAMS
	dp.cArgs = cArgs;
	dp.rgvarg = pArgs;

	// Handle special-case for property-puts!
	if(autoType & DISPATCH_PROPERTYPUT) {
		dp.cNamedArgs = 1;
		dp.rgdispidNamedArgs = &dispidNamed;
	}

	// Make the call!
	hr = pDisp->Invoke(dispID, IID_NULL, LOCALE_SYSTEM_DEFAULT, autoType, 
			&dp, pvResult, NULL, NULL);
	if(FAILED(hr)) {
		return hr;
	}
	// End variable-argument section...
	va_end(marker);

	delete [] pArgs;

	return hr;
}

void AfxCleanup(IDispatch* &pAfxApp)
{
	if (pAfxApp == NULL) {
		return;
	}

	// Cleanup
	pAfxApp->Release();

	// Uninitialize COM for this thread...
	CoUninitialize();

	pAfxApp = NULL;
}

void AfxTrace(IDispatch *pDisp, const char* line)
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
	AfxExec(DISPATCH_METHOD, NULL, pDisp, L"MesPrint", 1, x);
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

