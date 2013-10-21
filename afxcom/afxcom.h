#ifndef __AFX_COM__
#define __AFX_COM__

#include <ole2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern int AfxInit(IDispatch* &pAfxApp);
extern HRESULT AfxExec(int autoType, VARIANT *pvResult, IDispatch *pDisp, LPOLESTR ptName, int cArgs...);
extern int  AfxExec(IDispatch *pDisp, const char* line);
extern void AfxCleanup(IDispatch* &pAfxApp);
extern void AfxTrace(IDispatch *pDisp, const char* line);
extern void AfxTraceV(IDispatch *pDisp, const char* fmt, ...);

#endif
