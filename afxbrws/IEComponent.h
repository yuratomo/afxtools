#ifndef __CIECOMPONENT__
#define __CIECOMPONENT__

#include <windows.h>
#include <mshtml.h>

#import "libid:EAB22AC0-30C1-11CF-A7EB-0000C05BAE0B" no_namespace named_guids
// #import "shdocvw.dll" no_namespace named_guids

class IEComponent
{
private:
	HWND m_hWnd;
	HMODULE m_hAtl;
	IUnknown *m_lpUnkown;
	IWebBrowser2* m_lpIE;
	IOleInPlaceActiveObject* m_actObj;

public:
	IEComponent( HWND hOwnerWnd, HINSTANCE hInst, int ix, int iy, int iw, int ih );
	~IEComponent();
	void LoadPage( LPCSTR lpszPageAddr );
	IOleInPlaceActiveObject* GetActObj();
	HWND GetWndIE();
};

#endif // __CIECOMPONENT__
