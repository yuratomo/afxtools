#include "IEComponent.h"

typedef BOOL (WINAPI*ATLAXWININIT)();
typedef HRESULT (WINAPI*ATLAXGETCONTROL)(HWND,IUnknown **);

IEComponent::IEComponent( HWND hOwnerWnd, HINSTANCE hInst, int ix, int iy, int iw, int ih )
{
	int iCount;
	char szWndCls[32];
	m_hAtl = NULL;
	m_lpUnkown = NULL;
	m_lpIE = NULL;

	/*
	// ATL1.0/7.1/8.1のDLLを呼び出す
	for( iCount = 0; iCount < 3; iCount ++ )
	{
		if( iCount == 0 )
		{
			// ATL8.0
			m_hAtl = LoadLibrary( "atl80.dll" );
			if( m_hAtl )
			{
				lstrcpy( szWndCls, "AtlAxWin80" );
				break;
			}
		}
		if( iCount == 1 )
		{
			// ATL7.1
			m_hAtl = LoadLibrary( "atl71.dll" );
			if( m_hAtl )
			{
				lstrcpy( szWndCls, "AtlAxWin71" );
				break;
			}
		}
		if( iCount == 2 )
		{
			// ATL1.0
			m_hAtl = LoadLibrary( "atl.dll" );
			if( m_hAtl )
			{
				lstrcpy( szWndCls, "AtlAxWin" );
				break;
			}
		}
	}

	// DLLが見つからない
	if( iCount == 3 )
		return;
	*/
	// ATL1.0
	m_hAtl = LoadLibrary( "atl.dll" );
	if( m_hAtl )
	{
		lstrcpy( szWndCls, "AtlAxWin" );
	}
	else
	{
		return;
	}

	// COMを初期化
	CoInitialize( NULL );
	ATLAXWININIT AtlAxWinInit = (ATLAXWININIT)GetProcAddress( m_hAtl, "AtlAxWinInit" );
	AtlAxWinInit();

	//IEのウィンドウの作成
	m_hWnd = CreateWindow( szWndCls, "Shell.Explorer.2", WS_CHILD|WS_VISIBLE|WS_MAXIMIZE, ix, iy , iw, ih, hOwnerWnd, (HMENU)0, hInst, NULL );
	if(!m_hWnd)
		return;

	//IUnkownを取得
	ATLAXGETCONTROL AtlAxGetControl = (ATLAXGETCONTROL)GetProcAddress( m_hAtl, "AtlAxGetControl" );
	if( AtlAxGetControl(m_hWnd, &m_lpUnkown) == S_OK )
	{
		//IID_IWebBrowser2を検索
		m_lpUnkown->QueryInterface(IID_IWebBrowser2,(void **)&m_lpIE );

		//IID_IOleInPlaceActiveObject
		m_lpUnkown->QueryInterface(IID_IOleInPlaceActiveObject, (void**)&m_actObj);

		//LoadPage("http://www.yahoo.co.jp/");
		//m_lpIE->GoHome();
		m_lpIE->raw_GoSearch();
		int test = 0;
	}
}
IEComponent::~IEComponent( void )
{
	if( m_lpIE )
	{
		m_lpIE->Release();
		m_lpIE = NULL;
	}
	if( m_lpUnkown )
	{
		m_lpUnkown->Release();
		m_lpUnkown = NULL;
	}
	if( m_actObj ) {
		m_actObj->Release();
		m_actObj = NULL;
	}
	CoUninitialize();
	if( m_hAtl )
	{
		FreeLibrary( m_hAtl );
		m_hAtl = NULL;
	}
}

void IEComponent::LoadPage( LPCSTR lpszPageAddr )
{
	m_lpIE->Navigate2( &_variant_t(lpszPageAddr), 0, NULL, NULL, NULL );
}

IOleInPlaceActiveObject* IEComponent::GetActObj()
{
	return m_actObj;
}

HWND IEComponent::GetWndIE()
{
	return m_hWnd;
}


